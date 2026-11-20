#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Sample {
    int run;
    double seconds;
};

struct Benchmark {
    std::string model;
    int rays = 0;
    std::vector<Sample> samples;

    double mean_seconds() const {
        double total = 0.0;
        for (const Sample& sample : samples)
            total += sample.seconds;
        return total / samples.size();
    }
};

std::vector<std::string> split_csv_row(const std::string& row) {
    std::vector<std::string> fields;
    std::stringstream stream(row);
    std::string field;
    while (std::getline(stream, field, ','))
        fields.push_back(field);
    return fields;
}

int parse_int(const std::string& value, const std::string& column) {
    std::size_t parsed = 0;
    const int result = std::stoi(value, &parsed);
    if (parsed != value.size())
        throw std::runtime_error("invalid " + column + " value: " + value);
    return result;
}

double parse_double(const std::string& value, const std::string& column) {
    std::size_t parsed = 0;
    const double result = std::stod(value, &parsed);
    if (parsed != value.size() || !std::isfinite(result) || result < 0.0)
        throw std::runtime_error("invalid " + column + " value: " + value);
    return result;
}

Benchmark read_benchmark(const std::string& path, const std::string& expected_model) {
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("could not open benchmark CSV: " + path);

    std::string line;
    if (!std::getline(file, line))
        throw std::runtime_error("benchmark CSV is empty: " + path);
    const std::vector<std::string> header = split_csv_row(line);

    const auto column = [&header, &path](const std::string& name) {
        const auto it = std::find(header.begin(), header.end(), name);
        if (it == header.end())
            throw std::runtime_error("CSV is missing column '" + name + "': " + path);
        return static_cast<std::size_t>(it - header.begin());
    };
    const std::size_t model_column = column("model");
    const std::size_t rays_column = column("rays");
    const std::size_t run_column = column("run");
    const std::size_t time_column = column("wall_time_s");

    Benchmark benchmark;
    benchmark.model = expected_model;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#')
            break;
        const std::vector<std::string> fields = split_csv_row(line);
        const std::size_t largest_column =
            std::max(std::max(model_column, rays_column), std::max(run_column, time_column));
        if (fields.size() <= largest_column)
            throw std::runtime_error("malformed benchmark CSV row in: " + path);
        if (fields[model_column] != expected_model)
            throw std::runtime_error("expected model '" + expected_model + "' in: " + path);

        const int rays = parse_int(fields[rays_column], "rays");
        const int run = parse_int(fields[run_column], "run");
        const double seconds = parse_double(fields[time_column], "wall_time_s");
        if (rays < 1 || run < 1)
            throw std::runtime_error("rays and run numbers must be positive in: " + path);
        if (benchmark.rays != 0 && benchmark.rays != rays)
            throw std::runtime_error("ray count changes within benchmark CSV: " + path);

        benchmark.rays = rays;
        benchmark.samples.push_back({run, seconds});
    }

    if (benchmark.samples.empty())
        throw std::runtime_error("no measured runs found in: " + path);
    return benchmark;
}

double x_position(int run, int max_run, double left, double width) {
    if (max_run <= 1)
        return left + width / 2.0;
    return left + (static_cast<double>(run - 1) / (max_run - 1)) * width;
}

double y_position(double milliseconds, double max_milliseconds,
                  double top, double height) {
    return top + height - (milliseconds / max_milliseconds) * height;
}

std::string fixed(double value, int precision = 2) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

void draw_series(std::ofstream& svg, const Benchmark& benchmark,
                 const std::string& color, int max_run, double max_milliseconds,
                 double left, double top, double width, double height) {
    const double mean_milliseconds = benchmark.mean_seconds() * 1000.0;
    const double mean_y = y_position(mean_milliseconds, max_milliseconds, top, height);
    svg << "<line x1=\"" << left << "\" y1=\"" << mean_y
        << "\" x2=\"" << left + width << "\" y2=\"" << mean_y
        << "\" stroke=\"" << color << "\" stroke-dasharray=\"7 5\" "
        << "stroke-width=\"2\" opacity=\"0.8\"/>\n";

    svg << "<polyline fill=\"none\" stroke=\"" << color
        << "\" stroke-width=\"2\" points=\"";
    for (const Sample& sample : benchmark.samples) {
        svg << x_position(sample.run, max_run, left, width) << ','
            << y_position(sample.seconds * 1000.0, max_milliseconds, top, height) << ' ';
    }
    svg << "\"/>\n";

    for (const Sample& sample : benchmark.samples) {
        svg << "<circle cx=\"" << x_position(sample.run, max_run, left, width)
            << "\" cy=\"" << y_position(sample.seconds * 1000.0, max_milliseconds, top, height)
            << "\" r=\"2.5\" fill=\"" << color << "\"/>\n";
    }
}

void write_plot(const std::string& path,
                const Benchmark& sequential,
                const Benchmark& simd) {
    if (sequential.rays != simd.rays)
        throw std::runtime_error("CSV files have different ray counts; cannot compare them");

    int max_run = 1;
    double max_milliseconds = 0.0;
    for (const Benchmark* benchmark : {&sequential, &simd}) {
        for (const Sample& sample : benchmark->samples) {
            max_run = std::max(max_run, sample.run);
            max_milliseconds = std::max(max_milliseconds, sample.seconds * 1000.0);
        }
    }
    max_milliseconds = max_milliseconds > 0.0 ? max_milliseconds * 1.1 : 1.0;

    constexpr double svg_width = 1000.0;
    constexpr double svg_height = 620.0;
    constexpr double left = 95.0;
    constexpr double top = 100.0;
    constexpr double plot_width = 830.0;
    constexpr double plot_height = 410.0;

    const std::filesystem::path output(path);
    if (output.has_parent_path())
        std::filesystem::create_directories(output.parent_path());
    std::ofstream svg(path);
    if (!svg)
        throw std::runtime_error("could not create SVG plot: " + path);

    svg << std::fixed << std::setprecision(2);
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << svg_width
        << "\" height=\"" << svg_height << "\" viewBox=\"0 0 "
        << svg_width << ' ' << svg_height << "\">\n"
        << "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n"
        << "<text x=\"500\" y=\"38\" text-anchor=\"middle\" "
        << "font-family=\"sans-serif\" font-size=\"24\" font-weight=\"bold\">"
        << "Tiempo por frame: secuencial vs SIMD</text>\n"
        << "<text x=\"500\" y=\"66\" text-anchor=\"middle\" "
        << "font-family=\"sans-serif\" font-size=\"14\">"
        << sequential.rays << " rayos por frame</text>\n";

    for (int tick = 0; tick <= 5; ++tick) {
        const double value = max_milliseconds * tick / 5.0;
        const double y = y_position(value, max_milliseconds, top, plot_height);
        svg << "<line x1=\"" << left << "\" y1=\"" << y
            << "\" x2=\"" << left + plot_width << "\" y2=\"" << y
            << "\" stroke=\"#dddddd\" stroke-width=\"1\"/>\n"
            << "<text x=\"" << left - 12 << "\" y=\"" << y + 5
            << "\" text-anchor=\"end\" font-family=\"sans-serif\" font-size=\"12\">"
            << fixed(value) << "</text>\n";
    }

    const int x_ticks = std::min(max_run, 5);
    for (int tick = 0; tick < x_ticks; ++tick) {
        const int run = x_ticks == 1
            ? 1
            : 1 + tick * (max_run - 1) / (x_ticks - 1);
        const double x = x_position(run, max_run, left, plot_width);
        svg << "<line x1=\"" << x << "\" y1=\"" << top
            << "\" x2=\"" << x << "\" y2=\"" << top + plot_height
            << "\" stroke=\"#eeeeee\" stroke-width=\"1\"/>\n"
            << "<text x=\"" << x << "\" y=\"" << top + plot_height + 22
            << "\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"12\">"
            << run << "</text>\n";
    }

    svg << "<line x1=\"" << left << "\" y1=\"" << top + plot_height
        << "\" x2=\"" << left + plot_width << "\" y2=\"" << top + plot_height
        << "\" stroke=\"#333333\"/>\n"
        << "<line x1=\"" << left << "\" y1=\"" << top
        << "\" x2=\"" << left << "\" y2=\"" << top + plot_height
        << "\" stroke=\"#333333\"/>\n"
        << "<text x=\"510\" y=\"575\" text-anchor=\"middle\" "
        << "font-family=\"sans-serif\" font-size=\"15\">Ejecución medida</text>\n"
        << "<text x=\"25\" y=\"305\" text-anchor=\"middle\" "
        << "font-family=\"sans-serif\" font-size=\"15\" "
        << "transform=\"rotate(-90 25 305)\">Tiempo por frame (ms)</text>\n";

    draw_series(svg, sequential, "#4C72B0", max_run, max_milliseconds,
                left, top, plot_width, plot_height);
    draw_series(svg, simd, "#DD8452", max_run, max_milliseconds,
                left, top, plot_width, plot_height);

    const double legend_y = 90.0;
    svg << "<line x1=\"320\" y1=\"" << legend_y << "\" x2=\"350\" y2=\"" << legend_y
        << "\" stroke=\"#4C72B0\" stroke-width=\"2\"/>\n"
        << "<text x=\"358\" y=\"" << legend_y + 5
        << "\" font-family=\"sans-serif\" font-size=\"13\">sequential (media "
        << fixed(sequential.mean_seconds() * 1000.0, 4) << " ms)</text>\n"
        << "<line x1=\"600\" y1=\"" << legend_y << "\" x2=\"630\" y2=\"" << legend_y
        << "\" stroke=\"#DD8452\" stroke-width=\"2\"/>\n"
        << "<text x=\"638\" y=\"" << legend_y + 5
        << "\" font-family=\"sans-serif\" font-size=\"13\">simd (media "
        << fixed(simd.mean_seconds() * 1000.0, 4) << " ms)</text>\n"
        << "</svg>\n";
    if (!svg)
        throw std::runtime_error("failed while writing SVG plot: " + path);
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: plot_benchmark <sequential.csv> <simd.csv> [output.svg]\n";
        return 2;
    }

    try {
        const std::string output =
            argc == 4 ? argv[3] : "results/plots/benchmark_times.svg";
        const Benchmark sequential = read_benchmark(argv[1], "sequential");
        const Benchmark simd = read_benchmark(argv[2], "simd");
        write_plot(output, sequential, simd);

        std::cout << "Plot: " << output << '\n'
                  << "Runs: sequential=" << sequential.samples.size()
                  << ", simd=" << simd.samples.size() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}

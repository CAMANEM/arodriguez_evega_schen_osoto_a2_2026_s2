/**
 * @file main_benchmark.cpp
 * @brief Ejecuta campañas de benchmark y exporta las métricas de cada modelo.
 */
#include "core/config/raytracing_config.hpp"
#include "core/strategies/IRenderer.h"
#include "core/strategies/RendererFactory.h"
#include "metrics_interface.hpp"
#include "core/camera/CameraOrbit.h"
#include "core/utils/Timer.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

/** @brief Nombre, etiqueta estadística y cantidad de workers de un modelo. */
struct ModelConfig {
	const char* name;
	execution_model model;
	int workers;
};

struct BenchmarkMetrics {
	metrics_interface wall;
	struct FrameSample {
		int frame;
		double execution_time_s;
		long long virtual_time_ns;
		long long stall_time_ns;
		long long context_switch_time_ns;
		int stall_count;
		int context_switches;
	};
	std::vector<FrameSample> samples;

	explicit BenchmarkMetrics(execution_model model, int workers)
		: wall(model, workers) {}

	double mean_virtual_time_ns() const;
	double virtual_speedup(double sequential_time_ns) const {
		const double model_time_ns = mean_virtual_time_ns();
		if (sequential_time_ns <= 0.0 || model_time_ns <= 0.0) return 0.0;
		return sequential_time_ns / model_time_ns;
	}
};

struct SummaryStats {
	double mean = 0.0;
	double stddev = 0.0;
	double ci_lower = 0.0;
	double ci_upper = 0.0;
};

template <typename Value>
static SummaryStats summarize(const std::vector<BenchmarkMetrics::FrameSample>& samples,
							  Value value) {
	SummaryStats stats;
	if (samples.empty()) return stats;

	long double sum = 0.0L;
	for (const auto& sample : samples) sum += value(sample);
	stats.mean = static_cast<double>(sum / samples.size());

	if (samples.size() < 2) {
		stats.ci_lower = stats.mean;
		stats.ci_upper = stats.mean;
		return stats;
	}

	long double squared_differences = 0.0L;
	for (const auto& sample : samples) {
		const long double difference = value(sample) - stats.mean;
		squared_differences += difference * difference;
	}
	stats.stddev = std::sqrt(static_cast<double>(
		squared_differences / static_cast<long double>(samples.size() - 1)));
	const double half_width = 1.96 * stats.stddev /
		std::sqrt(static_cast<double>(samples.size()));
	stats.ci_lower = stats.mean - half_width;
	stats.ci_upper = stats.mean + half_width;
	return stats;
}

double BenchmarkMetrics::mean_virtual_time_ns() const {
	return summarize(samples, [](const FrameSample& sample) {
		return static_cast<double>(sample.virtual_time_ns);
	}).mean;
}

static double speedup_ci_half_width(const SummaryStats& sequential,
									const SummaryStats& model,
									std::size_t sequential_runs,
									std::size_t model_runs) {
	if (sequential.mean <= 0.0 || model.mean <= 0.0 ||
		sequential_runs < 2 || model_runs < 2)
		return 0.0;

	const double speedup = sequential.mean / model.mean;
	const double seq_relative_error = sequential.stddev /
		(std::sqrt(static_cast<double>(sequential_runs)) * sequential.mean);
	const double model_relative_error = model.stddev /
		(std::sqrt(static_cast<double>(model_runs)) * model.mean);
	return 1.96 * speedup * std::sqrt(
		seq_relative_error * seq_relative_error +
		model_relative_error * model_relative_error);
}

static const ModelConfig models[] = {
	{"sequential", execution_model::sequential, 1},
	{"fgmt", execution_model::fine_grained, constants::NUM_THREADS},
	{"cgmt", execution_model::coarse_grained, constants::NUM_THREADS},
	{"smt", execution_model::smt, constants::smt_context_count()},
	{"cmp", execution_model::cmp, constants::CMP_NUM_CORES}
};

/**
 * @brief Busca la configuración de un modelo registrado.
 * @param name Nombre CLI del modelo.
 * @return Referencia a su configuración estática.
 * @throws std::invalid_argument Si el modelo no está registrado.
 */
static const ModelConfig& find_model(const std::vector<ModelConfig>& configured_models,
									 const std::string& name) {
	for (const ModelConfig& model : configured_models)
		if (name == model.name) return model;
	throw std::invalid_argument("unknown model: " + name);
}

static int parse_positive_integer(const std::string& value, const char* option) {
	std::size_t parsed = 0;
	const int result = std::stoi(value, &parsed);
	if (parsed != value.size() || result < 1)
		throw std::invalid_argument(std::string(option) + " must be a positive integer");
	return result;
}

/**
 * @brief Renderiza varias repeticiones de un modelo y recoge tiempos de pared.
 * @param config Modelo y cantidad de workers.
 * @param runs Número de repeticiones.
 * @param last_frame Si no es nulo, recibe el último frame producido.
 * @return Métricas de las muestras renderizadas, en segundos.
 */
static BenchmarkMetrics run_model(const ModelConfig& config, int runs,
								  const std::string& frame_csv_path,
								  bool write_frame_csv) {
	std::unique_ptr<IRenderer> renderer =
		RendererFactory::create(config.name, config.workers);
	renderer->set_camera_pos(constants::CAMERA_ORIGIN);
	renderer->set_verbose(0);

	std::ofstream frame_csv;
	if (write_frame_csv) {
		const std::filesystem::path output(frame_csv_path);
		if (output.has_parent_path())
			std::filesystem::create_directories(output.parent_path());
		frame_csv.open(frame_csv_path);
		if (!frame_csv)
			throw std::runtime_error("could not open frame metrics output: " + frame_csv_path);
		frame_csv << "frame,model,n_workers,execution_time_s,virtual_time_ns,"
					 "stall_time_ns,context_switch_time_ns,stall_count,context_switches\n";
		frame_csv << std::setprecision(12);
	}

	BenchmarkMetrics metrics(config.model, config.workers);
	for (int run = 0; run < runs; ++run) {
		Timer timer;
		timer.start();
		renderer->render_frame();
		const double execution_time_s = timer.stopAndGetMilliseconds() / 1000.0;
		const BenchmarkMetrics::FrameSample sample{
			run + 1,
			execution_time_s,
			renderer->get_virtual_time_ns(),
			renderer->get_stall_time_ns(),
			renderer->get_context_switch_time_ns(),
			renderer->get_total_stalls(),
			renderer->get_context_switches()
		};
		metrics.wall.record_time(sample.execution_time_s);
		metrics.samples.push_back(sample);
		if (write_frame_csv) {
			frame_csv << sample.frame << ',' << config.name << ',' << config.workers << ','
					  << sample.execution_time_s << ',' << sample.virtual_time_ns << ','
					  << sample.stall_time_ns << ',' << sample.context_switch_time_ns << ','
					  << sample.stall_count << ','
					  << sample.context_switches << '\n';
		}
	}
	return metrics;
}

static std::string frame_csv_path(const std::string& summary_path, const char* model) {
	const std::filesystem::path summary(summary_path);
	return (summary.parent_path() /
		(summary.stem().string() + "_" + model + "_frames.csv")).string();
}

/**
 * @brief Escribe estadísticas agregadas de los modelos en un CSV.
 * @param path Ruta de salida; se crean sus directorios padre.
 * @param results Configuraciones y métricas que se exportan.
 * @throws std::runtime_error Si no se puede abrir el archivo.
 */
static void write_csv(const std::string& path,
					  const std::vector<std::pair<const ModelConfig*, BenchmarkMetrics>>& results,
					  const BenchmarkMetrics& sequential) {
	const std::filesystem::path output(path);
	if (output.has_parent_path())
		std::filesystem::create_directories(output.parent_path());
	std::ofstream file(path);
	if (!file)
		throw std::runtime_error("could not open metrics output: " + path);
	file << std::setprecision(12);

	const SummaryStats sequential_time = summarize(sequential.samples,
		[](const BenchmarkMetrics::FrameSample& sample) { return sample.execution_time_s; });
	const SummaryStats sequential_virtual = summarize(sequential.samples,
		[](const BenchmarkMetrics::FrameSample& sample) {
			return static_cast<double>(sample.virtual_time_ns);
		});

	file << "model,workload,n_workers,runs,mean_execution_time_s,stddev_execution_time_s,"
			"ci95_execution_lower_s,ci95_execution_upper_s,speedup,speedup_ci95_lower,"
			"speedup_ci95_upper,efficiency,mean_stall_time_ns,stddev_stall_time_ns,"
			"ci95_stall_time_lower_ns,ci95_stall_time_upper_ns,"
			"mean_context_switch_time_ns,stddev_context_switch_time_ns,"
			"ci95_context_switch_time_lower_ns,ci95_context_switch_time_upper_ns,"
			"mean_stall_count,"
			"stddev_stall_count,mean_context_switches,stddev_context_switches,"
			"mean_virtual_time_ns,virtual_speedup\n";
	for (const auto& result : results) {
		const ModelConfig& config = *result.first;
		const BenchmarkMetrics& benchmark = result.second;
		const metrics_interface& metrics = benchmark.wall;
		const SummaryStats execution = summarize(benchmark.samples,
			[](const BenchmarkMetrics::FrameSample& sample) { return sample.execution_time_s; });
		const SummaryStats stall_time = summarize(benchmark.samples,
			[](const BenchmarkMetrics::FrameSample& sample) {
				return static_cast<double>(sample.stall_time_ns);
			});
		const SummaryStats stall_count = summarize(benchmark.samples,
			[](const BenchmarkMetrics::FrameSample& sample) {
				return static_cast<double>(sample.stall_count);
			});
		const SummaryStats context_switch_time = summarize(benchmark.samples,
			[](const BenchmarkMetrics::FrameSample& sample) {
				return static_cast<double>(sample.context_switch_time_ns);
			});
		const SummaryStats context_switches = summarize(benchmark.samples,
			[](const BenchmarkMetrics::FrameSample& sample) {
				return static_cast<double>(sample.context_switches);
			});
		const double speedup = metrics.speedup();
		const double speedup_half_width = config.model == execution_model::sequential
			? 0.0 : speedup_ci_half_width(sequential_time, execution,
				sequential.samples.size(), benchmark.samples.size());
		const double virtual_mean = benchmark.mean_virtual_time_ns();
		const double virtual_speedup = sequential_virtual.mean > 0.0 && virtual_mean > 0.0
			? sequential_virtual.mean / virtual_mean : 0.0;
		file << config.name << ",raytracing,"
			 << metrics.get_n_workers() << ',' << metrics.get_run_count() << ','
			 << execution.mean << ',' << execution.stddev << ','
			 << execution.ci_lower << ',' << execution.ci_upper << ','
			 << speedup << ',' << speedup - speedup_half_width << ','
			 << speedup + speedup_half_width << ',' << metrics.efficiency() << ','
			 << stall_time.mean << ',' << stall_time.stddev << ','
			 << stall_time.ci_lower << ',' << stall_time.ci_upper << ','
			 << context_switch_time.mean << ',' << context_switch_time.stddev << ','
			 << context_switch_time.ci_lower << ',' << context_switch_time.ci_upper << ','
			 << stall_count.mean << ',' << stall_count.stddev << ','
			 << context_switches.mean << ',' << context_switches.stddev << ','
			 << virtual_mean << ',' << virtual_speedup << '\n';
	}
}

/**
 * @brief Analiza opciones CLI, mide modelos y opcionalmente genera el GIF orbital.
 * @param argc Cantidad de argumentos de la línea de comandos.
 * @param argv Argumentos recibidos por el proceso.
 * @return 0 si la campaña termina correctamente; 1 si ocurre un error.
 */
int main(int argc, char* argv[]) {
	try {
		int runs = 200;
		int requested_workers = 0;
		std::string selected_model = "all";
		std::string output = constants::RESULTS_DIR + "/benchmark.csv";
		bool generate_gif = false;
		bool write_frame_csv = false;

		for (int i = 1; i < argc; ++i) {
			const std::string argument = argv[i];
			if (argument == "--help" || argument == "-h") {
				std::cout << "Usage: raytracing_benchmark [--model all|sequential|fgmt|cgmt|smt|cmp] "
							 "[--runs N] [--workers N] [--output CSV] [--frame-csv] [--gif]\n"
							 "--workers configures FGMT/CGMT/CMP workers and SMT virtual contexts; "
							 "sequential remains one worker.\n";
				return 0;
			} else if (argument == "--model" && i + 1 < argc) {
				selected_model = argv[++i];
			} else if (argument == "--runs" && i + 1 < argc) {
				runs = parse_positive_integer(argv[++i], "runs");
			} else if (argument == "--workers" && i + 1 < argc) {
				requested_workers = parse_positive_integer(argv[++i], "workers");
				if (requested_workers > constants::IMAGE_WIDTH * constants::IMAGE_HEIGHT)
					throw std::invalid_argument("workers cannot exceed the number of pixels");
			} else if (argument == "--output" && i + 1 < argc) {
				output = argv[++i];
			} else if (argument == "--frame-csv") {
				write_frame_csv = true;
			} else if (argument == "--gif") {
				generate_gif = true;
			} else {
				throw std::invalid_argument("unknown or incomplete argument: " + argument);
			}
		}

		std::vector<ModelConfig> configured_models(
			std::begin(models), std::end(models));
		if (requested_workers > 0) {
			for (ModelConfig& config : configured_models)
				if (config.model != execution_model::sequential)
					config.workers = requested_workers;
		}

		std::vector<const ModelConfig*> selected;
		if (selected_model == "all") {
			for (const ModelConfig& model : configured_models)
				selected.push_back(&model);
		} else {
			selected.push_back(&find_model(configured_models, selected_model));
		}

		const ModelConfig& sequential = configured_models[0];
		const std::string sequential_csv = frame_csv_path(output, sequential.name);
		BenchmarkMetrics baseline = run_model(
			sequential, runs, sequential_csv, write_frame_csv);
		baseline.wall.set_sequential_time(baseline.wall.mean_time());
		const double sequential_virtual_time_ns = baseline.mean_virtual_time_ns();

		std::vector<std::pair<const ModelConfig*, BenchmarkMetrics>> results;
		for (const ModelConfig* config : selected) {
			BenchmarkMetrics metrics = baseline;
			if (config->model != execution_model::sequential) {
				const std::string model_csv = frame_csv_path(output, config->name);
				metrics = run_model(*config, runs, model_csv, write_frame_csv);
			}
			metrics.wall.set_sequential_time(baseline.wall.mean_time());
			results.emplace_back(config, metrics);
		}

		write_csv(output, results, baseline);
		std::cout << "Workload: raytracing | runs: " << runs << '\n';
		std::cout << std::left << std::setw(14) << "model" << std::right
				  << std::setw(14) << "wall (s)" << std::setw(12) << "wall-up"
				  << std::setw(14) << "efficiency" << std::setw(18) << "virtual (ns)"
				  << std::setw(14) << "virtual-up" << '\n';
		for (const auto& result : results) {
			const BenchmarkMetrics& benchmark = result.second;
			const metrics_interface& metrics = benchmark.wall;
			std::cout << std::left << std::setw(14) << result.first->name << std::right
					  << std::setw(14) << std::setprecision(6) << metrics.mean_time()
					  << std::setw(12) << metrics.speedup()
					  << std::setw(14) << metrics.efficiency()
					  << std::setw(18) << benchmark.mean_virtual_time_ns()
					  << std::setw(14) << benchmark.virtual_speedup(sequential_virtual_time_ns)
					  << '\n';
		}
		std::cout << "CSV: " << output << '\n';
		if (generate_gif) {
			render_camera_orbit_gif(requested_workers > 0
				? requested_workers : constants::CMP_NUM_CORES);
			std::cout << "GIF: " << constants::CAMERA_ORBIT_GIF_PATH << '\n';
		}
		if (write_frame_csv) {
			std::cout << "Frame CSV (sequential): "
					  << frame_csv_path(output, sequential.name) << '\n';
			for (const ModelConfig* config : selected) {
				if (config->model != execution_model::sequential)
					std::cout << "Frame CSV (" << config->name << "): "
							  << frame_csv_path(output, config->name) << '\n';
			}
		}
		return 0;
	} catch (const std::exception& error) {
		std::cerr << "Error: " << error.what() << '\n';
		return 1;
	}
}

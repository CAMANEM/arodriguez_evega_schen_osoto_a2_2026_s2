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
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
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

static std::vector<ModelConfig> make_models() {
	return {
		{"sequential", execution_model::sequential, 1},
		{"fine", execution_model::fine_grained, constants::NUM_THREADS},
		{"coarse", execution_model::coarse_grained, constants::NUM_THREADS},
		{"smt", execution_model::smt, constants::smt_context_count()},
		{"cmp", execution_model::cmp, constants::CMP_NUM_CORES}
	};
}

/**
 * @brief Normalizes CLI model aliases to campaign-canonical names.
 * @param name CLI model identifier (fine|fgmt|coarse|cgmt|...).
 * @return Canonical name used in CSV (fine|coarse|...).
 */
static std::string normalize_campaign_model(const std::string& name) {
	if (name == "fgmt") return "fine";
	if (name == "cgmt") return "coarse";
	return name;
}

/**
 * @brief Registry key expected by RendererFactory for a campaign model.
 * @param campaign_name Canonical campaign model name.
 * @return Factory key (fgmt for fine, cgmt for coarse).
 */
static std::string factory_model_name(const std::string& campaign_name) {
	if (campaign_name == "fine") return "fgmt";
	if (campaign_name == "coarse") return "cgmt";
	return campaign_name;
}

/**
 * @brief Busca la configuración de un modelo registrado.
 * @param name Nombre CLI del modelo (acepta aliases fgmt/cgmt).
 * @return Referencia a su configuración estática.
 * @throws std::invalid_argument Si el modelo no está registrado.
 */
static const ModelConfig& find_model(const std::vector<ModelConfig>& configured_models,
									 const std::string& name) {
	const std::string canonical = normalize_campaign_model(name);
	for (const ModelConfig& model : configured_models)
		if (canonical == model.name) return model;
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
 * @brief Renderiza warmup (descartado) y runs medidos de un modelo.
 * @param config Modelo y cantidad de workers.
 * @param warmup Discarded cold-start frames.
 * @param runs Measured repetitions recorded in summary stats.
 * @param frame_csv_path Optional raw/frame CSV path.
 * @param write_frame_csv Whether to emit per-frame rows with phase column.
 * @return Métricas de las muestras medidas, en segundos.
 */
static BenchmarkMetrics run_model(const ModelConfig& config, int warmup, int runs,
								  const std::string& frame_csv_path,
								  bool write_frame_csv) {
	std::unique_ptr<IRenderer> renderer =
		RendererFactory::create(factory_model_name(config.name), config.workers);
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
		frame_csv << "frame,model,phase,n_workers,execution_time_s,virtual_time_ns,"
					 "stall_time_ns,context_switch_time_ns,stall_count,context_switches\n";
		frame_csv << std::setprecision(12);
	}

	auto render_once = [&](int index, const char* phase) {
		Timer timer;
		timer.start();
		renderer->render_frame();
		const double execution_time_s = timer.stopAndGetMilliseconds() / 1000.0;
		const BenchmarkMetrics::FrameSample sample{
			index + 1,
			execution_time_s,
			renderer->get_virtual_time_ns(),
			renderer->get_stall_time_ns(),
			renderer->get_context_switch_time_ns(),
			renderer->get_total_stalls(),
			renderer->get_context_switches()
		};
		if (write_frame_csv) {
			frame_csv << sample.frame << ',' << config.name << ',' << phase << ','
					  << config.workers << ','
					  << sample.execution_time_s << ',' << sample.virtual_time_ns << ','
					  << sample.stall_time_ns << ',' << sample.context_switch_time_ns << ','
					  << sample.stall_count << ','
					  << sample.context_switches << '\n';
		}
		return sample;
	};

	BenchmarkMetrics metrics(config.model, config.workers);
	for (int run = 0; run < warmup; ++run)
		(void)render_once(run, "warmup");
	for (int run = 0; run < runs; ++run) {
		const BenchmarkMetrics::FrameSample sample =
			render_once(run, "measured");
		metrics.wall.record_time(sample.execution_time_s);
		metrics.samples.push_back(sample);
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
/**
 * @brief Writes campaign summary CSV (contract columns + ray-specific extras).
 * @param path Output path; parent directories are created.
 * @param results Model configs and measured metrics.
 * @param sequential Baseline metrics for speedup CI.
 * @param workload_n Pixel count used as workload_n.
 * @param warmup Discarded warmup count recorded in the summary.
 * @throws std::runtime_error If the file cannot be opened.
 */
static void write_csv(const std::string& path,
					  const std::vector<std::pair<const ModelConfig*, BenchmarkMetrics>>& results,
					  const BenchmarkMetrics& sequential,
					  int workload_n,
					  int warmup) {
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

	file << "model,workload_n,steps,effective_workers,quantum,smt_factor,seed,runs,warmup,"
			"mean_s,stddev_s,ci95_lo,ci95_hi,speedup,efficiency,"
			"speedup_ci95_lower,speedup_ci95_upper,"
			"mean_stall_time_ns,stddev_stall_time_ns,"
			"mean_context_switch_time_ns,mean_stall_count,mean_context_switches,"
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
		file << config.name << ',' << workload_n << ",1,"
			 << metrics.get_n_workers() << ',' << constants::PIXEL_QUANTUM_NS << ','
			 << constants::SMT_OVERSUBSCRIPTION_FACTOR << ','
			 << constants::CACHE_SEED_BASE << ',' << metrics.get_run_count() << ','
			 << warmup << ','
			 << execution.mean << ',' << execution.stddev << ','
			 << execution.ci_lower << ',' << execution.ci_upper << ','
			 << speedup << ',' << metrics.efficiency() << ','
			 << speedup - speedup_half_width << ','
			 << speedup + speedup_half_width << ','
			 << stall_time.mean << ',' << stall_time.stddev << ','
			 << context_switch_time.mean << ','
			 << stall_count.mean << ',' << context_switches.mean << ','
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
		int warmup = 0;
		int requested_workers = 0;
		int image_width = constants::IMAGE_WIDTH;
		int image_height = constants::IMAGE_HEIGHT;
		std::string selected_model = "all";
		std::string output = constants::RESULTS_DIR + "/benchmark.csv";
		bool generate_gif = false;
		bool write_frame_csv = false;

		for (int i = 1; i < argc; ++i) {
			const std::string argument = argv[i];
			if (argument == "--help" || argument == "-h") {
				std::cout << "Usage: raytracing_benchmark "
							 "[--model all|sequential|fine|fgmt|coarse|cgmt|smt|cmp] "
							 "[--runs N] [--warmup N] [--workers N|--threads N] "
							 "[--width N] [--height N] [--seed N] [--smt-factor N] "
							 "[--quantum N] [--output CSV] [--frame-csv] [--gif]\n"
							 "--workers/--threads configures Fine/Coarse/CMP workers and SMT "
							 "contexts; sequential remains one worker.\n"
							 "--warmup discards cold-start frames before measured runs.\n"
							 "--width/--height set image dimensions (default: 240 x 160).\n";
				return 0;
			} else if (argument == "--model" && i + 1 < argc) {
				selected_model = normalize_campaign_model(argv[++i]);
			} else if (argument == "--runs" && i + 1 < argc) {
				runs = parse_positive_integer(argv[++i], "runs");
			} else if (argument == "--warmup" && i + 1 < argc) {
				warmup = std::stoi(argv[++i]);
				if (warmup < 0)
					throw std::invalid_argument("warmup must be >= 0");
			} else if ((argument == "--workers" || argument == "--threads") &&
					   i + 1 < argc) {
				requested_workers = parse_positive_integer(argv[++i], "workers");
			} else if (argument == "--width" && i + 1 < argc) {
				image_width = parse_positive_integer(argv[++i], "width");
			} else if (argument == "--height" && i + 1 < argc) {
				image_height = parse_positive_integer(argv[++i], "height");
			} else if (argument == "--seed" && i + 1 < argc) {
				constants::set_cache_seed_base(
					static_cast<std::uint32_t>(parse_positive_integer(argv[++i], "seed")));
			} else if ((argument == "--smt-factor" || argument == "--oversubscribe") &&
					   i + 1 < argc) {
				constants::set_smt_oversubscription_factor(
					static_cast<unsigned int>(
						parse_positive_integer(argv[++i], "smt-factor")));
			} else if (argument == "--quantum" && i + 1 < argc) {
				constants::set_pixel_quantum_ns(
					parse_positive_integer(argv[++i], "quantum"));
			} else if (argument == "--output" && i + 1 < argc) {
				output = argv[++i];
			} else if (argument == "--frame-csv" || argument == "--raw-output") {
				write_frame_csv = true;
				if (argument == "--raw-output" && i + 1 < argc &&
					argv[i + 1][0] != '-') {
					// Optional path ignored; frame CSVs stay beside --output.
					++i;
				}
			} else if (argument == "--gif") {
				generate_gif = true;
			} else {
				throw std::invalid_argument("unknown or incomplete argument: " + argument);
			}
		}

		constants::set_image_dimensions(image_width, image_height);
		const int pixel_count = constants::IMAGE_WIDTH * constants::IMAGE_HEIGHT;
		if (requested_workers > pixel_count)
			throw std::invalid_argument("workers cannot exceed the number of pixels");

		std::vector<ModelConfig> configured_models = make_models();
		if (requested_workers > 0) {
			for (ModelConfig& config : configured_models)
				if (config.model != execution_model::sequential)
					config.workers = requested_workers;
		} else {
			for (ModelConfig& config : configured_models)
				if (config.model == execution_model::smt)
					config.workers = constants::smt_context_count();
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
			sequential, warmup, runs, sequential_csv, write_frame_csv);
		baseline.wall.set_sequential_time(baseline.wall.mean_time());
		const double sequential_virtual_time_ns = baseline.mean_virtual_time_ns();

		std::vector<std::pair<const ModelConfig*, BenchmarkMetrics>> results;
		for (const ModelConfig* config : selected) {
			BenchmarkMetrics metrics = baseline;
			if (config->model != execution_model::sequential) {
				const std::string model_csv = frame_csv_path(output, config->name);
				metrics = run_model(*config, warmup, runs, model_csv, write_frame_csv);
			}
			metrics.wall.set_sequential_time(baseline.wall.mean_time());
			results.emplace_back(config, metrics);
		}

		write_csv(output, results, baseline, pixel_count, warmup);
		std::cout << "Workload: raytracing | resolution: " << constants::IMAGE_WIDTH
				  << 'x' << constants::IMAGE_HEIGHT
				  << " | warmup: " << warmup << " | runs: " << runs << '\n';
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

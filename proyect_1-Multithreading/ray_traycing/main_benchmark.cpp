#include "raytracing_config.hpp"
#include "IRenderer.h"
#include "RendererFactory.h"
#include "metrics_interface.hpp"
#include "core/Timer.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

struct ModelConfig {
	const char* name;
	execution_model model;
	int workers;
};

static const ModelConfig models[] = {
	{"sequential", execution_model::sequential, 1},
	{"fgmt", execution_model::fine_grained, constants::NUM_THREADS},
	{"cgmt", execution_model::coarse_grained, constants::NUM_THREADS},
	{"smt", execution_model::smt, constants::smt_context_count()},
	{"cmp", execution_model::cmp, constants::CMP_NUM_CORES}
};

static const ModelConfig& find_model(const std::string& name) {
	for (const ModelConfig& model : models)
		if (name == model.name) return model;
	throw std::invalid_argument("unknown model: " + name);
}

static std::string workload_name(Workload workload) {
	return workload == Workload::dummy ? "dummy" : "raytracing";
}

static metrics_interface run_model(const ModelConfig& config, int runs,
								   Workload workload, std::vector<Vector3>* last_frame) {
	metrics_interface metrics(config.model, config.workers);
	std::unique_ptr<IRenderer> renderer = RendererFactory::create(config.name);
	renderer->set_workload(workload);

	for (int run = 0; run < runs; ++run) {
		Timer timer;
		timer.start();
		std::vector<Vector3> frame = renderer->render_frame();
		metrics.record_time(timer.stopAndGetMilliseconds() / 1000.0);
		if (last_frame != nullptr)
			*last_frame = std::move(frame);
	}
	return metrics;
}

static void write_csv(const std::string& path,
					  const std::vector<std::pair<const ModelConfig*, metrics_interface>>& results,
					  Workload workload) {
	const std::filesystem::path output(path);
	if (output.has_parent_path())
		std::filesystem::create_directories(output.parent_path());
	std::ofstream file(path);
	if (!file)
		throw std::runtime_error("could not open metrics output: " + path);

	file << "model,workload,n_workers,runs,mean_time_s,stddev_s,ci95_lower_s,"
			"ci95_upper_s,speedup,efficiency\n";
	for (const auto& result : results) {
		const ModelConfig& config = *result.first;
		const metrics_interface& metrics = result.second;
		file << config.name << ',' << workload_name(workload) << ','
			 << metrics.get_n_workers() << ',' << metrics.get_run_count() << ','
			 << metrics.mean_time() << ',' << metrics.stddev_time() << ','
			 << metrics.ci95_lower() << ',' << metrics.ci95_upper() << ','
			 << metrics.speedup() << ',' << metrics.efficiency() << '\n';
	}
}

int main(int argc, char* argv[]) {
	try {
		int runs = 5;
		std::string selected_model = "all";
		std::string output = "results/benchmark.csv";
		Workload workload = Workload::raytracing;

		for (int i = 1; i < argc; ++i) {
			const std::string argument = argv[i];
			if (argument == "--help" || argument == "-h") {
				std::cout << "Usage: raytracing_benchmark [--model all|sequential|fgmt|cgmt|smt|cmp] "
							 "[--workload raytracing|dummy] [--runs N] [--output CSV]\n";
				return 0;
			} else if (argument == "--model" && i + 1 < argc) {
				selected_model = argv[++i];
			} else if (argument == "--workload" && i + 1 < argc) {
				const std::string value = argv[++i];
				if (value == "dummy") workload = Workload::dummy;
				else if (value != "raytracing")
					throw std::invalid_argument("workload must be raytracing or dummy");
			} else if (argument == "--runs" && i + 1 < argc) {
				runs = std::stoi(argv[++i]);
				if (runs < 1) throw std::invalid_argument("runs must be positive");
			} else if (argument == "--output" && i + 1 < argc) {
				output = argv[++i];
			} else {
				throw std::invalid_argument("unknown or incomplete argument: " + argument);
			}
		}

		std::vector<const ModelConfig*> selected;
		if (selected_model == "all") {
			for (const ModelConfig& model : models) selected.push_back(&model);
		} else {
			selected.push_back(&find_model(selected_model));
		}

		const ModelConfig& sequential = models[0];
		metrics_interface baseline = run_model(sequential, runs, workload, nullptr);
		baseline.set_sequential_time(baseline.mean_time());

		std::vector<std::pair<const ModelConfig*, metrics_interface>> results;
		for (const ModelConfig* config : selected) {
			metrics_interface metrics = config->model == execution_model::sequential
				? baseline : run_model(*config, runs, workload, nullptr);
			metrics.set_sequential_time(baseline.mean_time());
			results.emplace_back(config, metrics);
		}

		write_csv(output, results, workload);
		std::cout << "Workload: " << workload_name(workload) << " | runs: " << runs << '\n';
		std::cout << std::left << std::setw(14) << "model" << std::right
				  << std::setw(14) << "mean (s)" << std::setw(12) << "speedup"
				  << std::setw(14) << "efficiency" << '\n';
		for (const auto& result : results) {
			const metrics_interface& metrics = result.second;
			std::cout << std::left << std::setw(14) << result.first->name << std::right
					  << std::setw(14) << std::setprecision(6) << metrics.mean_time()
					  << std::setw(12) << metrics.speedup()
					  << std::setw(14) << metrics.efficiency() << '\n';
		}
		std::cout << "CSV: " << output << '\n';
		return 0;
	} catch (const std::exception& error) {
		std::cerr << "Error: " << error.what() << '\n';
		return 1;
	}
}

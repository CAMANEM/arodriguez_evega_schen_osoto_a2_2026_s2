#include "IRenderer.h"
#include "RendererFactory.h"
#include "image_io.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

int main(int argc, char* argv[]) {
	try {
		std::string model = "sequential";
		std::string output = "results/frame.ppm";
		Workload workload = Workload::raytracing;

		for (int i = 1; i < argc; ++i) {
			const std::string argument = argv[i];
			if (argument == "--help" || argument == "-h") {
				std::cout << "Usage: raytracing_visual [--model sequential|fgmt|cgmt|smt|cmp] "
							 "[--workload raytracing|dummy] [--output IMAGE.ppm]\n";
				return 0;
			} else if (argument == "--model" && i + 1 < argc) {
				model = argv[++i];
			} else if (argument == "--workload" && i + 1 < argc) {
				const std::string value = argv[++i];
				if (value == "dummy") workload = Workload::dummy;
				else if (value != "raytracing")
					throw std::invalid_argument("workload must be raytracing or dummy");
			} else if (argument == "--output" && i + 1 < argc) {
				output = argv[++i];
			} else {
				throw std::invalid_argument("unknown or incomplete argument: " + argument);
			}
		}

		std::unique_ptr<IRenderer> renderer = RendererFactory::create(model);
		renderer->set_workload(workload);
		write_ppm(output, renderer->render_frame());
		std::cout << "Image: " << output << '\n';
		return 0;
	} catch (const std::exception& error) {
		std::cerr << "Error: " << error.what() << '\n';
		return 1;
	}
}

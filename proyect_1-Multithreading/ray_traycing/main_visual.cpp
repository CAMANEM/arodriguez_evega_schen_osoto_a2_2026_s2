/**
 * @file main_visual.cpp
 * @brief Renderiza modelos a PPM y genera la animación orbital de cámara.
 */
#include "raytracing_config.hpp"
#include "IRenderer.h"
#include "RendererFactory.h"
#include "core/CameraOrbit.h"
#include "image_io.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

/** @brief Identificadores aceptados por la opción --model. */
static const char* model_names[] = {"sequential", "fgmt", "cgmt", "smt", "cmp"};

/**
 * @brief Inserta el nombre del modelo antes de la extensión de salida.
 * @param output Ruta base solicitada por el usuario.
 * @param model Identificador del renderer.
 * @return Ruta derivada, por ejemplo frame_smt.ppm.
 */
static std::string output_for_model(const std::string& output, const std::string& model) {
	const std::filesystem::path path(output);
	return (path.parent_path() /
		(path.stem().string() + "_" + model + path.extension().string())).string();
}

/**
 * @brief Renderiza uno o todos los modelos y exporta imágenes PPM.
 * @param argc Cantidad de argumentos CLI.
 * @param argv Argumentos de selección, carga y salida.
 * @return 0 si el render y las exportaciones terminan correctamente; 1 ante error.
 */
int main(int argc, char* argv[]) {
	try {
		std::string model = "all";
		std::string output = constants::RESULTS_DIR + "/frame.ppm";
		Workload workload = Workload::raytracing;
		bool generate_gif = true;

		for (int i = 1; i < argc; ++i) {
			const std::string argument = argv[i];
			if (argument == "--help" || argument == "-h") {
				std::cout << "Usage: raytracing_visual [--model all|sequential|fgmt|cgmt|smt|cmp] "
							 "[--workload raytracing|dummy] [--output FRAME.ppm] [--no-gif]\n";
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
			} else if (argument == "--no-gif") {
				generate_gif = false;
			} else {
				throw std::invalid_argument("unknown or incomplete argument: " + argument);
			}
		}

		std::vector<std::string> selected_models;
		if (model == "all") {
			for (const char* model_name : model_names)
				selected_models.emplace_back(model_name);
		} else {
			bool known_model = false;
			for (const char* model_name : model_names) {
				if (model == model_name) {
					known_model = true;
					break;
				}
			}
			if (!known_model)
				throw std::invalid_argument("unknown model: " + model);
			selected_models.push_back(model);
		}

		for (const std::string& selected_model : selected_models) {
			std::unique_ptr<IRenderer> renderer = RendererFactory::create(selected_model);
			renderer->set_workload(workload);
			renderer->set_camera_pos(constants::CAMERA_ORIGIN);
			const std::string frame_path = model == "all"
				? output_for_model(output, selected_model) : output;
			write_ppm(frame_path, renderer->render_frame());
			std::cout << selected_model << " image: " << frame_path << '\n';
		}

		if (generate_gif) {
			render_camera_orbit_gif();
			std::cout << "Camera orbit GIF: " << constants::CAMERA_ORBIT_GIF_PATH << '\n';
		}
		return 0;
	} catch (const std::exception& error) {
		std::cerr << "Error: " << error.what() << '\n';
		return 1;
	}
}

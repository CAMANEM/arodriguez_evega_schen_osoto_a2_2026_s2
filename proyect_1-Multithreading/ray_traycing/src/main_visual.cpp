/**
 * @file main_visual.cpp
 * @brief Renderiza modelos a PPM y genera la animación orbital de cámara.
 */
#include "core/config/raytracing_config.hpp"
#include "core/strategies/IRenderer.h"
#include "core/strategies/RendererFactory.h"
#include "core/camera/CameraOrbit.h"
#include "core/utils/image_io.hpp"
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

static int parse_positive_integer(const std::string& value, const char* option) {
	std::size_t parsed = 0;
	const int result = std::stoi(value, &parsed);
	if (parsed != value.size() || result < 1)
		throw std::invalid_argument(std::string(option) + " must be a positive integer");
	if (result > constants::IMAGE_WIDTH * constants::IMAGE_HEIGHT)
		throw std::invalid_argument("workers cannot exceed the number of pixels");
	return result;
}

/**
 * @brief Renderiza uno o todos los modelos y exporta imágenes PPM.
 * @param argc Cantidad de argumentos CLI.
 * @param argv Argumentos de selección de modelo y salida.
 * @return 0 si el render y las exportaciones terminan correctamente; 1 ante error.
 */
int main(int argc, char* argv[]) {
	try {
		std::string model = "all";
		std::string output = constants::RESULTS_DIR + "/frame.ppm";
		int workers = 0;
		bool generate_gif = false;

		for (int i = 1; i < argc; ++i) {
			const std::string argument = argv[i];
			if (argument == "--help" || argument == "-h") {
				std::cout << "Usage: raytracing_visual [--model all|sequential|fgmt|cgmt|smt|cmp] "
							 "[--workers N] [--output FRAME.ppm] [--gif]\n"
							 "--workers configures FGMT/CGMT/CMP workers and SMT virtual contexts; "
							 "sequential remains one worker.\n";
				return 0;
			} else if (argument == "--model" && i + 1 < argc) {
				model = argv[++i];
			} else if (argument == "--output" && i + 1 < argc) {
				output = argv[++i];
			} else if (argument == "--workers" && i + 1 < argc) {
				workers = parse_positive_integer(argv[++i], "workers");
			} else if (argument == "--gif") {
				generate_gif = true;
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
			std::unique_ptr<IRenderer> renderer = workers > 0
				? RendererFactory::create(selected_model, workers)
				: RendererFactory::create(selected_model);
			renderer->set_camera_pos(constants::CAMERA_ORIGIN);
			const std::string frame_path = model == "all"
				? output_for_model(output, selected_model) : output;
			write_ppm(frame_path, renderer->render_frame());
			std::cout << selected_model << " image: " << frame_path << '\n';
		}

		if (generate_gif) {
			render_camera_orbit_gif(workers > 0 ? workers : constants::CMP_NUM_CORES);
			std::cout << "Camera orbit GIF: " << constants::CAMERA_ORBIT_GIF_PATH << '\n';
		}
		return 0;
	} catch (const std::exception& error) {
		std::cerr << "Error: " << error.what() << '\n';
		return 1;
	}
}

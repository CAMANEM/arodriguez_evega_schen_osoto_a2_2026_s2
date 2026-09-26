/**
 * @file CameraOrbit.cpp
 * @brief Renderiza frames alrededor de la escena y genera el GIF animado.
 */
#include "CameraOrbit.h"

#include "raytracing_config.hpp"
#include "RendererFactory.h"
#include "image_io.hpp"
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

/**
 * @brief Renderiza la órbita horaria con CMP y delega el GIF a Pillow.
 * @throws std::runtime_error Si falla el proceso Python de ensamblado.
 */
void render_camera_orbit_gif() {
    const std::filesystem::path frames_directory = constants::CAMERA_ORBIT_FRAMES_DIR;
    std::filesystem::create_directories(frames_directory);
    std::unique_ptr<IRenderer> renderer = RendererFactory::create("cmp");
    renderer->set_workload(Workload::raytracing);

    const double full_rotation = 2.0 * std::acos(-1.0);
    for (int frame_index = 0; frame_index < constants::CAMERA_ORBIT_FRAME_COUNT; ++frame_index) {
        const double angle = full_rotation / 4.0 -
            full_rotation * frame_index / constants::CAMERA_ORBIT_FRAME_COUNT;
        const Vector3 camera_position(
            constants::SCENE_CENTER_X + constants::CAMERA_ORBIT_RADIUS * std::cos(angle),
            constants::CAMERA_ORBIT_Y,
            constants::SCENE_CENTER_Z + constants::CAMERA_ORBIT_RADIUS * std::sin(angle));
        renderer->set_camera_pos(camera_position);

        std::ostringstream filename;
        filename << "frame_" << std::setw(3) << std::setfill('0') << frame_index << ".ppm";
        write_ppm((frames_directory / filename.str()).string(), renderer->render_frame());
    }

#ifdef _WIN32
    const std::string python = "python";
#else
    const std::string python = "python3";
#endif
    const std::string command = python + " \"" + constants::CAMERA_ORBIT_GIF_SCRIPT + "\" \"" +
        constants::CAMERA_ORBIT_FRAMES_DIR + "\" \"" + constants::CAMERA_ORBIT_GIF_PATH + "\" " +
        std::to_string(constants::CAMERA_ORBIT_FRAME_DELAY_MS);
    if (std::system(command.c_str()) != 0)
        throw std::runtime_error("could not create camera GIF; install Python 3 and Pillow");
}
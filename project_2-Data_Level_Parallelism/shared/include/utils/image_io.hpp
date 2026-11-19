#pragma once
#include "config/raytracing_config.hpp"
#include "geometry/Vector3.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @brief Escribe un frame RGB como archivo PPM texto (formato P3).
 * @param path Ruta destino; crea directorios padre si hacen falta.
 * @param pixels Buffer row-major de IMAGE_WIDTH × IMAGE_HEIGHT colores en [0,1].
 * @throws std::invalid_argument Si el buffer no tiene el tamaño configurado.
 * @throws std::runtime_error Si no se puede crear el archivo.
 */
inline void write_ppm(const std::string& path, const std::vector<Vector3>& pixels) {
    const std::size_t expected =
        static_cast<std::size_t>(constants::IMAGE_WIDTH) * constants::IMAGE_HEIGHT;
    if (pixels.size() != expected)
        throw std::invalid_argument("pixel buffer size does not match IMAGE_WIDTH * IMAGE_HEIGHT");

    const std::filesystem::path out(path);
    if (out.has_parent_path())
        std::filesystem::create_directories(out.parent_path());

    std::ofstream file(path);
    if (!file)
        throw std::runtime_error("could not open image output: " + path);

    file << "P3\n" << constants::IMAGE_WIDTH << " " << constants::IMAGE_HEIGHT << "\n255\n";
    for (const Vector3& c : pixels) {
        file << static_cast<int>(std::clamp(c.x * 255.0, 0.0, 255.0)) << " "
             << static_cast<int>(std::clamp(c.y * 255.0, 0.0, 255.0)) << " "
             << static_cast<int>(std::clamp(c.z * 255.0, 0.0, 255.0)) << "\n";
    }
}

/**
 * @file image_io.hpp
 * @brief Escritura de buffers de píxeles en formato Netpbm PPM P3.
 */
#pragma once

#include "raytracing_config.hpp"
#include "Vector3.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @brief Escribe un frame RGB como archivo PPM de texto.
 * @param path Ruta destino; crea los directorios padre si hacen falta.
 * @param pixels Buffer row-major de IMAGE_WIDTH × IMAGE_HEIGHT colores.
 * @throws std::invalid_argument Si el buffer no tiene el tamaño configurado.
 * @throws std::runtime_error Si no se puede abrir el archivo de salida.
 */
inline void write_ppm(const std::string& path, const std::vector<Vector3>& pixels) {
    const std::size_t expected = static_cast<std::size_t>(constants::IMAGE_WIDTH) *
                                 constants::IMAGE_HEIGHT;
    if (pixels.size() != expected)
        throw std::invalid_argument("pixel matrix has an unexpected size");

    const std::filesystem::path output(path);
    if (output.has_parent_path())
        std::filesystem::create_directories(output.parent_path());

    std::ofstream file(path);
    if (!file)
        throw std::runtime_error("could not open image output: " + path);

    file << "P3\n" << constants::IMAGE_WIDTH << " " << constants::IMAGE_HEIGHT
         << "\n255\n";
    for (const Vector3& color : pixels) {
        const int red = static_cast<int>(std::clamp(color.x * 255.0, 0.0, 255.0));
        const int green = static_cast<int>(std::clamp(color.y * 255.0, 0.0, 255.0));
        const int blue = static_cast<int>(std::clamp(color.z * 255.0, 0.0, 255.0));
        file << red << " " << green << " " << blue << "\n";
    }
}
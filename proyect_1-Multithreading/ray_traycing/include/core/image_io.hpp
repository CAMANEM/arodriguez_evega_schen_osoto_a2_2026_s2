#pragma once

#include "raytracing_config.hpp"
#include "Vector3.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

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
#pragma once
#include "geometry/Vector3.h"
#include <array>
#include <limits>
#include <stdexcept>
#include <string>

namespace constants {

/** @brief Parámetros de una esfera de la escena. */
struct SphereConfig {
    Vector3 center;
    double  radius;
    Vector3 color;
};

/** @brief Resolución horizontal del frame, en píxeles. Mutable vía set_image_dimensions(). */
inline int IMAGE_WIDTH  = 240;
/** @brief Resolución vertical del frame, en píxeles. Mutable vía set_image_dimensions(). */
inline int IMAGE_HEIGHT = 160;

/**
 * @brief Ajusta la resolución activa (N rayos = width * height).
 * @throws std::invalid_argument Si las dimensiones no son positivas o el producto desborda int.
 */
inline void set_image_dimensions(int width, int height) {
    if (width < 1 || height < 1 ||
        static_cast<long long>(width) * height > std::numeric_limits<int>::max())
        throw std::invalid_argument(
            "image dimensions must be positive and their product must fit in an int");
    IMAGE_WIDTH  = width;
    IMAGE_HEIGHT = height;
}

/** @brief Escena por defecto: esfera roja, verde y azul sobre el eje X. */
inline const std::array<SphereConfig, 3> SPHERES = {{
    {{-3.0, 0.0, 0.0}, 0.85, {1.0, 0.0, 0.0}},
    {{ 0.0, 0.0, 0.0}, 1.35, {0.0, 1.0, 0.0}},
    {{ 3.0, 0.0, 0.0}, 0.65, {0.0, 0.0, 1.0}}
}};

inline constexpr double SCENE_CENTER_X = 0.0;
inline constexpr double SCENE_CENTER_Y = 0.0;
inline constexpr double SCENE_CENTER_Z = 0.0;

inline const Vector3 CAMERA_ORIGIN     = {0.0, 0.0, 8.0};
inline const Vector3 CAMERA_UP         = {0.0, 1.0, 0.0};
inline const Vector3 BACKGROUND_COLOR  = {1.0, 1.0, 1.0};

inline constexpr double CAMERA_ORBIT_RADIUS      = 8.0;
inline constexpr double CAMERA_ORBIT_Y           = 0.0;
inline constexpr int    CAMERA_ORBIT_FRAME_COUNT = 72;
inline constexpr int    CAMERA_ORBIT_FRAME_DELAY_MS = 50;

inline const std::string RESULTS_DIR                = "results";
inline const std::string IMAGE_FILE_SEQUENTIAL      = "results/frame_sequential.ppm";
inline const std::string CSV_FILE_SEQUENTIAL        = "results/benchmark.csv";

} // namespace constants

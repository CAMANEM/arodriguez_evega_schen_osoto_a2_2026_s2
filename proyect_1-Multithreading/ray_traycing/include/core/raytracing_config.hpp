#ifndef RAYTRACING_CONFIG_HPP
#define RAYTRACING_CONFIG_HPP

#include "Vector3.h"
#include <algorithm>
#include <array>
#include <string>
#include <thread>

namespace constants {

struct SphereConfig {
    Vector3 center;
    double radius;
    Vector3 color;
};

inline constexpr int IMAGE_WIDTH = 80;
inline constexpr int IMAGE_HEIGHT = 60;
inline constexpr int NUM_FRAMES = 200;

inline const std::array<SphereConfig, 3> SPHERES = {{
    {{0.0, 0.0, -5.0}, 1.2, {1.0, 0.0, 0.0}},
    {{2.0, 1.0, -5.0}, 1.0, {0.0, 1.0, 0.0}},
    {{-2.0, -1.0, -5.0}, 1.0, {0.0, 0.0, 1.0}}
}};

inline constexpr double SCENE_CENTER_X = 0.0;
inline constexpr double SCENE_CENTER_Y = 0.0;
inline constexpr double SCENE_CENTER_Z = -5.0;
inline const Vector3 CAMERA_ORIGIN = {0.0, 0.0, 0.0};
inline const Vector3 CAMERA_UP = {0.0, 1.0, 0.0};
inline const Vector3 BACKGROUND_COLOR = {0.0, 0.0, 0.0};

inline constexpr double CAMERA_ORBIT_RX = 8.0;
inline constexpr double CAMERA_ORBIT_RZ = 6.0;
inline constexpr double CAMERA_ORBIT_Y = 0.0;

inline const std::string GIF_FRAMES_DIR = "tests/gif_utils";
inline const std::string RESULTS_DIR = "results";
inline const std::string IMAGE_DIR = "results/image";
inline const std::string GRAPHS_DIR = "results/graficas";
inline const std::string IMAGE_FILE_SEQUENTIAL = "results/image/frame_secuencial.ppm";
inline const std::string IMAGE_FILE_FGMT = "results/image/frame_fgmt.ppm";
inline const std::string IMAGE_FILE_CGMT = "results/image/frame_cgmt.ppm";
inline const std::string IMAGE_FILE_SMT = "results/image/frame_smt.ppm";
inline const std::string IMAGE_FILE_CMP = "results/image/frame_cmp.ppm";
inline const std::string CSV_FILE_SEQUENTIAL = "results/mediciones_secuencial.csv";
inline const std::string CSV_FILE_FGMT = "results/mediciones_fgmt.csv";
inline const std::string CSV_FILE_CGMT = "results/mediciones_cgmt.csv";
inline const std::string CSV_FILE_SMT = "results/mediciones_smt.csv";
inline const std::string CSV_FILE_CMP = "results/mediciones_cmp.csv";

inline constexpr int NUM_THREADS = 4;
inline constexpr int CACHE_SIZE = 256;
inline constexpr int NOP_PENALTY_NS = 100;
inline constexpr int NOPS_PER_STALL = 32;
inline constexpr long long PIXEL_QUANTUM_NS = 1000LL;
inline constexpr long long CACHE_MISS_PENALTY_NS =
    static_cast<long long>(NOPS_PER_STALL) * NOP_PENALTY_NS;
inline constexpr long long CONTEXT_SWITCH_COST_NS = 400LL;

inline constexpr unsigned int SMT_OVERSUBSCRIPTION_FACTOR = 2;
inline constexpr int SMT_ISSUE_WIDTH = 2;
inline constexpr int CMP_NUM_CORES = 4;

inline int smt_context_count() {
    const unsigned int logical_cores = std::max(1u, std::thread::hardware_concurrency());
    const unsigned int max_contexts = static_cast<unsigned int>(IMAGE_WIDTH * IMAGE_HEIGHT);
    return static_cast<int>(std::min(
        logical_cores * SMT_OVERSUBSCRIPTION_FACTOR, max_contexts));
}

} // namespace constants

#endif // RAYTRACING_CONFIG_HPP
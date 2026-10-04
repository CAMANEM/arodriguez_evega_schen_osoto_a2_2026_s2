/**
 * @file raytracing_config.hpp
 * @brief Configuración compartida de escena, cámara y modelos de ejecución.
 */
#ifndef RAYTRACING_CONFIG_HPP
#define RAYTRACING_CONFIG_HPP

#include "Vector3.h"
#include <algorithm>
#include <array>
#include <string>
#include <thread>

namespace constants {

/** @brief Parámetros geométricos y visuales de una esfera de la escena. */
struct SphereConfig {
    Vector3 center;
    double radius;
    Vector3 color;
};

/** @brief Resolución horizontal del frame, en píxeles. */
inline constexpr int IMAGE_WIDTH = 240;
/** @brief Resolución vertical del frame, en píxeles. */
inline constexpr int IMAGE_HEIGHT = 160;
/** @brief Cantidad de frames de animación usada por clientes que la requieran. */
inline constexpr int NUM_FRAMES = 200;

/**
 * @brief Tres esferas sobre X: roja a la izquierda, verde al centro y azul a la derecha.
 * @details La esfera verde está centrada en SCENE_CENTER y cada esfera tiene radio distinto.
 */
inline const std::array<SphereConfig, 3> SPHERES = {{
    {{-3.0, 0.0, 0.0}, 0.85, {1.0, 0.0, 0.0}},
    {{0.0, 0.0, 0.0}, 1.35, {0.0, 1.0, 0.0}},
    {{3.0, 0.0, 0.0}, 0.65, {0.0, 0.0, 1.0}}
}};

/** @brief Coordenadas del punto al que mira la cámara orbital. */
inline constexpr double SCENE_CENTER_X = 0.0;
inline constexpr double SCENE_CENTER_Y = 0.0;
inline constexpr double SCENE_CENTER_Z = 0.0;
/** @brief Posición inicial frente a la escena para un frame estático. */
inline const Vector3 CAMERA_ORIGIN = {0.0, 0.0, 8.0};
/** @brief Vector vertical de referencia usado por la proyección look-at. */
inline const Vector3 CAMERA_UP = {0.0, 1.0, 0.0};
/** @brief Color RGB del fondo para los rayos que no intersectan la escena. */
inline const Vector3 BACKGROUND_COLOR = {1.0, 1.0, 1.0};

/** @brief Radio constante de la órbita circular de cámara, en unidades de escena. */
inline constexpr double CAMERA_ORBIT_RADIUS = 8.0;
/** @brief Altura fija de la cámara mientras orbita en el plano XZ. */
inline constexpr double CAMERA_ORBIT_Y = 0.0;
/** @brief Número de imágenes que forman la animación de órbita. */
inline constexpr int CAMERA_ORBIT_FRAME_COUNT = 72;
/** @brief Duración de cada frame del GIF, en milisegundos. */
inline constexpr int CAMERA_ORBIT_FRAME_DELAY_MS = 50;

/** @brief Rutas de resultados y recursos, relativas al directorio de ejecución. */
inline const std::string GIF_FRAMES_DIR = "data/gif_frames";
inline const std::string CAMERA_ORBIT_FRAMES_DIR = "data/camera_orbit_frames";
inline const std::string CAMERA_ORBIT_GIF_PATH = "data/camera_orbit.gif";
inline const std::string CAMERA_ORBIT_GIF_SCRIPT = "scripts/create_camera_gif.py";
inline const std::string RESULTS_DIR = "data";
inline const std::string IMAGE_DIR = "data/image";
inline const std::string GRAPHS_DIR = "data/graficas";
inline const std::string IMAGE_FILE_SEQUENTIAL = "data/image/frame_secuencial.ppm";
inline const std::string IMAGE_FILE_FGMT = "data/image/frame_fgmt.ppm";
inline const std::string IMAGE_FILE_CGMT = "data/image/frame_cgmt.ppm";
inline const std::string IMAGE_FILE_SMT = "data/image/frame_smt.ppm";
inline const std::string IMAGE_FILE_CMP = "data/image/frame_cmp.ppm";
inline const std::string CSV_FILE_SEQUENTIAL = "data/mediciones_secuencial.csv";
inline const std::string CSV_FILE_FGMT = "data/mediciones_fgmt.csv";
inline const std::string CSV_FILE_CGMT = "data/mediciones_cgmt.csv";
inline const std::string CSV_FILE_SMT = "data/mediciones_smt.csv";
inline const std::string CSV_FILE_CMP = "data/mediciones_cmp.csv";

/** @brief Cantidad fija de contextos de los modelos FGMT y CGMT. */
inline constexpr int NUM_THREADS = 4;
/** @brief Tamaño de caché simulado por contexto, en bytes. */
inline constexpr int CACHE_SIZE = 256;
/** @brief Penalización de un NOP simulado, en nanosegundos. */
inline constexpr int NOP_PENALTY_NS = 100;
/** @brief Cantidad de NOP que representa un miss de caché. */
inline constexpr int NOPS_PER_STALL = 32;
/** @brief Quantum de ejecución por píxel, en nanosegundos virtuales. */
inline constexpr long long PIXEL_QUANTUM_NS = 1000LL;
/** @brief Latencia total de un miss sin ocultamiento. */
inline constexpr long long CACHE_MISS_PENALTY_NS =
    static_cast<long long>(NOPS_PER_STALL) * NOP_PENALTY_NS;
inline constexpr long long CONTEXT_SWITCH_COST_NS = 400LL;

/** @brief Multiplicador de los procesadores lógicos para contextos SMT virtuales. */
inline constexpr unsigned int SMT_OVERSUBSCRIPTION_FACTOR = 2;
/** @brief Slots que puede emitir el simulador SMT en un ciclo. */
inline constexpr int SMT_ISSUE_WIDTH = 2;
/** @brief Cantidad de workers reales usada por el modelo CMP. */
inline constexpr int CMP_NUM_CORES = 4;

/**
 * @brief Calcula los contextos SMT virtuales según el hardware disponible.
 * @return Procesadores lógicos por factor de sobre-suscripción, limitado al número de píxeles.
 */
inline int smt_context_count() {
    const unsigned int logical_cores = std::max(1u, std::thread::hardware_concurrency());
    const unsigned int max_contexts = static_cast<unsigned int>(IMAGE_WIDTH * IMAGE_HEIGHT);
    return static_cast<int>(std::min(
        logical_cores * SMT_OVERSUBSCRIPTION_FACTOR, max_contexts));
}

} // namespace constants

#endif // RAYTRACING_CONFIG_HPP
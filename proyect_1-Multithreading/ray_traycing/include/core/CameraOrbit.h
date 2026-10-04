/**
 * @file CameraOrbit.h
 * @brief Interfaz para renderizar y ensamblar la animación orbital de cámara.
 */
#ifndef CAMERA_ORBIT_H
#define CAMERA_ORBIT_H

/**
 * @brief Renderiza frames CMP con cámara orbital y los ensambla en un GIF.
 * @throws std::runtime_error Si Python o Pillow no pueden crear el archivo.
 */
void render_camera_orbit_gif();

#endif // CAMERA_ORBIT_H
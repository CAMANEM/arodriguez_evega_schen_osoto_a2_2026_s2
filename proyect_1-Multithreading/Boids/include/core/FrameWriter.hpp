#ifndef FRAME_WRITER_HPP
#define FRAME_WRITER_HPP

#include <string>

#include "core/Flock.hpp"

/**
 * @brief Exporta el estado del enjambre como una imagen PPM a color, usada
 *        como evidencia visual funcional durante la demostración.
 *
 * Cada boid se dibuja como un pequeño punto de color con un breve segmento
 * en la dirección de su velocidad, sobre un fondo oscuro. Se optó por PPM
 * (igual que en la implementación de Mandelbrot) por no requerir ninguna
 * librería externa de imágenes.
 *
 * 
 */
class FrameWriter {
public:
    /**
     * @brief Dibuja el enjambre completo y escribe el resultado a un
     *        archivo .ppm. Se asume que las coordenadas del mundo
     *        coinciden 1:1 con los píxeles del lienzo (mismo ancho/alto),
     *        para evitar lógica adicional de escalado.
     * @param flock Enjambre a dibujar.
     * @param canvasWidth Ancho del lienzo en píxeles.
     * @param canvasHeight Alto del lienzo en píxeles.
     * @param filepath Ruta del archivo .ppm de salida.
     * @return true si el archivo se escribió correctamente.
     */
    static bool writeFrame(const Flock& flock, int canvasWidth, int canvasHeight,
                            const std::string& filepath);
};

#endif // FRAME_WRITER_HPP

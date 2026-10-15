#include "core/FrameWriter.hpp"

#include <algorithm>
#include <fstream>
#include <vector>

namespace {

struct Color {
    unsigned char r, g, b;
};

/**
 * @brief Pinta un píxel del lienzo si las coordenadas caen dentro de los
 *        límites, evitando escribir fuera del arreglo.
 */
void plotPixel(std::vector<unsigned char>& canvas, int width, int height, int x, int y,
               const Color& color) {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return;
    }
    const size_t offset = (static_cast<size_t>(y) * width + x) * 3;
    canvas[offset] = color.r;
    canvas[offset + 1] = color.g;
    canvas[offset + 2] = color.b;
}

/**
 * @brief Dibuja un pequeño cuadrado relleno centrado en (cx, cy),
 *        representando el cuerpo de un boid.
 */
void plotBody(std::vector<unsigned char>& canvas, int width, int height, int cx, int cy,
              const Color& color) {
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            plotPixel(canvas, width, height, cx + dx, cy + dy, color);
        }
    }
}

} // namespace

bool FrameWriter::writeFrame(const Flock& flock, int canvasWidth, int canvasHeight,
                              const std::string& filepath) {
    std::ofstream outputFile(filepath, std::ios::binary);
    if (!outputFile.is_open()) {
        return false;
    }

    // Fondo oscuro (azul muy tenue) para que los boids resalten.
    const Color background{10, 12, 30};
    const Color bodyColor{120, 220, 255};
    const Color headingColor{255, 255, 255};

    std::vector<unsigned char> canvas(static_cast<size_t>(canvasWidth) * canvasHeight * 3);
    for (size_t i = 0; i < canvas.size(); i += 3) {
        canvas[i] = background.r;
        canvas[i + 1] = background.g;
        canvas[i + 2] = background.b;
    }

    for (int i = 0; i < flock.getBoidCount(); ++i) {
        const Boid& boid = flock.getBoid(i);
        const int cx = static_cast<int>(boid.getPosition().getX());
        const int cy = static_cast<int>(boid.getPosition().getY());

        plotBody(canvas, canvasWidth, canvasHeight, cx, cy, bodyColor);

        // Pequeño segmento de dirección en el sentido de la velocidad.
        const Vector2D headingDirection = boid.getVelocity().normalized();
        for (int step = 2; step <= 5; ++step) {
            const int hx = cx + static_cast<int>(headingDirection.getX() * step);
            const int hy = cy + static_cast<int>(headingDirection.getY() * step);
            plotPixel(canvas, canvasWidth, canvasHeight, hx, hy, headingColor);
        }
    }

    outputFile << "P6\n" << canvasWidth << " " << canvasHeight << "\n255\n";
    outputFile.write(reinterpret_cast<const char*>(canvas.data()), canvas.size());

    return true;
}

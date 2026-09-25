/**
 * @file main_visual.cpp
 * @brief Punto de entrada de la modalidad gráfica secuencial de Boids.
 */

#include <cstdlib>
#include <iostream>

#include "core/Flock.hpp"
#include "core/FlockingConfig.hpp"
#include "core/SequentialScheme.hpp"
#include "visual/Renderer.hpp"

/**
 * @brief Ejecuta la simulación secuencial hasta cerrar la ventana.
 */
int main() {
    const int boidCount = 250;
    const int windowWidth = 1200;
    const int windowHeight = 800;

    const FlockingConfig config(boidCount, static_cast<double>(windowWidth),
                                 static_cast<double>(windowHeight),
                                 /*perceptionRadius=*/50.0, /*separationRadius=*/45.0,
                                 /*maxSpeed=*/2.6, /*maxForce=*/0.15,
                                 /*separationWeight=*/6.0, /*alignmentWeight=*/0.7,
                                 /*cohesionWeight=*/0.1, /*deltaTime=*/1.0);

    std::unique_ptr<Renderer> renderer = createRaylibRenderer();
    if (!renderer->init(windowWidth, windowHeight, "Flocking - Sequential")) {
        std::cerr << "Error al inicializar la ventana\n";
        return 1;
    }

    Flock flock(config, /*seed=*/42);
    SequentialScheme scheme;

    while (!renderer->shouldClose()) {
        scheme.simulateStep(flock, config);

        renderer->beginDrawing();
        renderer->drawFlock(flock);
        renderer->endDrawing();
    }

    renderer->shutdown();
    return 0;
}

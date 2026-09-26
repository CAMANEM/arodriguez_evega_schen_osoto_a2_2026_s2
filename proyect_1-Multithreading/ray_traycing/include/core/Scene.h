#ifndef SCENE_H
#define SCENE_H

#include "Sphere.h"
#include "raytracing_config.hpp"
#include "Ray.h"
#include <vector>

// Scene: Escena que contiene objetos (esferas) a renderizar.
// 
// Responsabilidad:
//   - Almacenar lista de objetos (esferas).
//   - Calcular trazado de rayos (ray tracing) contra todos los objetos.
//   - Determinar el objeto más cercano a la cámara para cada rayo.
// 
// Notas:
//   - Implementa algoritmo naive de ray tracing (complejidad O(n) por rayo).
//   - Para futuras extensiones: agregar árboles BVH o estructuras espaciales.
struct Scene {
    std::vector<Sphere> spheres;  // Lista de objetos en la escena

    // Inicializa la escena desde la configuración centralizada.
    Scene() {
        for (const auto& sphere : constants::SPHERES)
            spheres.emplace_back(sphere.center, sphere.radius, sphere.color);
    }

    // Traza un rayo en la escena y retorna el color del objeto más cercano.
    // 
    // Param: ray - Rayo a trazar (origen y dirección).
    // Return: Color del objeto intersectado (Vector3 R,G,B en [0,1]).
    //         Si no hay intersección, retorna negro (0, 0, 0).
    // 
    // Algoritmo:
    //   1. Iterar sobre todas las esferas en la escena.
    //   2. Calcular intersección rayo-esfera.
    //   3. Guardar la intersección más cercana (menor t).
    //   4. Retornar color del objeto más cercano.
    Vector3 trace(const Ray& ray) const {
        double t_min = INFINITY;
        Vector3 color = constants::BACKGROUND_COLOR;
        for (const auto& sphere : spheres) {
            double t;
            if (sphere.intersect(ray, t) && t < t_min) {
                t_min = t;
                color = sphere.color;
            }
        }
        return color;
    }
};
#endif // SCENE_H
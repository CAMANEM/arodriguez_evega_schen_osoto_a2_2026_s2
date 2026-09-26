#ifndef WORKLOAD_H
#define WORKLOAD_H

#include "raytracing_config.hpp"
#include "Ray.h"
#include "Scene.h"

enum class Workload {
    raytracing,
    dummy
};

inline Vector3 compute_pixel(const Scene& scene, int x, int y,
                             const Vector3& camera_pos, Workload workload) {
    if (workload == Workload::raytracing)
        return scene.trace(make_ray(x, y, camera_pos));

    const double width = static_cast<double>(constants::IMAGE_WIDTH);
    const double height = static_cast<double>(constants::IMAGE_HEIGHT);
    return Vector3(x / width, y / height,
                   ((x + y) % constants::IMAGE_WIDTH) / width);
}

#endif
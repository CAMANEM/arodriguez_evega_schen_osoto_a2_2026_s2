#include "visual/Renderer.hpp"

#include "raylib.h"

#include <cmath>
#include <vector>

/**
 * @brief Renderizador concreto que dibuja cada boid como un triángulo.
 */
class RaylibRenderer : public Renderer {
public:
    bool init(int width, int height, const char* title) override {
        width_ = width;
        height_ = height;
        InitWindow(width, height, title);
        SetTargetFPS(60);
        return IsWindowReady();
    }

    void beginDrawing() override {
        BeginDrawing();
        DrawRectangle(0, 0, width_, height_, Color{10, 12, 30, 100});
    }

    void drawFlock(const Flock& flock) override {
        if (smoothedVx_.size() != static_cast<size_t>(flock.getBoidCount())) {
            smoothedVx_.resize(flock.getBoidCount(), 0.0f);
            smoothedVy_.resize(flock.getBoidCount(), 0.0f);
        }

        const float smoothing = 0.3f;

        for (int i = 0; i < flock.getBoidCount(); ++i) {
            const Boid& boid = flock.getBoid(i);
            const float cx = static_cast<float>(boid.getPosition().getX());
            const float cy = static_cast<float>(boid.getPosition().getY());

            const Vector2D vel = boid.getVelocity().normalized();
            const float targetVx = static_cast<float>(vel.getX());
            const float targetVy = static_cast<float>(vel.getY());

            smoothedVx_[i] += (targetVx - smoothedVx_[i]) * smoothing;
            smoothedVy_[i] += (targetVy - smoothedVy_[i]) * smoothing;

            const float hvx = smoothedVx_[i];
            const float hvy = smoothedVy_[i];
            const float hmag = std::sqrt(hvx * hvx + hvy * hvy);
            const float nvx = (hmag > 0.001f) ? hvx / hmag : targetVx;
            const float nvy = (hmag > 0.001f) ? hvy / hmag : targetVy;

            const float height = 7.0f;
            const float halfBase = 2.5f;
            const float baseOffset = 2.0f;

            const float tipX = cx + nvx * height;
            const float tipY = cy + nvy * height;
            const float baseCenterX = cx - nvx * baseOffset;
            const float baseCenterY = cy - nvy * baseOffset;
            const float perpX = -nvy;
            const float perpY = nvx;

            Vector2 v1;
            v1.x = tipX;
            v1.y = tipY;
            Vector2 v2;
            v2.x = baseCenterX + perpX * halfBase;
            v2.y = baseCenterY + perpY * halfBase;
            Vector2 v3;
            v3.x = baseCenterX - perpX * halfBase;
            v3.y = baseCenterY - perpY * halfBase;

            Color col;
            const float speed = static_cast<float>(boid.getVelocity().magnitude());
            const float t = (speed < 3.0f) ? speed / 3.0f : 1.0f;
            col.r = static_cast<unsigned char>(80 + t * 175);
            col.g = static_cast<unsigned char>(180 + t * 40);
            col.b = static_cast<unsigned char>(255 - t * 50);
            col.a = 220;

            DrawTriangle(v1, v2, v3, col);
            DrawTriangle(v1, v3, v2, col);
        }
    }

    void endDrawing() override {
        EndDrawing();
    }

    bool shouldClose() override {
        return WindowShouldClose();
    }

    void shutdown() override {
        CloseWindow();
    }

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<float> smoothedVx_;
    std::vector<float> smoothedVy_;
};

std::unique_ptr<Renderer> createRaylibRenderer() {
    return std::make_unique<RaylibRenderer>();
}

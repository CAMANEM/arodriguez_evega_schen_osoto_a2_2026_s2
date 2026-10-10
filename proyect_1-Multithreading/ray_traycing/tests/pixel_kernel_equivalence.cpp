#include "core/strategies/RendererFactory.h"
#include <array>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool same_frame(const std::vector<Vector3>& expected,
                const std::vector<Vector3>& actual,
                const std::string& model) {
    if (expected.size() != actual.size()) {
        std::cerr << model << ": frame size differs\n";
        return false;
    }

    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (expected[i].x != actual[i].x ||
            expected[i].y != actual[i].y ||
            expected[i].z != actual[i].z) {
            std::cerr << model << ": pixel " << i << " differs\n";
            return false;
        }
    }

    return true;
}

bool verify_models() {
    constexpr std::array<const char*, 5> models = {
        "sequential", "fgmt", "cgmt", "smt", "cmp"
    };
    const Vector3 camera(0.35, 0.4, 8.0);

    auto reference = RendererFactory::create("sequential");
    reference->set_camera_pos(camera);
    const std::vector<Vector3> expected = reference->render_frame();
    if (reference->get_stall_time_ns() !=
        static_cast<long long>(reference->get_total_stalls()) *
            constants::CACHE_MISS_PENALTY_NS) {
        std::cerr << "sequential: stall time does not match miss count\n";
        return false;
    }

    for (const char* model : models) {
        if (std::string(model) == "sequential") continue;

        auto renderer = RendererFactory::create(model);
        renderer->set_camera_pos(camera);
        const std::vector<Vector3> actual = renderer->render_frame();
        if (!same_frame(expected, actual, model))
            return false;

        long long worker_stall_time_ns = 0LL;
        int worker_stalls = 0;
        int worker_context_switches = 0;
        for (const auto& stats : renderer->get_thread_metrics()) {
            worker_stall_time_ns += stats.stall_time_ns;
            worker_stalls += stats.cache_misses;
            worker_context_switches += stats.context_switches;
        }
        if (worker_stalls != renderer->get_total_stalls() ||
            worker_stall_time_ns != renderer->get_stall_time_ns() ||
            worker_context_switches != renderer->get_context_switches()) {
            std::cerr << model << ": aggregate metrics differ from worker metrics\n";
            return false;
        }

        long long expected_stall_time_ns = 0LL;
        if (std::string(model) == "fgmt")
            expected_stall_time_ns = static_cast<long long>(worker_stalls) *
                constants::PIXEL_QUANTUM_NS;
        else if (std::string(model) == "cgmt")
            expected_stall_time_ns = static_cast<long long>(worker_stalls) *
                constants::CONTEXT_SWITCH_COST_NS;
        else
            expected_stall_time_ns = static_cast<long long>(worker_stalls) *
                constants::CACHE_MISS_PENALTY_NS;
        if (renderer->get_stall_time_ns() != expected_stall_time_ns) {
            std::cerr << model << ": stall time does not match the model penalty\n";
            return false;
        }
        if ((std::string(model) == "cmp" && renderer->get_context_switches() != 0) ||
            (std::string(model) != "cmp" && renderer->get_context_switches() == 0)) {
            std::cerr << model << ": unexpected context switch count\n";
            return false;
        }

        const std::vector<Vector3> repeated_frame = renderer->render_frame();
        if (!same_frame(expected, repeated_frame, model))
            return false;

        if (std::string(model) == "smt") {
            long long context_time_ns = 0LL;
            for (const auto& stats : renderer->get_thread_metrics())
                context_time_ns += stats.virtual_time_ns;
            const long long expected_time_ns =
                static_cast<long long>(constants::IMAGE_WIDTH) *
                constants::IMAGE_HEIGHT * constants::PIXEL_QUANTUM_NS;
            if (context_time_ns != expected_time_ns) {
                std::cerr << "smt: context metrics were not reset between frames\n";
                return false;
            }
        }
    }

    return true;
}

} // namespace

int main() {
    return verify_models() ? 0 : 1;
}
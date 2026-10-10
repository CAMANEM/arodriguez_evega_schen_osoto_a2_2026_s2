#ifndef THREADED_SPH_SCHEME_HPP
#define THREADED_SPH_SCHEME_HPP

#include <algorithm>
#include <functional>
#include <thread>

#include "core/SphForces.hpp"
#include "core/SphScheme.hpp"

class ThreadedSphScheme : public SphScheme, public SphStageRunner {
public:
    void initialize(SphFluid& fluid, const SphConfig& config) override;
    void runStage(SphStage stage, SphFluid& fluid,
                  const SphConfig& config) override;
    void runStep(SphFluid& fluid, const SphConfig& config) override;
    void runAllSteps(SphFluid& fluid, const SphConfig& config) override;
    const std::vector<SphStageMetrics>& stageMetrics() const override;
    const SphRunMetrics& runMetrics() const override;

    SphMetrics simulate(SphFluid& fluid, const SphConfig& config);
    SphMetrics simulateStep(SphFluid& fluid,
                            const SphConfig& config) override;

protected:
    virtual unsigned int requestedThreadCount() const = 0;

private:
    template <typename Task>
    void parallelFor(int itemCount, Task task) const;

    unsigned int effectiveThreadCount(int itemCount) const;
    void recordStage(SphStage stage, double seconds);

    SphNeighborList neighbors_;
    std::vector<SphForceData> forces_;
    std::vector<SphStageMetrics> stageMetrics_;
    SphRunMetrics runMetrics_{0.0, 0, {}};
    unsigned int lastEffectiveThreadCount_ = 1;
};

template <typename Task>
void ThreadedSphScheme::parallelFor(int itemCount, Task task) const {
    if (itemCount <= 0) {
        return;
    }

    const unsigned int workerCount = effectiveThreadCount(itemCount);
    const int blockSize =
        (itemCount + static_cast<int>(workerCount) - 1) /
        static_cast<int>(workerCount);
    std::vector<std::thread> workers;
    workers.reserve(workerCount);

    for (unsigned int worker = 0; worker < workerCount; ++worker) {
        const int begin = static_cast<int>(worker) * blockSize;
        const int end = std::min(itemCount, begin + blockSize);
        if (begin >= end) {
            break;
        }
        workers.emplace_back([&, begin, end]() { task(begin, end); });
    }

    for (std::thread& worker : workers) {
        worker.join();
    }
}

#endif

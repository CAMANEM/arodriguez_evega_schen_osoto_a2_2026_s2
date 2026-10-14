#include <cmath>
#include <iostream>

#include "core/SphConfig.hpp"
#include "core/SphFluid.hpp"
#include "core/SphParticle.hpp"
#include "core/SphMetrics.hpp"
#include "core/SequentialSphScheme.hpp"
#include "strategies/coarse_grained/CoarseGrainedSphScheme.hpp"
#include "strategies/fine_grained/FineGrainedSphScheme.hpp"
#include "strategies/smt/SmtSphScheme.hpp"
#include "strategies/cmp/CmpSphScheme.hpp"
#include "object_interface.hpp"

int main() {
    const SphConfig config(8, 10, 0.04, 0.001, 1.0, 1000.0, 2000.0,
                           0.1, -9.81, 2.0, 3.0,
                           execution_model::sequential, 1, 17);
    SphFluid fluid(config);

    if (fluid.getParticleCount() != 8) {
        return 1;
    }
    if (fluid.getParticle(0).get_pos_x() <= 0.0 ||
        fluid.getParticle(0).get_pos_y() <= 0.0 ||
        fluid.getParticle(0).get_pos_x() >= config.getDomainWidth() ||
        fluid.getParticle(0).get_pos_y() >= config.getDomainHeight()) {
        return 1;
    }

    object_interface* particle = &fluid.getParticle(0);
    particle->set_force(1.0, 0.0);
    particle->update(config.getDeltaTime());
    fluid.reset();

    if (std::abs(particle->get_force_x()) > 1e-12 ||
        std::abs(particle->get_force_y()) > 1e-12) {
        return 1;
    }

    const SphMetrics metrics(execution_model::sequential, "sequential", 1, 8, 0.5);
    if (metrics.get_run_count() != 1 || metrics.getParticlesProcessed() != 8) {
        return 1;
    }

    SphFluid sequentialFluid(config);
    SequentialSphScheme sequential;
    const SphMetrics sequentialMetrics =
        sequential.simulate(sequentialFluid, config);
    if (sequentialMetrics.get_model() != execution_model::sequential ||
        sequentialMetrics.getParticlesProcessed() !=
            config.getParticleCount() ||
        sequentialMetrics.get_run_count() != 1 ||
        sequentialMetrics.getMeasuredSeconds() < 0.0) {
        return 1;
    }
    const SphRunMetrics& runMetrics = sequential.runMetrics();
    if (runMetrics.completedSteps != config.getTimeSteps() ||
        runMetrics.totalWallSeconds <= 0.0 ||
        runMetrics.perStage.size() != 8) {
        return 1;
    }
    for (const SphStageMetrics& stage : runMetrics.perStage) {
        if (stage.executions != config.getTimeSteps() &&
            stage.stage != SphStage::Setup) {
            return 1;
        }
        if (stage.wallSeconds < 0.0) {
            return 1;
        }
    }
    for (int index = 0; index < sequentialFluid.getParticleCount(); ++index) {
        const SphParticle& particle = sequentialFluid.getParticle(index);
        if (particle.getDensity() <
                config.getRestDensity() * config.getMinDensityRatio() ||
            std::abs(particle.getPressure()) > config.getMaxPressure() ||
            particle.get_pos_x() < 0.0 ||
            particle.get_pos_x() > config.getDomainWidth() ||
            particle.get_pos_y() < 0.0 ||
            particle.get_pos_y() > config.getDomainHeight()) {
            return 1;
        }
    }

    SphFluid fineFluid(config);
    FineGrainedSphScheme fine(2);
    const SphMetrics fineMetrics = fine.simulate(fineFluid, config);
    if (fineMetrics.get_model() != execution_model::fine_grained ||
        fine.runMetrics().completedSteps != config.getTimeSteps() ||
        fine.runMetrics().perStage.size() != runMetrics.perStage.size()) {
        return 1;
    }
    for (const SphStageMetrics& stage : fine.runMetrics().perStage) {
        if (stage.stage != SphStage::Setup &&
            stage.executions != config.getTimeSteps()) {
            return 1;
        }
    }
    for (int index = 0; index < fineFluid.getParticleCount(); ++index) {
        const SphParticle& expected = sequentialFluid.getParticle(index);
        const SphParticle& actual = fineFluid.getParticle(index);
        if (std::abs(expected.getDensity() - actual.getDensity()) > 1e-9 ||
            std::abs(expected.getPressure() - actual.getPressure()) > 1e-9 ||
            std::abs(expected.get_pos_x() - actual.get_pos_x()) > 1e-9 ||
            std::abs(expected.get_pos_y() - actual.get_pos_y()) > 1e-9) {
            return 1;
        }
    }

    SphFluid coarseFluid(config);
    CoarseGrainedSphScheme coarse(2, 4);
    const SphMetrics coarseMetrics = coarse.simulate(coarseFluid, config);
    if (coarseMetrics.get_model() != execution_model::coarse_grained ||
        coarse.runMetrics().completedSteps != config.getTimeSteps() ||
        coarse.runMetrics().perStage.size() != runMetrics.perStage.size()) {
        return 1;
    }
    for (const SphStageMetrics& stage : coarse.runMetrics().perStage) {
        if (stage.stage != SphStage::Setup &&
            stage.executions != config.getTimeSteps()) {
            return 1;
        }
    }
    for (int index = 0; index < coarseFluid.getParticleCount(); ++index) {
        const SphParticle& expected = sequentialFluid.getParticle(index);
        const SphParticle& actual = coarseFluid.getParticle(index);
        if (std::abs(expected.getDensity() - actual.getDensity()) > 1e-9 ||
            std::abs(expected.getPressure() - actual.getPressure()) > 1e-9 ||
            std::abs(expected.get_pos_x() - actual.get_pos_x()) > 1e-9 ||
            std::abs(expected.get_pos_y() - actual.get_pos_y()) > 1e-9) {
            return 1;
        }
    }

    SphFluid smtFluid(config);
    SmtSphScheme smt(1);
    const SphMetrics smtMetrics = smt.simulate(smtFluid, config);
    if (smtMetrics.get_model() != execution_model::smt ||
        smtMetrics.get_n_workers() < 1 ||
        smt.runMetrics().completedSteps != config.getTimeSteps()) {
        return 1;
    }

    SphFluid cmpFluid(config);
    CmpSphScheme cmp;
    const SphMetrics cmpMetrics = cmp.simulate(cmpFluid, config);
    if (cmpMetrics.get_model() != execution_model::cmp ||
        cmpMetrics.get_n_workers() < 1 ||
        cmp.runMetrics().completedSteps != config.getTimeSteps()) {
        return 1;
    }

    std::cout << "Liquids interface tests passed.\n";
    return 0;
}

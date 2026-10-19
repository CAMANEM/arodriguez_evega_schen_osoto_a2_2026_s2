#include "cli/SchemeFactory.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

#include "core/CmpScheme.hpp"
#include "core/CoarseGrainedScheme.hpp"
#include "core/FineGrainedScheme.hpp"
#include "core/SequentialScheme.hpp"
#include "core/SmtScheme.hpp"

namespace {

CoarseGrainedScheme::Options makeCoarseOptions(const CliOptions& options) {
    CoarseGrainedScheme::Options coarse;
    coarse.threadCount = static_cast<unsigned int>(std::max(1, options.workers));
    coarse.stallEveryBoids = options.stallEvery;
    coarse.stallProbability = options.stallProbability;
    coarse.stallMilliseconds = options.stallMs;
    coarse.stallMillisecondsMin = options.stallMsMin;
    coarse.stallMillisecondsMax = options.stallMsMax;
    coarse.seed = static_cast<std::uint32_t>(options.seed);
    coarse.logCheckpoints = options.logCoarseCheckpoints;
    return coarse;
}

} // namespace

std::unique_ptr<FlockingScheme> createScheme(const CliOptions& options) {
    switch (options.scheme) {
    case RunScheme::Sequential:
        return std::make_unique<SequentialScheme>();
    case RunScheme::Fine:
        return std::make_unique<FineGrainedScheme>(options.finePartialBoids);
    case RunScheme::Coarse:
        return std::make_unique<CoarseGrainedScheme>(makeCoarseOptions(options));
    case RunScheme::Smt:
        return std::make_unique<SmtScheme>(options.smtOversubscribe);
    case RunScheme::Cmp:
        return std::make_unique<CmpScheme>();
    case RunScheme::Compare:
        throw std::runtime_error("Compare no crea un unico esquema; use runCompare()");
    }
    throw std::runtime_error("Esquema no soportado");
}

#include "cli/SphSchemeFactory.hpp"

#include "cli/SphCli.hpp"
#include "core/SequentialSphScheme.hpp"
#include "strategies/cmp/CmpSphScheme.hpp"
#include "strategies/coarse_grained/CoarseGrainedSphScheme.hpp"
#include "strategies/fine_grained/FineGrainedSphScheme.hpp"
#include "strategies/smt/SmtSphScheme.hpp"

std::unique_ptr<SphScheme> makeSphScheme(const std::string& model,
                                         const SphCliOptions& options) {
    const execution_model executionModel = parseSphModel(model);
    switch (executionModel) {
    case execution_model::sequential:
        return std::make_unique<SequentialSphScheme>();
    case execution_model::fine_grained:
        return std::make_unique<FineGrainedSphScheme>(options.threads,
                                                       options.quantum);
    case execution_model::coarse_grained:
        return std::make_unique<CoarseGrainedSphScheme>(options.threads,
                                                         options.chunk);
    case execution_model::smt:
        return std::make_unique<SmtSphScheme>(options.smtFactor);
    case execution_model::cmp:
        return std::make_unique<CmpSphScheme>();
    }
    throw std::invalid_argument("unsupported SPH model: " + model);
}

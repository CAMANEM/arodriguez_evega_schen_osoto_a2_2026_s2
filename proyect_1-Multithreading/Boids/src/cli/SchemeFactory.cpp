#include "cli/SchemeFactory.hpp"

#include <algorithm>
#include <stdexcept>

#include "core/CmpScheme.hpp"
#include "core/CoarseGrainedScheme.hpp"
#include "core/FineGrainedScheme.hpp"
#include "core/SequentialScheme.hpp"
#include "core/SmtScheme.hpp"

std::unique_ptr<FlockingScheme> createScheme(const CliOptions& options) {
    switch (options.scheme) {
    case RunScheme::Sequential:
        return std::make_unique<SequentialScheme>();
    case RunScheme::Fine:
        return std::make_unique<FineGrainedScheme>(options.finePartialBoids);
    case RunScheme::Coarse:
        return std::make_unique<CoarseGrainedScheme>(
            static_cast<unsigned int>(std::max(1, options.workers)));
    case RunScheme::Smt:
        return std::make_unique<SmtScheme>(options.smtOversubscribe);
    case RunScheme::Cmp:
        return std::make_unique<CmpScheme>();
    case RunScheme::Compare:
        throw std::runtime_error("Compare no crea un unico esquema; use runCompare()");
    }
    throw std::runtime_error("Esquema no soportado");
}

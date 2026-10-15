#ifndef SPH_SCHEME_FACTORY_HPP
#define SPH_SCHEME_FACTORY_HPP

#include <memory>
#include <string>

#include "core/SphScheme.hpp"

struct SphCliOptions;

std::unique_ptr<SphScheme> makeSphScheme(const std::string& model,
                                         const SphCliOptions& options);

#endif

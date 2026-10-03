#ifndef SCHEME_FACTORY_HPP
#define SCHEME_FACTORY_HPP

#include <memory>

#include "cli/CliOptions.hpp"
#include "core/FlockingScheme.hpp"

/**
 * @brief Crea el esquema de ejecución pedido por CLI.
 * @note No aplica a RunScheme::Compare (ese modo orquesta varios esquemas).
 */
std::unique_ptr<FlockingScheme> createScheme(const CliOptions& options);

#endif // SCHEME_FACTORY_HPP

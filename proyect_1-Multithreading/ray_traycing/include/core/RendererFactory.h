/**
 * @file RendererFactory.h
 * @brief Registro central de renderers y creación por nombre de modelo.
 */
#ifndef RENDERER_FACTORY_H
#define RENDERER_FACTORY_H

#include "IRenderer.h"
#include "SequentialRenderer.h"
#include "FinegrainedRenderer.h"
#include "CoarseRenderer.h"
#include "SMTRenderer.h"
#include "CMPRenderer.h"
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

/**
 * @brief Crea implementaciones de IRenderer a partir de su identificador.
 *
 * Los modelos disponibles se registran en un único mapa; los clientes dependen
 * de la interfaz y no de las clases concretas.
 */
class RendererFactory {
    using FactoryFn = std::function<std::unique_ptr<IRenderer>()>;

    // Registro de modelos disponibles: modelo → factory lambda.
    static const std::unordered_map<std::string, FactoryFn>& available_registry() {
        static const std::unordered_map<std::string, FactoryFn> reg = {
            {"sequential", [] { return std::make_unique<SequentialRenderer>(); }},
            {"fgmt",       [] { return std::make_unique<FinegrainedRenderer>(); }},
            {"cgmt",       [] { return std::make_unique<CoarseRenderer>(); }},
            {"smt",        [] { return std::make_unique<SMTRenderer>(); }},
            {"cmp",        [] { return std::make_unique<CMPRenderer>(); }},
        };
        return reg;
    }

    // Registro de modelos en desarrollo: modelo → mensaje de error.
    static const std::unordered_map<std::string, std::string>& dev_registry() {
        static const std::unordered_map<std::string, std::string> reg = {
        };
        return reg;
    }

public:
    /**
     * @brief Construye el renderer solicitado.
     * @param model_name Identificador del modelo.
     * @return Renderer propietario mediante unique_ptr.
     * @throws std::runtime_error Si el modelo está registrado como no disponible.
     * @throws std::invalid_argument Si el identificador no se reconoce.
     */
    static std::unique_ptr<IRenderer> create(const std::string& model_name) {
        auto it = available_registry().find(model_name);
        if (it != available_registry().end())
            return it->second();

        auto dev_it = dev_registry().find(model_name);
        if (dev_it != dev_registry().end())
            throw std::runtime_error(dev_it->second);

        throw std::invalid_argument(
            "Unknown model: " + model_name + ". Available: sequential, fgmt, cgmt, smt, cmp");
    }

    /** @param model_name Identificador del modelo. @return true si está disponible. */
    static bool is_available(const std::string& model_name) {
        return available_registry().count(model_name) > 0;
    }

    /** @return Texto de ayuda con modelos y opciones registradas. */
    static std::string get_help_message() {
        return "Usage: ./raytracer [--model MODEL] [--runs N] [--verbose N]\n"
               "Models available: sequential, fgmt, cgmt, smt, cmp\n"
               "Options:\n"
               "  --verbose N   Imprimir los primeros N ciclos del scheduler (todos los modelos)\n"
               "Example: ./raytracer --model smt --runs 1 --verbose 30\n";
    }
};

#endif // RENDERER_FACTORY_H

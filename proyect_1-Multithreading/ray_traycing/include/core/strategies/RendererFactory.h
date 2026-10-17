/**
 * @file RendererFactory.h
 * @brief Registro central de renderers y creación por nombre de modelo.
 */
#ifndef RENDERER_FACTORY_H
#define RENDERER_FACTORY_H

#include "core/strategies/IRenderer.h"
#include "core/strategies/SequentialRenderer.h"
#include "core/strategies/FinegrainedRenderer.h"
#include "core/strategies/CoarseRenderer.h"
#include "core/strategies/SMTRenderer.h"
#include "core/strategies/CMPRenderer.h"
#include "core/config/raytracing_config.hpp"
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
    using FactoryFn = std::function<std::unique_ptr<IRenderer>(int)>;

    // Registro de modelos disponibles: modelo → factory lambda.
    static const std::unordered_map<std::string, FactoryFn>& available_registry() {
        static const std::unordered_map<std::string, FactoryFn> reg = {
            {"sequential", [](int) { return std::make_unique<SequentialRenderer>(); }},
            {"fgmt", [](int workers) {
                return workers > 0 ? std::make_unique<FinegrainedRenderer>(workers)
                                  : std::make_unique<FinegrainedRenderer>();
            }},
            {"cgmt", [](int workers) {
                return workers > 0 ? std::make_unique<CoarseRenderer>(workers)
                                  : std::make_unique<CoarseRenderer>();
            }},
            {"smt", [](int workers) {
                return workers > 0 ? std::make_unique<SMTRenderer>(workers)
                                  : std::make_unique<SMTRenderer>();
            }},
            {"cmp", [](int workers) {
                return workers > 0 ? std::make_unique<CMPRenderer>(workers)
                                  : std::make_unique<CMPRenderer>();
            }},
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
        return create(model_name, 0);
    }

    /**
     * @brief Construye un modelo paralelo con una cantidad explícita de workers.
     * @param model_name Identificador del modelo.
     * @param workers Cantidad de workers o contextos virtuales.
     * @return Renderer propietario mediante unique_ptr.
     */
    /**
     * @brief Maps campaign aliases to registry keys (fine→fgmt, coarse→cgmt).
     * @param model_name CLI model identifier.
     * @return Canonical registry key.
     */
    static std::string normalize_model_name(const std::string& model_name) {
        if (model_name == "fine") return "fgmt";
        if (model_name == "coarse") return "cgmt";
        return model_name;
    }

    /**
     * @brief Maps registry keys to campaign-canonical CSV labels.
     * @param registry_name Internal model key (fgmt|cgmt|...).
     * @return Canonical label (fine|coarse|...).
     */
    static std::string canonical_model_label(const std::string& registry_name) {
        if (registry_name == "fgmt") return "fine";
        if (registry_name == "cgmt") return "coarse";
        return registry_name;
    }

    static std::unique_ptr<IRenderer> create(const std::string& model_name, int workers) {
        const std::string key = normalize_model_name(model_name);
        auto it = available_registry().find(key);
        if (it != available_registry().end())
            return it->second(workers);

        throw std::invalid_argument(
            "Unknown model: " + model_name +
            ". Available: sequential, fine|fgmt, coarse|cgmt, smt, cmp");
    }

    /** @param model_name Identificador del modelo. @return true si está disponible. */
    static bool is_available(const std::string& model_name) {
        return available_registry().count(normalize_model_name(model_name)) > 0;
    }

    /** @return Texto de ayuda con modelos y opciones registradas. */
    static std::string get_help_message() {
        return "Usage: ./raytracer [--model MODEL] [--runs N] [--workers N] [--verbose N]\n"
               "Models available: sequential, fine|fgmt, coarse|cgmt, smt, cmp\n"
               "Options:\n"
               "  --workers N  Workers for FGMT/CGMT/CMP; virtual contexts for SMT\n"
               "  --verbose N   Imprimir los primeros N ciclos del scheduler (todos los modelos)\n"
               "Example: ./raytracer --model smt --runs 1 --verbose 30\n";
    }
};

#endif // RENDERER_FACTORY_H

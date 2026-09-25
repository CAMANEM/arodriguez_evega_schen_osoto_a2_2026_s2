#ifndef COARSE_GRAINED_SCHEME_HPP
#define COARSE_GRAINED_SCHEME_HPP

#include "core/ThreadedScheme.hpp"

/**
 * @brief Implementación directa mediante hilos tradicionales del sistema
 *        operativo (Coarse-Grained Multithreading).
 *
 * Representa el mecanismo base de "hilos tradicionales donde el cambio de
 * contexto ocurre únicamente ante eventos de bloqueo costosos" (join),
 * usando una cantidad fija y moderada de hilos, independiente de la
 * cantidad de núcleos físicos disponibles.
 *
 * 
 */
class CoarseGrainedScheme : public ThreadedScheme {
public:
    /**
     * @brief Construye el esquema de grano grueso.
     * @param threadCount Cantidad fija de hilos tradicionales a utilizar.
     */
    explicit CoarseGrainedScheme(unsigned int threadCount = 4);

    std::string getSchemeName() const override;
    execution_model getExecutionModel() const override;

protected:
    unsigned int computeThreadCount(const Flock& flock) const override;

private:
    unsigned int threadCount_;
};

#endif // COARSE_GRAINED_SCHEME_HPP

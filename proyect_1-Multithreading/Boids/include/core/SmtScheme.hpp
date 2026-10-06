#ifndef SMT_SCHEME_HPP
#define SMT_SCHEME_HPP

#include "core/ThreadedScheme.hpp"

/**
 * @brief Aproximación por software de SMT mediante sobre-suscripción de hilos.
 *
 * @details Calcula `T = hardware_concurrency() * oversubscriptionFactor` para
 *          forzar contención entre hilos de software sobre los procesadores
 *          lógicos reportados. Reutiliza `ThreadedScheme` (partición, workers,
 *          join e integración); esta clase solo define la política de `T` y
 *          las métricas del modelo `smt`.
 *
 * @note No implementa Hyper-Threading / SMT de hardware. El contraste real se
 *       hace deshabilitando SMT desde BIOS/UEFI y perfilando con perf/VTune
 *       (ver `docs/demo2/smt.md`). `hardware_concurrency()` reporta
 *       procesadores lógicos, no núcleos físicos.
 *
 * @see ThreadedScheme, CmpScheme, CoarseGrainedScheme
 */
class SmtScheme : public ThreadedScheme {
public:
    /**
     * @brief Construye el esquema SMT por sobre-suscripción.
     * @param oversubscriptionFactor Factor `F` (>= 1). Valores menores a 1 se
     *        clampan a 1. Default: 2 → `T = 2 * L`.
     */
    explicit SmtScheme(unsigned int oversubscriptionFactor = 2);

    std::string getSchemeName() const override;
    execution_model getExecutionModel() const override;

    /**
     * @return Factor de sobre-suscripción efectivo (`F`, siempre >= 1).
     */
    unsigned int getOversubscriptionFactor() const;

    /**
     * @brief Procesadores lógicos reportados por el SO.
     * @return `max(1, std::thread::hardware_concurrency())`.
     * @note Si SMT hardware ya está ON, este valor incluye hilos HT.
     */
    static unsigned int logicalProcessorCount();

    /**
     * @brief Presupuesto de hilos solicitado: `L * F` (antes de acotar por N).
     * @return Producto de procesadores lógicos y factor de sobre-suscripción.
     */
    unsigned int requestedThreadCount() const;

protected:
    /**
     * @brief Política SMT: `T = L * F`.
     * @param flock No usado; la política depende de `L` y `F`, no de N.
     * @return Cantidad de hilos a pedir a `ThreadedScheme` (>= 1).
     */
    unsigned int computeThreadCount(const Flock& flock) const override;

    /**
     * @brief Métricas con modelo `smt`, workers efectivos, `F` y `L`.
     */
    BoidsMetrics makeStepMetrics(int workersUsed, double elapsedMilliseconds,
                                 int boidsProcessed) const override;

private:
    unsigned int oversubscriptionFactor_;
};

#endif // SMT_SCHEME_HPP

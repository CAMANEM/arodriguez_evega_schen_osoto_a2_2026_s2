#ifndef CMP_SCHEME_HPP
#define CMP_SCHEME_HPP

#include "core/ThreadedScheme.hpp"

/**
 * @brief Esquema CMP: paralelismo directo con un hilo por procesador lógico.
 *
 * @details Usa `T = hardware_concurrency()` para aprovechar el paralelismo
 *          multinúcleo/multiprocesador visible al sistema operativo, sin
 *          sobre-suscripción deliberada (SMT) ni cantidad fija arbitraria
 *          (Coarse). Reutiliza `ThreadedScheme` (partición, workers, join e
 *          integración); esta clase solo define la política de `T` y las
 *          métricas del modelo `cmp`.
 *
 * @note `hardware_concurrency()` reporta procesadores lógicos, no núcleos
 *       físicos. Con SMT/HT habilitado en BIOS, `L` puede incluir hilos
 *       hardware SMT; eso debe declararse al interpretar mediciones
 *       (ver `docs/demo2/cmp.md`). No se exige affinity/pinneo de hilos.
 *
 * @see ThreadedScheme, SmtScheme, CoarseGrainedScheme
 */
class CmpScheme : public ThreadedScheme {
public:
    std::string getSchemeName() const override;
    execution_model getExecutionModel() const override;

    /**
     * @brief Procesadores lógicos reportados por el SO.
     * @return `max(1, std::thread::hardware_concurrency())`.
     * @note Si `hardware_concurrency()` es 0, se clamp a 1.
     */
    static unsigned int logicalProcessorCount();

protected:
    /**
     * @brief Política CMP canónica: `T = L` (sin multiplicar).
     * @param flock No usado; la política depende solo de `L`.
     * @return Cantidad de hilos a pedir a `ThreadedScheme` (>= 1).
     */
    unsigned int computeThreadCount(const Flock& flock) const override;

    /**
     * @brief Métricas con modelo `cmp`, workers efectivos y `L` detectado.
     */
    BoidsMetrics makeStepMetrics(int workersUsed, double elapsedMilliseconds,
                                 int boidsProcessed) const override;
};

#endif // CMP_SCHEME_HPP

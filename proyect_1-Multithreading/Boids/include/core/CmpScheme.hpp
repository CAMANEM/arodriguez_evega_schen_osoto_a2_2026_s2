#ifndef CMP_SCHEME_HPP
#define CMP_SCHEME_HPP

#include "core/ThreadedScheme.hpp"

/**
 * @brief Aproximación de multiprocesamiento de chip (CMP) con hilos reales.
 *
 * A diferencia de SmtScheme (que sobre-suscribe hilos deliberadamente para
 * competir por los mismos núcleos), este esquema lanza exactamente un hilo
 * por procesador lógico reportado por el sistema operativo. La API estándar
 * no distingue núcleos físicos de hilos SMT; esta limitación debe declararse
 * al interpretar las mediciones.
 *
 * 
 */
class CmpScheme : public ThreadedScheme {
public:
    std::string getSchemeName() const override;
    execution_model getExecutionModel() const override;

protected:
    unsigned int computeThreadCount(const Flock& flock) const override;
};

#endif // CMP_SCHEME_HPP

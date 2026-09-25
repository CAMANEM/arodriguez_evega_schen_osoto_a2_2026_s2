#ifndef SMT_SCHEME_HPP
#define SMT_SCHEME_HPP

#include "core/ThreadedScheme.hpp"

/**
 * @brief Aproximación por software del Multihilo Simultáneo (SMT).
 *
 * El enunciado indica que SMT "debe aproximarse mediante ejecución
 * concurrente con sobre-suscripción de hilos respecto a los núcleos
 * físicos". Esta clase lanza el doble de hilos de los que reporta
 * std::thread::hardware_concurrency(), forzando que varios hilos de
 * software compitan por los mismos núcleos físicos.
 *
 * 
 */
class SmtScheme : public ThreadedScheme {
public:
    /**
     * @brief Construye el esquema SMT.
     * @param oversubscriptionFactor Cuántas veces se sobre-suscriben los
     *        hilos respecto a los núcleos lógicos reportados por el
     *        sistema operativo (por defecto, 2x).
     */
    explicit SmtScheme(unsigned int oversubscriptionFactor = 2);

    std::string getSchemeName() const override;
    execution_model getExecutionModel() const override;

protected:
    unsigned int computeThreadCount(const Flock& flock) const override;

private:
    unsigned int oversubscriptionFactor_;
};

#endif // SMT_SCHEME_HPP

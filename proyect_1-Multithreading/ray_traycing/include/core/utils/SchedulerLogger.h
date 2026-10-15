/**
 * @file SchedulerLogger.h
 * @brief Logging sincronizado de eventos de los schedulers por ciclo.
 */
#ifndef SCHEDULER_LOGGER_H
#define SCHEDULER_LOGGER_H

#include <iostream>
#include <iomanip>
#include <mutex>
#include <string>

// SchedulerLogger: traza ciclo-a-ciclo del scheduler, compartida por todos los modelos.
//
// SOLID:
//   SRP — toda la lógica de formato vive aquí; los renderers solo llaman log_*.
//   OCP — agregar un modelo nuevo solo requiere añadir llamadas log_*; no tocar este archivo.
//   DIP — IRenderer::set_verbose() expone el contrato; main depende de la interfaz,
//          no de cada renderer concreto.
//   DRY — un único formato de columnas para los 5 modelos (sequential, fgmt, cgmt, smt, cmp).
//
// Thread-safety:
//   std::mutex protege la salida para modelos con OS threads reales (CMP).
//   En modelos serializados (FGMT semaforos, CGMT mutex) el mutex no contende
//   porque solo un thread escribe a la vez.
//
// Uso:
//   logger.set_max_cycles(32);               // 0 = deshabilitado
//   logger.log_header("fgmt", 4);
//   logger.log_compute(cycle, tid, x, y, PIXEL_QUANTUM_NS);
//   logger.log_stall  (cycle, tid, x, y, cost_ns, "nota del scheduler");
//   logger.log_done   (cycle, tid);
//
// Desde la línea de comandos:
//   ./build/raytracer --model fgmt --verbose 30 --runs 1
/**
 * @brief Emite trazas acotadas de cómputo, stalls y finalización de workers.
 *
 * El mutex protege las escrituras concurrentes del modelo CMP. El límite de
 * ciclos evita que el logging cambie excesivamente el coste de una ejecución.
 */
class SchedulerLogger {
public:
    /** @brief Configura cuántos ciclos iniciales se registran. */
    void set_max_cycles(int n) { max_cycles_ = n; }

    /** @param cycle Ciclo consultado. @return true si está dentro del límite activo. */
    bool active(int cycle) const { return max_cycles_ > 0 && cycle < max_cycles_; }

    /**
     * @brief Imprime configuración del modelo al inicio de una traza.
     * @param model Nombre del esquema.
     * @param threads Cantidad de workers o contextos reportados.
     * @param issue_width Slots de emisión por ciclo.
     * @param pixel_quantum_ns Quantum por píxel, en ns.
     * @param stall_penalty_ns Penalización de miss, en ns.
     */
    void log_header(const std::string& model, int threads, int issue_width = 1,
                    long long pixel_quantum_ns = 1000, long long stall_penalty_ns = 3200) {
        if (max_cycles_ <= 0) return;
        std::lock_guard<std::mutex> lk(mu_);
        std::cout << "\n[" << model << "]"
                  << "  threads="       << threads
                  << "  issue_width="   << issue_width
                  << "  pixel_quantum=" << pixel_quantum_ns << "ns"
                  << "  stall_penalty=" << stall_penalty_ns << "ns\n"
                  << std::string(72, '-') << "\n";
    }

    /** @brief Registra el procesamiento exitoso de un píxel. */
    void log_compute(int cycle, int tid, int x, int y, long long vt_ns) {
        if (!active(cycle)) return;
        std::lock_guard<std::mutex> lk(mu_);
        std::cout << "[C" << std::setw(5) << cycle << "]"
                  << " T" << tid << " COMPUTE"
                  << " px=(" << std::setw(3) << x << "," << std::setw(2) << y << ")"
                  << " +" << std::setw(5) << vt_ns << "ns\n";
    }

    /**
     * @brief Registra un miss y el coste virtual que le asigna el modelo.
     * @param cycle Ciclo simulado.
     * @param tid Identificador del worker.
     * @param x Coordenada horizontal del píxel pendiente.
     * @param y Coordenada vertical del píxel pendiente.
     * @param vt_ns Coste virtual asignado al evento, en nanosegundos.
     * @param note Texto breve sobre la política aplicada.
     */
    void log_stall(int cycle, int tid, int x, int y, long long vt_ns,
                   const char* note = "") {
        if (!active(cycle)) return;
        std::lock_guard<std::mutex> lk(mu_);
        std::cout << "[C" << std::setw(5) << cycle << "]"
                  << " T" << tid << " STALL  "
                  << " px=(" << std::setw(3) << x << "," << std::setw(2) << y << ")"
                  << " +" << std::setw(5) << vt_ns << "ns";
        if (note && *note) std::cout << "  (" << note << ")";
        std::cout << "\n";
    }

    /** @brief Registra que un worker terminó su rango de píxeles. */
    void log_done(int cycle, int tid) {
        if (!active(cycle)) return;
        std::lock_guard<std::mutex> lk(mu_);
        std::cout << "[C" << std::setw(5) << cycle << "]"
                  << " T" << tid << " DONE\n";
    }

private:
    int        max_cycles_ = 0;
    std::mutex mu_;
};

#endif // SCHEDULER_LOGGER_H

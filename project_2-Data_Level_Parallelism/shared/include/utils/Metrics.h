#pragma once
#include <cmath>
#include <limits>
#include <vector>

namespace dlp {

/**
 * @brief Estadísticas de pared de N ejecuciones de un renderer.
 *
 * Usada por todos los renderers (Sequential, SIMD, CUDA, FPGA) para comparar
 * speedup y eficiencia contra el baseline secuencial.
 */
struct RunStats {
    int    runs     = 0;
    double mean_s   = 0.0;
    double stddev_s = 0.0;
    double min_s    = std::numeric_limits<double>::max();
    double max_s    = 0.0;

    static RunStats from_times(const std::vector<double>& times) {
        RunStats s;
        if (times.empty()) return s;
        s.runs = static_cast<int>(times.size());
        double sum = 0.0;
        for (double t : times) {
            sum += t;
            if (t < s.min_s) s.min_s = t;
            if (t > s.max_s) s.max_s = t;
        }
        s.mean_s = sum / s.runs;
        if (s.runs > 1) {
            double sq = 0.0;
            for (double t : times) sq += (t - s.mean_s) * (t - s.mean_s);
            s.stddev_s = std::sqrt(sq / (s.runs - 1));
        }
        return s;
    }

    /** @brief Speedup frente al baseline secuencial (1.0 si es el propio baseline). */
    double speedup(double seq_mean_s) const {
        return (seq_mean_s > 0.0 && mean_s > 0.0) ? seq_mean_s / mean_s : 1.0;
    }

    /** @brief Eficiencia paralela: speedup / workers (1.0 para el baseline). */
    double efficiency(double seq_mean_s, int workers) const {
        return workers > 0 ? speedup(seq_mean_s) / workers : 0.0;
    }
};

} // namespace dlp

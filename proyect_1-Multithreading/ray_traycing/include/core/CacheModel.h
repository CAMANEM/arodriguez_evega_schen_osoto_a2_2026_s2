/**
 * @file CacheModel.h
 * @brief Modelo probabilístico reproducible de hits y misses por píxel.
 */
#ifndef CACHE_MODEL_H
#define CACHE_MODEL_H

#include <cmath>
#include <random>

/**
 * @brief Simula misses de caché con localidad espacial y semilla reproducible.
 *
 * La probabilidad base depende del tamaño configurado y se reduce para píxeles
 * próximos al último miss. Cada contexto debe usar su propia instancia.
 */
class CacheModel {
private:
    int cache_size;                // Bytes de cache disponible
    int last_miss_x, last_miss_y;  // Posición del último miss
    std::mt19937 rng;              // Generador aleatorio
    std::uniform_real_distribution<double> dist;

public:
    /**
     * @brief Crea un modelo de caché con generador aleatorio independiente.
     * @param cache_size Capacidad simulada en bytes.
     * @param seed Semilla del generador; usar una distinta por contexto.
     * @pre cache_size debe ser positivo.
     */
    CacheModel(int cache_size = 32768, uint32_t seed = 42u)
        : cache_size(cache_size), last_miss_x(0), last_miss_y(0), dist(0.0, 1.0) {
        rng.seed(seed);
    }

    /**
     * @brief Decide si acceder al píxel indicado produce un miss.
     * @param x Coordenada horizontal del píxel.
     * @param y Coordenada vertical del píxel.
     * @return true si ocurre un miss; false si el acceso es un hit.
     */
    bool is_cache_miss(int x, int y) {
        // Probabilidad base inversamente proporcional al tamaño de cache
        double base_prob = 64.0 / cache_size;  // ~0.002 para 32KB cache
        
        // Distancia manhatan desde último miss
        int dx = std::abs(x - last_miss_x);
        int dy = std::abs(y - last_miss_y);
        int distance = dx + dy;
        
        // Localidad espacial: si está cerca del último miss, menor probabilidad
        // Rango típico: 0-50 píxeles de distancia
        double spatial_factor = 1.0 - std::exp(-distance / 20.0);  // Converge a 1.0
        
        double miss_prob = base_prob * spatial_factor;
        
        // Generar valor aleatorio y comparar
        bool is_miss = dist(rng) < miss_prob;
        
        // Actualizar posición de último miss si ocurre
        if (is_miss) {
            last_miss_x = x;
            last_miss_y = y;
        }
        
        return is_miss;
    }

    /** @brief Reinicia la localidad espacial al comenzar un frame. */
    void reset() {
        last_miss_x = 0;
        last_miss_y = 0;
    }

    /** @return Capacidad simulada de caché en bytes. */
    int get_cache_size() const { return cache_size; }
    /** @param size Nueva capacidad simulada, en bytes. */
    void set_cache_size(int size) { cache_size = size; }
};

#endif // CACHE_MODEL_H

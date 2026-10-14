#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <memory>

#include "core/Flock.hpp"

/**
 * @brief Interfaz abstracta para la visualización del enjambre.
 *
 * Permite ejecutar la lógica de simulación sin dependencias gráficas
 * (por ejemplo, en modo benchmark) o con una implementación visual
 * concreta (raylib, SDL, etc.) sin modificar el código cliente.
 */
class Renderer {
public:
    virtual ~Renderer() = default;

    /**
     * @brief Inicializa la ventana y recursos gráficos.
     * @param width Ancho de la ventana en píxeles.
     * @param height Alto de la ventana en píxeles.
     * @param title Título de la ventana.
     * @return true si la inicialización fue exitosa.
     */
    virtual bool init(int width, int height, const char* title) = 0;

    /** @brief Inicia el dibujado de un frame. */
    virtual void beginDrawing() = 0;

    /**
     * @brief Dibuja el estado actual del enjambre.
     * @param flock Enjambre a visualizar.
     */
    virtual void drawFlock(const Flock& flock) = 0;

    /** @brief Finaliza el dibujado y presenta el frame. */
    virtual void endDrawing() = 0;

    /** @return true si el usuario pidió cerrar la ventana. */
    virtual bool shouldClose() = 0;

    /** @brief Libera recursos gráficos y cierra la ventana. */
    virtual void shutdown() = 0;
};

/**
 * @brief Crea la implementación de Renderer basada en Raylib.
 * @return Puntero propietario con liberación automática.
 */
std::unique_ptr<Renderer> createRaylibRenderer();

#endif // RENDERER_HPP

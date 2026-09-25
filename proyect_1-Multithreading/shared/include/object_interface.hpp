#pragma once

/**
 * @brief Contrato físico común para los objetos de los problemas del proyecto.
 *
 * La clase conserva posición, velocidad, aceleración y fuerza en dos
 * dimensiones. Cada problema decide cómo aplica sus restricciones particulares
 * al implementar update() y reset().
 */
class object_interface {

/* ===== ATTRIBUTES ============================================================================ */
protected:
    int id;
    double mass;
    double pos_x;
    double pos_y;
    double acc_x;
    double acc_y;
    double force_x;
    double force_y;
    double speed_x;
    double speed_y;

public:
    /**
     * @brief Construye un objeto físico en reposo en el origen.
     * @param id Identificador estable del objeto.
     * @param mass Masa positiva utilizada para calcular la aceleración.
     */
    explicit object_interface(int id, double mass = 1.0)
        : id(id), mass(mass),
          pos_x(0.0), pos_y(0.0),
          acc_x(0.0), acc_y(0.0),
          force_x(0.0), force_y(0.0),
          speed_x(0.0), speed_y(0.0)
    {}

    /** @brief Conserva el estado físico al copiar objetos derivados. */
    object_interface(const object_interface&) = default;

    /** @brief Conserva el estado físico al asignar objetos derivados. */
    object_interface& operator=(const object_interface&) = default;

    /** @brief Permite destruir objetos derivados mediante la interfaz común. */
    virtual ~object_interface() = default;

    /**
     * @brief Avanza el estado físico un intervalo de tiempo.
     * @param dt Duración positiva del paso de simulación.
     */
    virtual void update(double dt) = 0;

    /** @brief Restablece el estado dinámico definido por la clase concreta. */
    virtual void reset() = 0;

    /** @return Identificador estable del objeto. */
    int get_id() const {return id;}
    /** @return Masa del objeto. */
    double get_mass() const {return mass;}
    /** @return Coordenada horizontal. */
    double get_pos_x() const {return pos_x;}
    /** @return Coordenada vertical. */
    double get_pos_y() const {return pos_y;}
    /** @return Aceleración horizontal. */
    double get_acc_x() const {return acc_x;}
    /** @return Aceleración vertical. */
    double get_acc_y() const {return acc_y;}
    /** @return Fuerza horizontal acumulada. */
    double get_force_x() const {return force_x;}
    /** @return Fuerza vertical acumulada. */
    double get_force_y() const {return force_y;}
    /** @return Velocidad horizontal. */
    double get_speed_x() const {return speed_x;}
    /** @return Velocidad vertical. */
    double get_speed_y() const {return speed_y;}

    /** @brief Cambia la masa del objeto. */
    void set_mass(double m) {mass = m;}
    /** @brief Cambia ambas coordenadas de posición. */
    void set_position(double x, double y) {pos_x = x; pos_y = y;}
    /** @brief Cambia ambas componentes de aceleración. */
    void set_acceleration(double ax, double ay) {acc_x = ax; acc_y = ay;}
    /** @brief Cambia ambas componentes de fuerza. */
    void set_force(double fx, double fy) {force_x = fx; force_y = fy;}
    /** @brief Cambia ambas componentes de velocidad. */
    void set_velocity(double vx, double vy) {speed_x = vx; speed_y = vy;}
};

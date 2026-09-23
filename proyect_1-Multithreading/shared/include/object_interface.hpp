#pragma once

/* ===== GENERAL OBJECTS INTERFACE ============================================================= */
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
    /* ===== CONSTRUCTOR AND DESTRUCTOR ======================================================== */
    explicit object_interface(int id, double mass = 1.0)
        : id(id), mass(mass),
          pos_x(0.0), pos_y(0.0),
          acc_x(0.0), acc_y(0.0),
          force_x(0.0), force_y(0.0),
          speed_x(0.0), speed_y(0.0)
    {}

    virtual ~object_interface() = default;

    /* ===== OTHER METHODS ===================================================================== */
    virtual void update(double dt) = 0;
    virtual void reset() = 0;

    /* ===== GETTERS =========================================================================== */
    int get_id() const {return id;}
    double get_mass() const {return mass;}
    double get_pos_x() const {return pos_x;}
    double get_pos_y() const {return pos_y;}
    double get_acc_x() const {return acc_x;}
    double get_acc_y() const {return acc_y;}
    double get_force_x() const {return force_x;}
    double get_force_y() const {return force_y;}
    double get_speed_x() const {return speed_x;}
    double get_speed_y() const {return speed_y;}

    /* ===== SETTERS =========================================================================== */
    void set_mass(double m) {mass = m;}
    void set_position(double x, double y) {pos_x = x; pos_y = y;}
    void set_acceleration(double ax, double ay) {acc_x = ax; acc_y = ay;}
    void set_force(double fx, double fy) {force_x = fx; force_y = fy;}
    void set_velocity(double vx, double vy) {speed_x = vx; speed_y = vy;}
};

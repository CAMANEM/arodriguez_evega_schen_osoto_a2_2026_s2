#ifndef VECTOR2D_HPP
#define VECTOR2D_HPP

/**
 * @brief Vector bidimensional de punto flotante con las operaciones
 *        aritméticas básicas necesarias para la física del enjambre
 *        (posición, velocidad, fuerzas de dirección).
 *
 * Se implementó como una clase propia, en vez de usar un par de doubles
 * sueltos, para que las operaciones vectoriales (suma, resta, escala,
 * magnitud, normalización, límite) se lean como álgebra vectorial en el
 * resto del código y no como aritmética escalar dispersa.
 *
 * 
 */
class Vector2D {
public:
    /**
     * @brief Construye un vector, por defecto el vector cero.
     * @param x Componente en el eje X.
     * @param y Componente en el eje Y.
     */
    explicit Vector2D(double x = 0.0, double y = 0.0);

    /** @return Componente en el eje X. */
    double getX() const;

    /** @return Componente en el eje Y. */
    double getY() const;

    Vector2D operator+(const Vector2D& other) const;
    Vector2D operator-(const Vector2D& other) const;
    Vector2D operator*(double scalar) const;
    Vector2D& operator+=(const Vector2D& other);

    /** @return Magnitud (longitud euclidiana) del vector. */
    double magnitude() const;

    /**
     * @brief Retorna una copia normalizada (magnitud 1) de este vector.
     * @return Vector normalizado, o el vector cero si la magnitud es 0.
     */
    Vector2D normalized() const;

    /**
     * @brief Retorna una copia de este vector con su magnitud limitada.
     * @param maxMagnitude Magnitud máxima permitida.
     * @return Este vector sin cambios si ya cumple el límite, o una
     *         versión escalada a maxMagnitude en caso contrario.
     */
    Vector2D limited(double maxMagnitude) const;

private:
    double x_;
    double y_;
};

#endif // VECTOR2D_HPP

# Intrinsics AVX usados por `SimdRenderer`

Esta guía explica los intrinsics que aparecen en
[`SimdRenderer.cpp`](../ray_tracing/src/core/SimdRenderer.cpp), cómo operan sobre
los cuatro píxeles de cada bloque y cómo se usan para calcular sus intersecciones.

## El tipo `__m256d`

`__m256d` representa un registro vectorial de 256 bits. Como cada valor `double`
ocupa 64 bits, contiene cuatro carriles:

```text
| carril 0 | carril 1 | carril 2 | carril 3 |
| píxel x | píxel x+1 | píxel x+2 | píxel x+3 |
```

Un intrinsic con sufijo `_pd` aplica una operación a los cuatro `double`
empaquetados. Por ejemplo, `_mm256_add_pd(a, b)` suma los carriles
correspondientes: el resultado del carril `i` es `a[i] + b[i]`.

## Intrinsics usados

### Crear vectores

| Intrinsic | Funcionamiento | Uso en el renderer |
|---|---|---|
| `_mm256_setr_pd(a, b, c, d)` | Crea un vector con los valores en ese orden: carriles 0 a 3. La `r` indica orden de lectura, de menor a mayor carril. | Guarda las coordenadas horizontales `u` de cuatro píxeles consecutivos. |
| `_mm256_set1_pd(x)` | Copia el escalar `x` a los cuatro carriles. | Difunde valores comunes a los cuatro rayos, como `aspect`, componentes de cámara, radio, constantes y color de una esfera. |
| `_mm256_setzero_pd()` | Crea un vector cuyos cuatro carriles son cero. | Proporciona el cero para las comparaciones y para expresar `-b` como `0 - b`. |

### Aritmética por carril

| Intrinsic | Funcionamiento | Uso en el renderer |
|---|---|---|
| `_mm256_add_pd(a, b)` | Suma `a[i] + b[i]` en cada carril. | Forma componentes de los rayos, productos punto y sumas de la ecuación de intersección. |
| `_mm256_sub_pd(a, b)` | Resta `a[i] - b[i]` en cada carril. | Calcula el discriminante y el numerador de la raíz `t`. |
| `_mm256_mul_pd(a, b)` | Multiplica `a[i] * b[i]` en cada carril. | Calcula productos, cuadrados, coeficientes y términos de la ecuación rayo-esfera. |
| `_mm256_div_pd(a, b)` | Divide `a[i] / b[i]` en cada carril. | Normaliza las direcciones y calcula la raíz `t` de cada rayo. |
| `_mm256_sqrt_pd(a)` | Calcula la raíz cuadrada de cada carril. | Obtiene la longitud de los rayos y la raíz cuadrada del discriminante. |
| `_mm256_max_pd(a, b)` | Selecciona el valor mayor entre `a[i]` y `b[i]` en cada carril. | Limita temporalmente el discriminante a cero o más antes de calcular su raíz cuadrada. La máscara de impacto sigue comparando el discriminante original. |

### Comparar y seleccionar

| Intrinsic o constante | Funcionamiento | Uso en el renderer |
|---|---|---|
| `_mm256_cmp_pd(a, b, predicado)` | Compara los carriles correspondientes y devuelve una máscara por carril: todos los bits en uno si la comparación es verdadera, todos en cero si es falsa. | Genera las máscaras que indican si hay intersección, si `t` es positivo y si es más cercano que el impacto guardado. |
| `_CMP_GE_OQ` | Predicado para `a >= b` (comparación ordenada y silenciosa). | Verifica que el discriminante sea no negativo. |
| `_CMP_GT_OQ` | Predicado para `a > b` (comparación ordenada y silenciosa). | Verifica que la raíz `t` quede delante del origen del rayo. |
| `_CMP_LT_OQ` | Predicado para `a < b` (comparación ordenada y silenciosa). | Verifica que la nueva raíz sea más cercana que `nearest_t`. |
| `_mm256_and_pd(a, b)` | Realiza AND bit a bit entre los 256 bits de los vectores. No es una operación AND numérica sobre los valores `double`. | Combina las tres máscaras de comparación. Un carril solo queda marcado como impacto si cumple las tres condiciones. |
| `_mm256_blendv_pd(a, b, mask)` | Para cada carril, copia el valor de `b` si el bit más significativo de ese carril de `mask` está en uno; de lo contrario, copia `a`. | Actualiza `nearest_t` y el color únicamente en los carriles donde la máscara de impacto es verdadera. |

### Pasar los resultados a memoria

| Intrinsic | Funcionamiento | Uso en el renderer |
|---|---|---|
| `_mm256_storeu_pd(destino, vector)` | Escribe los cuatro `double` del vector en memoria. La `u` indica que la dirección no tiene que estar alineada a 32 bytes. | Extrae por separado los carriles rojo, verde y azul a sus arreglos antes de construir los cuatro `Vector3`. |

## Cómo se combinan para probar una esfera

Para cada esfera, el código usa la ecuación cuadrática de intersección entre un
rayo y una esfera. Sus coeficientes son:

```text
oc = origen - centro
a  = dirección · dirección
b  = 2 * (oc · dirección)
c  = oc · oc - radio²
D  = b² - 4ac
t  = (-b - sqrt(D)) / (2a)
```

Los valores que dependen del rayo (`a`, `b`, `D` y `t`) son vectores: cada
carril calcula la intersección de un píxel distinto con la misma esfera. Las
propiedades de la esfera (`centro`, `radio`, `c` y color) son comunes a los
cuatro carriles.

El renderer combina comparaciones para aceptar solo carriles donde `D >= 0`,
`t > 0` y `t < nearest_t`. Luego usa `_mm256_blendv_pd` para conservar el
impacto más cercano y su color por separado para cada píxel. Los colores
empiezan con el color de fondo; si ningún impacto es válido, no se reemplazan.

## Elementos relacionados que no son intrinsics

- `__m256d` es el tipo de dato del registro vectorial.
- `_CMP_GE_OQ`, `_CMP_GT_OQ` y `_CMP_LT_OQ` son constantes que seleccionan el
  predicado de `_mm256_cmp_pd`; no ejecutan operaciones por sí mismas.
- Los bucles sobre las esferas y sobre los bloques de píxeles siguen siendo
  bucles escalares. El trabajo dentro de `trace_four` es el que opera sobre
  cuatro rayos por instrucción vectorial.
- Los últimos cero a tres píxeles de cada fila se procesan con
  `pixel_kernel_.compute`, no con un bloque AVX incompleto.

La compilación del renderer habilita AVX2 (`-mavx2` con GCC/Clang o
`/arch:AVX2` con MSVC). Para ejecutarlo, el procesador también debe admitir AVX2.

# Flocking / Boids — Demostración 2

Implementación C++17 de Boids para comparar una base secuencial con modelos
fine-grained, coarse-grained, SMT y CMP.

## Requisitos

- Compilador compatible con C++17.
- CMake 3.16 o posterior.
- Raylib solamente para la modalidad gráfica.

Las mediciones principales del proyecto deben ejecutarse en hardware físico,
no en WSL, máquinas virtuales ni contenedores.

## Compilar y probar

Desde la raíz del repositorio:

```bash
cmake -S proyect_1-Multithreading/Boids -B build/boids
cmake --build build/boids --config Release
ctest --test-dir build/boids -C Release --output-on-failure
```

Si Raylib no está disponible, CMake omite `boids_visual` y mantiene funcionales
el benchmark y las pruebas. También se puede desactivar explícitamente:

```bash
cmake -S proyect_1-Multithreading/Boids -B build/boids -DBOIDS_BUILD_VISUAL=OFF
```

## Ejecutar

Modalidad no gráfica:

```bash
./build/boids/boids_benchmark
```

En generadores multiconfiguración de Windows, el ejecutable suele quedar en
`build/boids/Release/boids_benchmark.exe`.

El benchmark imprime tiempos y validaciones de los cinco esquemas. También
exporta 70 frames PPM en `frames/`, correspondientes a 350 pasos. Se pueden
convertir a video con:

```bash
ffmpeg -framerate 15 -i frames/frame_%03d.ppm -pix_fmt yuv420p flock.mp4
```

Modalidad gráfica secuencial, cuando Raylib esté disponible:

```bash
./build/boids/boids_visual
```

## Evidencia de la Demostración 2

| Requisito | Implementación |
|---|---|
| Sistema base sin hilos | `SequentialScheme` |
| Variables críticas | `FlockingConfig` y `main_benchmark.cpp` |
| Dummy fine-grained | `FineGrainedScheme` y `SteeringContext` |
| Dummy coarse-grained | `CoarseGrainedScheme` y `ThreadedScheme` |
| Aproximación SMT | `SmtScheme` con sobresuscripción |
| Aproximación CMP | `CmpScheme` con trabajadores reales |
| Mediciones | `BoidsMetrics`, `metrics_interface` y `Timer` |
| Modalidad no gráfica | `main_benchmark.cpp` y `FrameWriter` |
| Modalidad gráfica | `main_visual.cpp` y `RaylibRenderer` |
| Correctitud | `tests/test_boids.cpp` y validaciones del benchmark |

Los diagramas y la explicación completa están en
[`../docs/demo2/README.md`](../docs/demo2/README.md).

## Variables que aumentan el paralelismo

- `boidCount`: determina el volumen de trabajo. La búsqueda directa revisa
  hasta `boidCount * (boidCount - 1)` pares por paso.
- `perceptionRadius` y `separationRadius`: cambian cuántos vecinos contribuyen
  realmente a las reglas y producen una carga dependiente de la densidad.
- Cantidad de trabajadores: coarse, SMT y CMP dividen los boids en bloques;
  fine crea un contexto virtual por cada boid de su demostración parcial.
- Los pesos de separación, alineamiento y cohesión cambian el comportamiento
  visual, pero no la complejidad de la búsqueda actual.

## Decisiones que deben explicarse en la defensa

- El paso se divide en cálculo de fuerzas y aplicación de integraciones. Así
  todos los esquemas leen el mismo estado y se evitan condiciones de carrera.
- Fine-grained simula por software un cambio round-robin en cada vecino
  candidato. Sus trabajadores son contextos virtuales, no hilos del SO.
- Coarse-grained usa `std::thread` y `join()` como sincronización costosa.
- SMT sobresuscribe procesadores lógicos. Esto es una aproximación de software,
  no sustituye la comparación física con SMT habilitado y deshabilitado.
- CMP usa `std::thread::hardware_concurrency()`. La API informa procesadores
  lógicos disponibles; no garantiza que sean núcleos físicos.
- `Boid` implementa `object_interface` y `BoidsMetrics` deriva de
  `metrics_interface`, por lo que Boids respeta los contratos compartidos.

## Alcance estadístico

Esta demostración usa mediciones preliminares para probar funcionamiento. Las
200 o más ejecuciones por configuración, intervalos de confianza, boxplots y
perfilado con perf o VTune corresponden a la campaña experimental final.

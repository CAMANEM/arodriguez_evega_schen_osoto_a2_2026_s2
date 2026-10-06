# Flocking / Boids — Demostración 2

Implementación C++17 de Boids para comparar una base secuencial con modelos
fine-grained, coarse-grained, SMT y CMP.

## Requisitos

- Compilador compatible con C++17.
- CMake 3.16 o posterior.
- Git (para descargar Raylib con FetchContent si no está instalado).
- Raylib solamente para la modalidad gráfica (sistema o descarga automática).

Las mediciones principales del proyecto deben ejecutarse en hardware físico,
no en WSL, máquinas virtuales ni contenedores.

## Setup automático (recomendado)

Desde la raíz del repositorio.

**Windows (PowerShell):**

```powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
.\proyect_1-Multithreading\scripts\setup_windows.ps1
# Abra una terminal nueva y luego:
.\proyect_1-Multithreading\scripts\run_demo2.ps1 -Mode all
```

**Linux:**

```bash
chmod +x proyect_1-Multithreading/scripts/*.sh
./proyect_1-Multithreading/scripts/setup_linux.sh
./proyect_1-Multithreading/scripts/run_demo2.sh all
```

Modos útiles de `run_demo2`:

| Modo | Qué hace |
|---|---|
| `benchmark` (default) | `boids --scheme compare` (tabla + frames PPM) |
| `sequential` / `cmp` | Headless infinito de ese modelo |
| `sequential-ui` / `cmp-ui` | `boids --scheme … --gui` |
| `all` | Tests + compare + ambas UIs |
| `build` / `test` | Solo compilar o compilar+ctest |

## Compilar y probar (manual)

Desde la raíz del repositorio:

```bash
cmake -S proyect_1-Multithreading/Boids -B build/boids
cmake --build build/boids --config Release
ctest --test-dir build/boids -C Release --output-on-failure
```

El resultado es un solo ejecutable: `build/boids/boids` (o `boids.exe`).
Si no hay Raylib, CMake intenta descargarlo; sin Raylib el binario funciona
pero `--gui` no estará disponible (`-DBOIDS_BUILD_VISUAL=OFF` lo omite).

## Ejecutar (CLI unificado)

```text
boids --scheme sequential|fine|coarse|smt|cmp|compare
      [--gui|--no-gui] [--forever|--steps N]
      [--boids N|--bodies N|-n N] [--workers N] [--partial N] [--oversubscribe N]
      [--perception R] [--separation R] [--seed N] ...
```

Ejemplos:

```bash
# Demo 2: comparar todos los esquemas + frames (sin UI)
./build/boids/boids --scheme compare

# Secuencial / CMP indefinidos sin UI
./build/boids/boids --scheme sequential --forever
./build/boids/boids --scheme cmp --forever

# Con GUI (hasta cerrar la ventana)
./build/boids/boids --scheme sequential --gui
./build/boids/boids --scheme cmp --gui --boids 300

# Fine-grained simulado (flock completo; --partial N para demos)
./build/boids/boids --scheme fine --validate --boids 40
./build/boids/boids --scheme fine --partial 20 --steps 1
./build/boids/boids --scheme fine --gui -n 80 --perception 60 --separation 25
./build/boids/boids --scheme fine --no-gui --bodies 120 --seed 7 --steps 50

# CMP (T = L automático; barrer N/steps, no --workers)
./build/boids/boids --scheme cmp --validate --boids 120 --steps 1
./build/boids/boids --scheme cmp --no-gui --boids 200 --steps 50
./build/boids/boids --scheme cmp --gui --boids 300

# Ayuda
./build/boids/boids --help
```

En Windows (MinGW): `build\boids\boids.exe`. Con MSVC suele estar en
`build\boids\Release\boids.exe`.

Frames PPM del modo `compare` → `frames/`. Video opcional:

```bash
ffmpeg -framerate 15 -i frames/frame_%03d.ppm -pix_fmt yuv420p flock.mp4
```

## Evidencia de la Demostración 2

| Requisito | Implementación |
|---|---|
| Sistema base sin hilos | `SequentialScheme` (`--scheme sequential`) |
| Variables críticas | `FlockingConfig` + flags CLI (`--boids`, radios, pesos) |
| Fine-grained simulado | `FineGrainedScheme` (`--scheme fine`, `--partial N` opcional) |
| Dummy coarse-grained | `CoarseGrainedScheme` (`--scheme coarse --workers N`) |
| Stalls coarse | `--stall-every`, `--stall-probability`, `--stall-ms`, `--stall-ms-min/max` |
| Aproximación SMT | `SmtScheme` (`--scheme smt --oversubscribe F`, default 2); ver [`../docs/demo2/smt.md`](../docs/demo2/smt.md) |
| CMP (multinúcleo) | `CmpScheme` (`--scheme cmp`, `T = L`); ver [`../docs/demo2/cmp.md`](../docs/demo2/cmp.md) |
| Mediciones | `BoidsMetrics`, `metrics_interface` y `Timer` |
| Modalidad no gráfica | `boids --no-gui` (default) |
| Modalidad gráfica | `boids --gui` + `RaylibRenderer` |
| Correctitud | `tests/test_boids.cpp` y `--scheme compare` / `--validate` |

Los diagramas y la explicación completa están en
[`../docs/demo2/README.md`](../docs/demo2/README.md).

## Variables que aumentan el paralelismo

- `boidCount`: determina el volumen de trabajo. La búsqueda directa revisa
  hasta `boidCount * (boidCount - 1)` pares por paso.
- `perceptionRadius` y `separationRadius`: cambian cuántos vecinos contribuyen
  realmente a las reglas y producen una carga dependiente de la densidad.
- Cantidad de trabajadores: coarse, SMT y CMP dividen los boids en bloques;
  fine crea un contexto virtual por boid (o por el subconjunto `--partial`).
- Los pesos de separación, alineamiento y cohesión cambian el comportamiento
  visual, pero no la complejidad de la búsqueda actual.

## Decisiones que deben explicarse en la defensa

- El paso se divide en cálculo de fuerzas y aplicación de integraciones. Así
  todos los esquemas leen el mismo estado y se evitan condiciones de carrera.
- Fine-grained simula por software un cambio round-robin en cada vecino
  candidato. Sus trabajadores son contextos virtuales, no hilos del SO.
- Coarse-grained usa `std::thread` y `join()` como sincronización costosa.
  Opcionalmente inyecta stalls didácticos solo tras boid completo
  con un checkpoint manual que **evidencia** el save/resume; el SO ya conserva
  el contexto del hilo de forma implícita. Detalle: [`../docs/demo2/coarse-grained.md`](../docs/demo2/coarse-grained.md).
- SMT sobresuscribe procesadores lógicos (`T = L × F`). Es una aproximación de
  software, no Hyper-Threading: el contraste real es BIOS ON/OFF + perfilado.
  Detalle: [`../docs/demo2/smt.md`](../docs/demo2/smt.md).
- CMP usa `T = hardware_concurrency()` (un hilo por procesador lógico). Es
  paralelismo directo multinúcleo; no sobresuscribe ni fija T a mano. La API
  reporta lógicos, no físicos — interpretar con BIOS SMT ON/OFF.
  Detalle: [`../docs/demo2/cmp.md`](../docs/demo2/cmp.md).
- `Boid` implementa `object_interface` y `BoidsMetrics` deriva de
  `metrics_interface`, por lo que Boids respeta los contratos compartidos.

## Alcance estadístico

Esta demostración usa mediciones preliminares para probar funcionamiento. Las
200 o más ejecuciones por configuración, intervalos de confianza, boxplots y
perfilado con perf o VTune corresponden a la campaña experimental final.

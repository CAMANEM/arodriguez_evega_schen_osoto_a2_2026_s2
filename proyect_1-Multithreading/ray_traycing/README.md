# Ray Tracing — Benchmark multithreading

Implementación C++17 de ray tracing para comparar una base secuencial con los
modelos fine-grained (FGMT), coarse-grained (CGMT), SMT y CMP. El benchmark
permite elegir el modelo, el número de workers y las dimensiones de la imagen al
ejecutar el programa.

Las mediciones experimentales finales deben ejecutarse directamente en hardware
físico, no en WSL, máquinas virtuales ni contenedores.

## Requisitos

- Compilador compatible con C++17.
- CMake 3.10 o posterior (CMake 3.21 o posterior para usar los presets).
- Soporte de threads del sistema.
- Python 3 y Matplotlib para el script de análisis de workers.
- Python 3 y Pillow solo para crear el GIF (`python -m pip install Pillow`).

## Compilar y probar

Ejecuta los comandos desde `proyect_1-Multithreading/ray_traycing/`.

**Windows (MSYS2 MinGW-w64):**

```powershell
cmake --preset msys2-mingw64
cmake --build --preset msys2-mingw64 --parallel
ctest --test-dir build-msys2 --output-on-failure
```

El preset usa `C:/msys64/mingw64/bin/g++.exe` y
`C:/msys64/mingw64/bin/mingw32-make.exe`. Si MSYS2 está instalado en otra
ruta, actualiza `CMakePresets.json`.

**Visual Studio:**

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

**Linux / macOS:**

```bash
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Con el preset MSYS2, los ejecutables quedan en `build-msys2/`. En Windows con
Visual Studio, normalmente quedan en `build/Release/`.

## Ejecutar el benchmark

```text
raytracing_benchmark
  [--model all|sequential|fgmt|cgmt|smt|cmp]
  [--runs N] [--workers N]
  [--width N] [--height N]
  [--output CSV] [--frame-csv] [--gif]
```

Ejemplos desde PowerShell:

```powershell
# Comparar todos los modelos, 200 frames por modelo
./build-msys2/raytracing_benchmark.exe --model all --runs 200 --output data/benchmark.csv

# Configurar resolución en tiempo de ejecución: 800 píxeles de ancho x 600 de alto
./build-msys2/raytracing_benchmark.exe --model all --width 800 --height 600 --runs 200 --output data/benchmark_800x600.csv

# Configurar workers/contextos en los modelos paralelos
./build-msys2/raytracing_benchmark.exe --model all --workers 8 --runs 200 --output data/all_8_workers.csv

# Ejecutar solo el modelo CMP y guardar las muestras por frame
./build-msys2/raytracing_benchmark.exe --model cmp --width 640 --height 360 --workers 8 --runs 200 --frame-csv --output data/cmp_640x360.csv

# Consultar todas las opciones
./build-msys2/raytracing_benchmark.exe --help
```

`--width N` indica el ancho y `--height N` el alto en píxeles. Las dimensiones
se aplican al frame que procesan todos los modelos, a la proyección de cámara y
a los datos de salida; se configuran al iniciar cada ejecución, sin editar el
código ni recompilar. Si se omiten, se usa la resolución predeterminada de
240 × 160. Ambos valores deben ser enteros positivos y su producto no debe
exceder el límite de tamaño del frame. El número de workers no puede ser mayor
que `width × height`.

## Modelos y configuración

| Modelo | Comportamiento |
|---|---|
| `sequential` | Procesa los píxeles en orden; baseline con un worker. |
| `fgmt` | Contextos comparten un pipeline y rotan en cada quantum. |
| `cgmt` | Contextos comparten un pipeline y ceden al detectar stalls. |
| `smt` | Simula dos slots de emisión y contextos virtuales; no crea hilos del SO. |
| `cmp` | Procesa rangos de píxeles en paralelo con threads del sistema operativo. |

`--workers N` configura FGMT, CGMT y CMP, y la cantidad de contextos virtuales
SMT. No cambia el modelo secuencial. Si se omite, cada modelo usa su valor
predeterminado; SMT calcula sus contextos a partir de los procesadores lógicos
detectados. Al ejecutar un solo modelo paralelo también se mide el baseline
secuencial para calcular el speedup.

## Barrido experimental y gráficas

El script ejecuta el benchmark para distintos conteos de workers, recoge las
muestras por frame y genera CSV consolidados y gráficas. Las mediciones finales
deben ejecutarse en hardware físico. El script rechaza WSL y contenedores Linux
conocidos; la confirmación requerida no detecta todas las máquinas virtuales.
Matplotlib debe estar instalado (`python -m pip install matplotlib`).

Desde esta carpeta:

```powershell
# Resolución predeterminada (240 x 160)
python scripts/run_worker_sweep.py --workers 1 2 4 8 --runs 200 --confirm-physical-host

# Definir ancho y alto en tiempo de ejecución (640 x 360)
python scripts/run_worker_sweep.py --width 640 --height 360 --workers 1 2 4 8 --runs 200 --confirm-physical-host
```

`--width` y `--height` se pasan a cada ejecución del benchmark. El script
permite también `--benchmark RUTA` para indicar otro ejecutable y
`--output-dir RUTA` para cambiar la carpeta de resultados. El valor mínimo de
`--runs` es 200. Los conteos de workers deben estar entre 1 y el total de
píxeles de la resolución elegida.

Por defecto, los resultados se guardan en `data/worker_sweep/`:

| Archivo | Contenido |
|---|---|
| `worker_sweep_summary.csv` | Métricas resumidas y speedup virtual por modelo y configuración. |
| `worker_sweep_frames.csv` | Muestras de cada frame, incluyendo dimensiones y workers. |
| `convergence.csv` | Media, desviación estándar e intervalo de confianza por configuración. |
| `speedup_vs_workers.png` | Speedup virtual e intervalos de confianza. |
| `virtual_time_distributions.png` | Distribuciones del tiempo virtual. |
| `convergence.png` | Evolución de medias acumuladas por número de ejecuciones. |

El speedup virtual se calcula como la media del tiempo virtual secuencial
dividida por la media del modelo. No representa el speedup de tiempo de pared;
para este último, consulta `speedup` en el CSV resumen del benchmark. El script
indica si el intervalo normal aproximado del 95 % supera el 5 % de la media;
si lo supera, aumenta `--runs` y repite la campaña.

## Archivos de salida del benchmark

Por defecto, el benchmark solo genera el CSV resumen indicado por `--output`
(predeterminado: `data/benchmark.csv`). Con `--frame-csv` también genera un CSV
de muestras por frame para el baseline secuencial y los modelos seleccionados.
Los archivos se nombran agregando el modelo al nombre base, por ejemplo:

```text
data/benchmark.csv
data/benchmark_sequential_frames.csv
data/benchmark_fgmt_frames.csv
data/benchmark_cgmt_frames.csv
data/benchmark_smt_frames.csv
data/benchmark_cmp_frames.csv
```

Las métricas incluyen tiempo de pared, tiempo virtual, stalls y cambios de
contexto. El tiempo de pared mide `render_frame()`, no la escritura de CSV ni
la generación del GIF. El tiempo virtual y los stalls corresponden a las reglas
simuladas de cada modelo, por lo que no son equivalentes al tiempo de pared.

Con `--gif`, el benchmark crea `data/camera_orbit.gif` con 72 frames de cámara
orbital; los PPM intermedios quedan en `data/camera_orbit_frames/`. El GIF se
genera después de medir y usa CMP. Sin `--gif`, no se crean estos archivos.

## Renderizar imágenes PPM

El ejecutable `raytracing_visual` renderiza los cinco modelos por defecto y
guarda un PPM por modelo en `data/`. A diferencia del benchmark, este CLI no
acepta opciones `--width`/`--height`; sus dimensiones son las configuradas por
defecto en el código.

```powershell
# Renderizar todos los modelos
./build-msys2/raytracing_visual.exe

# Renderizar solo CMP con 8 workers
./build-msys2/raytracing_visual.exe --model cmp --workers 8 --output data/frame_cmp.ppm

# Consultar opciones
./build-msys2/raytracing_visual.exe --help
```

`--model` acepta `all`, `sequential`, `fgmt`, `cgmt`, `smt` o `cmp`.
Los nombres de salida relativos se interpretan desde el directorio de trabajo
actual.

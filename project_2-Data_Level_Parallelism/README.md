# Proyecto 2 — Data-Level Parallelism: Ray Tracing

Framework experimental de paralelismo a nivel de datos aplicado al problema de Ray Tracing,
implementado con cuatro estrategias: secuencial (baseline), SIMD (AVX2), GPU (CUDA) y FPGA.

## Estructura del proyecto

```
project_2-Data_Level_Parallelism/
├── docs/                   Documentación técnica: diagramas, decisiones de diseño, etc
├── shared/                 Utilidades y configuración compartidas por todas las estrategias
│   ├── include/
│   │   ├── geometry/       Tipos base: Ray, Vector3, Sphere, Scene
│   │   ├── utils/          Timer, Metrics, image_io
│   │   └── config/         Parámetros globales del ray tracer (resolución, profundidad de rayo, etc.)
│   └── src/
│       └── utils/          Implementaciones de utilidades compartidas
├── ray_tracing/            Benchmark principal: todas las estrategias DLP sobre el mismo problema
│   ├── include/core/
│   │   ├── strategies/     Interfaces y cabeceras de cada renderer (Sequential, SIMD, CUDA, FPGA)
│   │   └── camera/         Lógica de cámara
│   ├── src/
│   │   ├── core/           Implementaciones .cpp de cada renderer y punto de entrada del benchmark
│   │   └── cuda/           Kernel CUDA (.cu), compilado con nvcc
│   └── data/               CSVs y resultados generados en tiempo de ejecución
├── fpga/                   Acelerador FPGA
│   ├── isa/                Especificación del ISA mínimo del acelerador (opcodes, registros, encoding)
│   ├── rtl/
│   │   ├── src/            Módulos RTL en Verilog/VHDL
│   │   └── top/            Wrapper top-level (pines, PLL, glue logic)
│   ├── hls/                Fuentes HLS (Vivado HLS / Intel HLS), si se utiliza
│   ├── constraints/        Restricciones de timing y asignación de pines (.qsf / .xdc)
│   ├── sim/                Testbenches de simulación RTL
│   └── scripts/            Scripts de síntesis, place & route y generación de bitstream
├── results/                Archivos de salida de ejecuciones (imágenes, CSVs, logs de profiling)
├── scripts/                Scripts de automatización de ejecuciones y barridos de parámetros
└── paper/                  Artículo en formato IEEE Transactions
```

## Compilación y ejecución

### Compilación

Ejecuta los comandos desde la raíz de `project_2-Data_Level_Parallelism/`.
Usa solo una de estas opciones para configurar y compilar.

#### Opción A: Ninja

Requiere Ninja instalado. El preset `benchmark` lo configura:

```powershell
cmake --preset benchmark
cmake --build --preset benchmark
```

El ejecutable queda en `build/ray_tracing/rt_benchmark.exe`.

#### Opción B: MinGW Makefiles

Requiere w64devkit. Si está instalado en otra ruta, ajusta `PATH` y
`CMAKE_CXX_COMPILER`:

```powershell
$env:PATH = "C:\raylib\w64devkit\bin;$env:PATH"
cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="C:/raylib/w64devkit/bin/g++.exe" -DCMAKE_BUILD_TYPE=Release
cmake --build build-mingw
```

El ejecutable queda en `build-mingw/ray_tracing/rt_benchmark.exe`.

### Uso

```
rt_benchmark [--width W] [--height H] [--runs N] [--warmup N]
             [--model sequential|simd|all] [--output results/benchmark.csv]
             [--image results/frame.ppm]
```

| Parametro | Descripción | Default |
|--------|-------------|---------|
| `--width W` | Ancho de la imagen en píxeles | 100 |
| `--height H` | Alto de la imagen en píxeles | 100 |
| `--model NAME` | Estrategia: `sequential`, `simd` (AVX2) o `all` | `sequential` |
| `--runs N` | Número de ejecuciones medidas | 10 |
| `--warmup N` | Ejecuciones descartadas antes de medir | 1 |
| `--output PATH` | Ruta base para CSVs; con `all`, se usa su directorio | `results/benchmark.csv` |
| `--image PATH` | Guarda el último frame como PPM; con `all`, genera un archivo por modelo | (desactivado) |

La variable paralelizable son los `rayos (N) = width × height`. Por lo tanto, para evaluar escalabilidad se recomienda alterar el valor de `--width` y `--height` entre ejecuciones.

La estrategia `simd` procesa cuatro rayos por bloque con AVX2. Requiere un CPU
compatible con AVX2 y deja los píxeles sobrantes al final de cada fila al kernel
escalar. La estrategia secuencial continúa siendo la opción predeterminada.
Consulta la [guía de intrinsics AVX](docs/simd_intrinsics.md) para ver cómo
funciona cada instrucción vectorial usada por el renderer.

`--model all` ejecuta todos los modelos disponibles en el benchmark (actualmente
`sequential` y `simd`). Cada modelo conserva su último frame medido; al terminar,
el programa compara el color RGB de cada píxel entre modelos y falla si encuentra
alguna diferencia.

Por defecto, cada modelo escribe su CSV en `results/<modelname>_benchmark.csv`
(por ejemplo `results/sequential_benchmark.csv` y `results/simd_benchmark.csv`).
También crea una copia con timestamp en `results/log/`. Si se pasa `--output`,
para un solo modelo se usa esa ruta exacta; con `all`, se usa el directorio de
esa ruta y se conserva el nombre `<modelname>_benchmark.csv`.

### Ejemplos de ejecución

Ejecuta los comandos desde la raíz de `project_2-Data_Level_Parallelism/`.
El argumento `--model all` corre `sequential` y `simd`, guarda un CSV por
modelo y compara los colores de los últimos frames.

#### Con Ninja

Todos los modelos (1000 × 1000, 10 ejecuciones):

```powershell
.\build\ray_tracing\rt_benchmark.exe --model all --width 1000 --height 1000 --runs 10 --warmup 10
```

#### 200 ejecuciones y PPM por modelo

```powershell
.\build\ray_tracing\rt_benchmark.exe --model all --width 100 --height 100 --runs 200 --warmup 10 --image results\frame.ppm
```

Prueba de correctitud:

```powershell
ctest --test-dir build --output-on-failure
```

#### Con MinGW Makefiles

Todos los modelos (1000 × 1000, 10 ejecuciones):

```powershell
.\build-mingw\ray_tracing\rt_benchmark.exe --model all --width 1000 --height 1000 --runs 10 --warmup 10
```

200 ejecuciones y un PPM por modelo:

```powershell
.\build-mingw\ray_tracing\rt_benchmark.exe --model all --width 1000 --height 1000 --runs 200 --warmup 10 --image results\frame.ppm
```

Prueba de correctitud:

```powershell
ctest --test-dir build-mingw --output-on-failure
```

Los comandos de 200 ejecuciones guardan los últimos frames como
`results/frame_sequential.ppm` y `results/frame_simd.ppm`. Los CSV quedan en
`results/sequential_benchmark.csv` y `results/simd_benchmark.csv`.


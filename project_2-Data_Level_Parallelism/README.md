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

## Compilación y ejecución secuencial

### Compilación

Desde la raíz del proyecto (`project_2-Data_Level_Parallelism/`):

```powershell
cmake --preset benchmark
```

O bien,

```powershell
cmake --build --preset benchmark
```

El ejecutable queda en `build/ray_tracing/rt_benchmark.exe`.

### Uso

```
rt_benchmark [--width W] [--height H] [--runs N] [--warmup N]
             [--output results/benchmark.csv] [--image results/frame.ppm]
```

| Parametro | Descripción | Default |
|--------|-------------|---------|
| `--width W` | Ancho de la imagen en píxeles | 100 |
| `--height H` | Alto de la imagen en píxeles | 100 |
| `--runs N` | Número de ejecuciones medidas | 10 |
| `--warmup N` | Ejecuciones descartadas antes de medir | 1 |
| `--output PATH` | Ruta del CSV de resultados | `results/benchmark.csv` |
| `--image PATH` | Guarda el último frame renderizado como PPM | (desactivado) |

La variable paralelizable son los `rayos (N) = width × height`. Por lo tanto, para evaluar escalabilidad se recomienda alterar el valor de `--width` y `--height` entre ejecuciones.

Cada ejecución produce dos archivos:
- `results/benchmark.csv` — sobreescrito con los resultados más recientes.
- `results/log/execution_YYYYMMDD_HHMMSS.csv` — copia permanente con timestamp.

### Ejemplos de ejecución

Corridas con `--runs 10 --warmup 10`, ejecutadas desde la raíz del proyecto:

#### 100 × 100 (10 000 rayos)

```powershell
.\build\ray_tracing\rt_benchmark.exe --width 100 --height 100 --runs 10 --warmup 10
```

#### 1000 × 1000 (1 000 000 rayos)

```powershell
.\build\ray_tracing\rt_benchmark.exe --width 1000 --height 1000 --runs 10 --warmup 10
```

#### 10000 × 10000 (100 000 000 rayos)

```powershell
.\build\ray_tracing\rt_benchmark.exe --width 10000 --height 10000 --runs 10 --warmup 10
```


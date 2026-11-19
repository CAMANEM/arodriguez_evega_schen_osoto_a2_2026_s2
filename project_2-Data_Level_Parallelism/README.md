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

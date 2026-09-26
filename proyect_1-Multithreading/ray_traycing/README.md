# Ray tracing benchmark

Subproyecto C++17 para ejecutar y comparar cinco esquemas de ejecución con una
imagen de 80 x 60 píxeles. El renderer, el benchmark y el exportador de imágenes
están dentro de esta carpeta; el benchmark reutiliza la interfaz de métricas
compartida en `../shared`.

`include/core/raytracing_config.hpp` centraliza la geometría y los colores de la
escena, la cámara, la resolución y los parámetros de ejecución. El benchmark usa
`Timer` para medir cada frame en milisegundos y convierte las muestras a segundos
antes de entregarlas a la interfaz común de métricas.

La configuración de escena y de ejecución está centralizada en
`include/core/raytracing_config.hpp`; el benchmark mide cada frame con `Timer` y
convierte sus milisegundos a segundos para las métricas CSV.

## Requisitos

- CMake 3.21 o posterior para usar los presets; el proyecto sin presets conserva
	un mínimo de 3.10.
- Compilador C++17 (por ejemplo, MinGW-w64 GCC, GCC, Clang o Visual C++).
- Soporte de threads del sistema, detectado por CMake.

## Compilación

Ejecuta los comandos desde esta carpeta. En Windows con MSYS2 MinGW-w64:

```powershell
cmake --preset msys2-mingw64
cmake --build --preset msys2-mingw64 --parallel
```

El preset selecciona explícitamente `C:/msys64/mingw64/bin/g++.exe` y
`C:/msys64/mingw64/bin/mingw32-make.exe`. Si MSYS2 está instalado en otra ruta,
actualiza ambas rutas en `CMakePresets.json`. Para Visual Studio, también se
puede usar el generador predeterminado de CMake:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

En Linux o macOS, si CMake encuentra el compilador C++17:

```sh
cmake -S . -B build
cmake --build build --parallel
```

Con el preset, los ejecutables quedan en `build-msys2/`. Con Visual Studio,
quedan dentro de la carpeta de configuración, por ejemplo `build/Release/`.
Si cambias de compilador o generador, configura en otro directorio de build
para evitar reutilizar una configuración anterior.

## Esquemas de ejecución

| Nombre | Comportamiento |
| --- | --- |
| `sequential` | Un worker procesa los píxeles en orden. Es el baseline. |
| `fgmt` | Cuatro contextos comparten un pipeline; el turno rota entre contextos. |
| `cgmt` | Cuatro contextos comparten un pipeline; el scheduler cede el turno al detectar un stall. |
| `smt` | Simula emisión de ancho 2 con `max(1, hardware_concurrency()) × 2` contextos virtuales, limitados al total de píxeles; no crea OS threads. |
| `cmp` | Cuatro workers con threads del sistema operativo y trabajo dividido por core. |

FGMT y CGMT simulan un pipeline compartido. SMT sobre-suscribe los contextos
virtuales según los procesadores lógicos detectados y un factor fijo de 2; no
lanza threads del sistema operativo. En el CSV, `n_workers` para SMT representa
ese número de contextos virtuales, no hilos del sistema operativo. CMP usa
paralelismo real del sistema operativo, por lo que sus tiempos de pared también
incluyen diferencias de scheduler y creación/sincronización de threads.

## Benchmark

Desde PowerShell, ejecuta los cinco esquemas con ray tracing, cinco repeticiones
por esquema:

```powershell
./build-msys2/raytracing_benchmark.exe --model all --workload raytracing --runs 5
```

Ejecuta la carga dummy con los mismos esquemas:

```powershell
./build-msys2/raytracing_benchmark.exe --model all --workload dummy --runs 5
```

Opciones:

| Opción | Valores / valor predeterminado | Descripción |
| --- | --- | --- |
| `--model` | `all` (predeterminado), `sequential`, `fgmt`, `cgmt`, `smt`, `cmp` | Ejecuta todos los modelos o uno. |
| `--workload` | `raytracing` (predeterminado), `dummy` | Selecciona el cálculo de cada píxel. |
| `--runs` | `5` | Número de repeticiones; debe ser positivo. |
| `--output` | `results/benchmark.csv` | Ruta del CSV. Se crean los directorios necesarios. |
| `--help`, `-h` | | Muestra el uso del programa. |

Cuando se elige un solo modelo paralelo, el benchmark también ejecuta el modelo
secuencial para obtener el baseline, aunque el CSV solo incluya el modelo
solicitado. Speedup y eficiencia se calculan usando los promedios de tiempo de
pared del mismo workload; eficiencia divide el speedup entre `n_workers`.
Cada muestra mide solo `render_frame()`, no la escritura del CSV.

El CSV contiene `model`, `workload`, `n_workers`, `runs`, promedio y desviación
estándar del tiempo en segundos, límites del intervalo de confianza del 95 %,
speedup y eficiencia. Con una sola repetición, la desviación estándar es cero.
Estos son tiempos de pared: no son los relojes virtuales internos de los
schedulers.

La carga `dummy` omite la intersección de rayos y genera un patrón de color
determinista. Conserva el recorrido de píxeles y la lógica de scheduler/cache
de cada esquema, para poder probarlos sin el cálculo geométrico. No representa
una carga ray tracing equivalente en trabajo de CPU.

Los nombres de salida son relativos al directorio de trabajo desde el que se
ejecuta el programa.

Para repetir la verificación secuencial y guardar sus resultados en `data/`:

```powershell
./build-msys2/raytracing_benchmark.exe --model sequential --workload raytracing --runs 5 --output data/sequential_raytracing.csv
./build-msys2/raytracing_visual.exe --model sequential --workload raytracing --output data/sequential_raytracing.ppm
```

## Exportar una imagen

`raytracing_visual` renderiza un frame con el modelo secuencial por defecto y
lo escribe en formato PPM P3. No abre una ventana; usa un visor compatible con
PPM para ver el archivo.

```powershell
./build-msys2/raytracing_visual.exe --model sequential --workload raytracing --output results/frame.ppm
```

También se puede seleccionar otro esquema o el workload dummy:

```powershell
./build-msys2/raytracing_visual.exe --model cmp --workload dummy --output results/frame_dummy.ppm
```

`--model` acepta `sequential`, `fgmt`, `cgmt`, `smt` o `cmp`. `--workload`
acepta `raytracing` o `dummy`; ambos son opcionales. `--output` establece la
ruta de salida (predeterminada: `results/frame.ppm`). `--help` o `-h` muestra
el uso del programa.

Los resultados generados de la verificación secuencial se guardan en `data/`;
los artefactos de compilación permanecen en `build-msys2/`.
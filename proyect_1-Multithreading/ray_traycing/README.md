# Ray tracing benchmark

Subproyecto C++17 para ejecutar y comparar cinco esquemas de ejecución con una
imagen de 80 x 60 píxeles. El renderer, el benchmark y el exportador de imágenes
están dentro de esta carpeta; el benchmark reutiliza la interfaz de métricas
compartida en `../shared`.

`include/core/config/raytracing_config.hpp` centraliza la geometría y los colores
de la escena, la cámara, la resolución y los parámetros de ejecución. El benchmark usa
`Timer` para medir cada frame en milisegundos y convierte las muestras a segundos
antes de entregarlas a la interfaz común de métricas.

## Requisitos

- CMake 3.21 o posterior para usar los presets; el proyecto sin presets conserva
  un mínimo de 3.10.
- Compilador C++17 (por ejemplo, MinGW-w64 GCC, GCC, Clang o Visual C++).
- Soporte de threads del sistema, detectado por CMake.
- Python 3 y Pillow para generar el GIF (`python -m pip install Pillow`).

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

### Limpiar y recompilar (Windows / MSYS2)

Ejecuta estos comandos desde `ray_traycing/`. Para limpiar solo los artefactos
de compilación y conservar la configuración de CMake:

```powershell
cmake --build --preset msys2-mingw64 --target clean
cmake --build --preset msys2-mingw64 --parallel
```

Para borrar por completo el directorio generado y configurar una compilación
nueva, elimina solo `build-msys2/`; los datos de `data/` no se borran:

```powershell
Remove-Item -Recurse -Force build-msys2 -ErrorAction SilentlyContinue
cmake --preset msys2-mingw64
cmake --build --preset msys2-mingw64 --parallel
```

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

Desde PowerShell, ejecuta los cinco esquemas con ray tracing, 200 repeticiones
por esquema (el mínimo de referencia de la evaluación):

```powershell
./build-msys2/raytracing_benchmark.exe --model all --runs 200 --output data/benchmark_raytracing.csv
```

Opciones:

| Opción | Valores / valor predeterminado | Descripción |
| --- | --- | --- |
| `--model` | `all` (predeterminado), `sequential`, `fgmt`, `cgmt`, `smt`, `cmp` | Ejecuta todos los modelos o uno. |
| `--runs` | `200` | Número de repeticiones por modelo; debe ser positivo. |
| `--output` | `data/benchmark.csv` | Ruta del CSV. Se crean los directorios necesarios. |
| `--no-gif` | | Omite la generación predeterminada del GIF. |
| `--help`, `-h` | | Muestra el uso del programa. |

El argumento `--output` nombra el CSV resumen. Además, se crea un CSV por
modelo con una fila por frame medido; por ejemplo, para
`data/benchmark_raytracing.csv` se generan
`data/benchmark_raytracing_sequential_frames.csv`,
`data/benchmark_raytracing_fgmt_frames.csv`,
`data/benchmark_raytracing_cgmt_frames.csv`,
`data/benchmark_raytracing_smt_frames.csv` y
`data/benchmark_raytracing_cmp_frames.csv`. Si se solicita un solo modelo
paralelo, también se genera el CSV secuencial usado como baseline.

Cada CSV por modelo contiene el tiempo de pared del frame, el tiempo virtual,
latencia virtual atribuida a stalls, cantidad de misses/stalls y cambios de
contexto simulados. Secuencial y CMP pagan la latencia completa de miss
(3200 ns por miss); FGMT cuenta un quantum desperdiciado (1000 ns por miss);
CGMT registra el costo de cambio de contexto (400 ns por miss; la latencia del
miss se considera oculta); SMT registra la latencia modelada (3200 ns por miss).
Los cambios de contexto son transferencias entre contextos distintas dentro
del scheduler simulado: FGMT al rotar a otro worker, CGMT al ceder/terminar un
tile y SMT cuando un miss expulsa un contexto. Secuencial y CMP reportan cero,
porque no se instrumentan cambios del scheduler del sistema operativo.

El CSV resumen se calcula a partir de las muestras de los CSV por modelo.
Incluye media, desviación estándar muestral e intervalo de confianza normal
aproximado del 95 % para el tiempo de ejecución y el tiempo virtual de stall,
además de medias/desviaciones para cantidad de stalls y cambios de contexto.
El speedup usa la razón entre el promedio secuencial y el promedio del modelo;
su intervalo del 95 % usa propagación de error para dos medias independientes.
La eficiencia es `speedup / n_workers`. `virtual_speedup` compara los tiempos
virtuales medios. El tiempo de pared mide solo `render_frame()`, no escritura
de CSV ni generación del GIF.

Al elegir un único modelo paralelo, también se ejecuta la campaña secuencial
completa para construir el baseline, aunque solo se incluya el modelo elegido
en el CSV resumen.

Al terminar, el benchmark crea `data/camera_orbit.gif`: 72 frames de la
cámara orbitando en sentido horario sobre un círculo de radio 8 en el plano XZ,
alrededor del centro de la escena. Los
frames PPM intermedios quedan en `data/camera_orbit_frames/`; la animación se
genera después de las mediciones y no afecta sus tiempos. Usa `--no-gif` para
omitir este paso. El GIF usa el modelo CMP.

Los nombres de salida son relativos al directorio de trabajo desde el que se
ejecuta el programa.

Para repetir la verificación secuencial y guardar sus resultados en `data/`:

```powershell
./build-msys2/raytracing_benchmark.exe --model sequential --runs 200 --output data/sequential_raytracing.csv
./build-msys2/raytracing_visual.exe --model sequential --output data/sequential_raytracing.ppm
```

## Renderizar modelos

`raytracing_visual` ejecuta los cinco modelos por defecto, guarda un PPM por
modelo y genera el GIF de órbita de cámara descrito arriba. No abre una ventana;
usa un visor compatible con PPM para ver las imágenes.

```powershell
./build-msys2/raytracing_visual.exe
```

Cada salida recibe el nombre `data/frame_<modelo>.ppm`. Se puede ejecutar un
solo modelo y elegir su archivo de salida:

```powershell
./build-msys2/raytracing_visual.exe --model cmp --output data/frame_cmp.ppm --no-gif
```

`--model` acepta `all`, `sequential`, `fgmt`, `cgmt`, `smt` o `cmp`. Con
`--model all`, `--output` se usa como prefijo para generar un archivo por modelo.
`--no-gif` omite el GIF y `--help` muestra el uso del programa.

Todos los CSV, PPM, GIF y frames intermedios se guardan en `data/`; los artefactos
de compilación permanecen en `build-msys2/`.
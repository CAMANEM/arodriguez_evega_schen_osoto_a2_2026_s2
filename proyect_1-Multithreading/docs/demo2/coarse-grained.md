# Coarse-Grained Multithreading — Boids (BOIDS-CG-001)

Implementación directa con hilos reales del SO. El cambio de contexto observable
se asocia a **bloqueos costosos** (stalls didácticos inyectados y `join()`), no a
round-robin por quantum (eso es Fine-Grained).

## Mapeo teoría → Boids

| Concepto | Implementación |
|---|---|
| Hilo tradicional | `std::thread` |
| Trabajo de grano grueso | Bloque `[start, end)` de boids |
| Stall / bloqueo costoso | `sleep_for` inyectado + `join()` final |
| Contexto real (SO) | Stack/PCB preservados **implícitamente** al bloquear |
| Contexto didáctico | `CoarseWorkerCheckpoint` guardado/restaurado **manualmente** |
| Unidad que no se parte | Un boid completo |

## stall en frontera de boid

```mermaid
flowchart TB
    S["Worker en su bloque"] --> C["Calcular boid i COMPLETO"]
    C --> W["Escribir steeringForces[i]"]
    W --> Q{"¿Stall? cada K o probabilidad p"}
    Q -->|No| N["i = i + 1"]
    N --> T{"¿Quedan boids?"}
    T -->|Sí| C
    T -->|No| End["Fin del worker"]
    Q -->|Sí| Save["1. Checkpoint MANUAL"]
    Save --> Block["2. sleep/wait — SO guarda contexto IMPLÍCITO"]
    Block --> Restore["3. Restore MANUAL"]
    Restore --> N
```

## Fine vs Coarse

```mermaid
flowchart TB
    subgraph fine["Fine-Grained"]
        F1["1 hilo SO"]
        F2["Contextos virtuales"]
        F3["Cede SIEMPRE cada 1 vecino"]
        F1 --> F2 --> F3
    end
    subgraph coarse["Coarse-Grained"]
        C1["T hilos SO reales"]
        C2["Bloques grandes"]
        C3["Corre boids completos hasta STALL"]
        C4["Checkpoint manual + bloqueo"]
        C1 --> C2 --> C3 --> C4
    end
```

## Frase clave para defensa

> En un sistema real, al bloquearse el hilo el sistema operativo conserva
> automáticamente su contexto (stack, registros, PCB). Este checkpoint manual
> no sustituye ese mecanismo: lo **hace visible** para demostración académica
> del save/resume ante un stall costoso.

## CLI

```text
boids --scheme coarse
      [--workers T]
      [--stall-every K]
      [--stall-probability P]
      [--stall-ms X]
      [--stall-ms-min A --stall-ms-max B]
      [--log-checkpoints]
      [--boids N] [--steps N|--forever] [--seed S]
      [--gui|--no-gui] [--validate]
```

- **Sin stalls** (`--stall-every 0` y probabilidad 0): baseline de speedup coarse.
- **Con stalls**: demo académica; el resultado numérico sigue siendo equivalente al secuencial.
- Disparo: `(cada K) OR (sorteo P)`. Duración: rango min/max si ambos se pasan; si no, `--stall-ms`.

## Ejemplos

```bash
# Speedup sin stalls didácticos
boids --scheme coarse --workers 4 --boids 200 --steps 100 --validate

# Demo con stalls observables + traza de checkpoint
boids --scheme coarse --workers 2 --stall-every 5 --stall-ms 10 --log-checkpoints --boids 40 --steps 1

# Stall aleatorio reproducible
boids --scheme coarse --stall-probability 0.2 --stall-ms-min 1 --stall-ms-max 5 --seed 7 --validate
```

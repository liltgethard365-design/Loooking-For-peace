# TurboPalmTree - Real-time Console Automation System

Deterministic, high-precision timing automation for NBA 2K26 green meter shooting.

## Architecture

```
Capture Card → Ring Buffer (zero-copy) → Vision → Logic → Synthesis → Console
```

## Current Phase: Hardware-to-Software Bridge

### Sprint 1 Priorities

1. **Latency Benchmarking** (Priority #1) - In Progress
   - Round-trip measurement: Capture → Vision → Logic → Injection
   - Target: P95 < 20ms for consistent "perfect" timing

2. **Thread Pinning** (Priority #2)
   - Pin worker threads to specific physical cores
   - Eliminate OS scheduler jitter

3. **Vision-to-Engine Integration** (Priority #3)
   - Zero-copy frame passing (pointers, not copies)
   - Ring buffer for non-blocking delivery

4. **Calibration Loop** (Priority #4)
   - State machine: IDLE → CALIBRATING → READY → AIMING
   - Auto-profile green window characteristics

## Building

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

## Running Benchmarks

```bash
./latency_benchmark
```

Expected: P95 latency < 20ms

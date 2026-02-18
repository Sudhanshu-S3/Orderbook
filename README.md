# Orderbook

A limit order book implementation in C++20 supporting order matching, cancellation, and modification.

## Features

- **Order types**: GoodTillCancel, FillAndKill, Market
- **Operations**: Add, Cancel, Modify orders
- **Matching engine**: Price-time priority (bids sorted descending, asks ascending)
- **Level info**: Query aggregated bid/ask depth

## Project Structure

```
Orderbook/
├── include/
│   ├── Types.h          # Core types (Price, Quantity, OrderId, Side, OrderType)
│   ├── Order.h          # Order class and smart pointer types
│   ├── OrderModify.h    # Order modification request
│   ├── Trade.h          # Trade result type
│   ├── LevelInfo.h      # Price level aggregation
│   └── Orderbook.h      # Main Orderbook class
├── src/
│   ├── Order.cpp        # Order implementation
│   ├── Orderbook.cpp    # Orderbook matching engine
│   └── main.cpp         # Entry point / tests
├── benchmark/
│   └── benchmark.cpp    # Latency and throughput benchmarks
└── CMakeLists.txt
```

## Build

Requires CMake 3.20+ and a C++20-capable compiler.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run

```bash
./build/orderbook
```

## Benchmark

```bash
./build/benchmark
```

Benchmarks cover:
- **Insertion throughput** — 100k orders with no matches
- **Match latency** — per-trade latency (p50/p95/p99)
- **Cancel latency** — per-cancel latency (p50/p95/p99)
- **Mixed workload** — 200k random insert/cancel/match operations

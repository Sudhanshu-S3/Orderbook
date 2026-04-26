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
│   └── main.cpp         # Entry point / smoke test
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

## Bug Fixes

- **Heap use-after-free in `CancelOrder` and `ModifyOrder`** — structured
  bindings (`const auto& [order, iterator] = orders_.at(...)`) bound
  references into the `unordered_map` node that was then erased. Reads
  through those references afterwards were undefined behavior. Fix copies
  the `shared_ptr` and iterator to locals before erasing, and folds
  `contains` + `at` + `erase(key)` into a single `find` + `erase(it)`.
- **O(n) iterator lookup in `AddOrder`** — `std::next(orders.begin(), orders.size() - 1)`
  walks the entire price-level list on every insert because `std::list`
  iterators are bidirectional, not random-access. Replaced with
  `std::prev(orders.end())`, which is O(1).
- **FillAndKill cleanup only inspected top-of-book** — the post-match
  cleanup peeked at the best price level's front order, missing FAK
  remainders elsewhere. Replaced with an id-based cancel of the incoming
  order's remainder, performed in `AddOrder` after matching.

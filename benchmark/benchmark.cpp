#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <random>
#include <format>
#include "../include/orderbook.h"

using Clock = std::chrono::high_resolution_clock;
using Ns    = std::chrono::nanoseconds;

static Ns ElapsedNs(Clock::time_point start, Clock::time_point end)
{
    return std::chrono::duration_cast<Ns>(end - start);
}

static void PrintLatencyStats(const std::string& label, std::vector<int64_t>& samples)
{
    std::sort(samples.begin(), samples.end());
    double avg = static_cast<double>(std::accumulate(samples.begin(), samples.end(), int64_t{0}))
                 / static_cast<double>(samples.size());
    int64_t p50 = samples[samples.size() * 50 / 100];
    int64_t p95 = samples[samples.size() * 95 / 100];
    int64_t p99 = samples[samples.size() * 99 / 100];

    std::cout << std::format("\n=== {} ===\n", label);
    std::cout << std::format("  Samples : {:>10}\n", samples.size());
    std::cout << std::format("  Avg     : {:>8.1f} ns\n", avg);
    std::cout << std::format("  p50     : {:>8} ns\n", p50);
    std::cout << std::format("  p95     : {:>8} ns\n", p95);
    std::cout << std::format("  p99     : {:>8} ns\n", p99);
}

// -----------------------------------------------------------------------
// Benchmark 1: Pure insertion throughput (no matches)
//   Buy orders priced 1–50, Sell orders priced 51–100
// -----------------------------------------------------------------------
static void BenchInsertThroughput()
{
    constexpr int N = 100'000;

    Orderbook orderbook;
    std::vector<OrderPointer> orders;
    orders.reserve(N);

    for (int i = 0; i < N; ++i)
    {
        if (i % 2 == 0)
            orders.push_back(std::make_shared<Order>(
                OrderType::GoodTillCancel, i, Side::Buy, 1 + (i % 50), 10));
        else
            orders.push_back(std::make_shared<Order>(
                OrderType::GoodTillCancel, i, Side::Sell, 51 + (i % 50), 10));
    }

    auto start = Clock::now();
    for (auto& o : orders)
        orderbook.AddOrder(o);
    auto end = Clock::now();

    double ms = ElapsedNs(start, end).count() / 1e6;
    double throughput = N / (ms / 1000.0);

    std::cout << std::format("\n=== Insertion Throughput (no match) ===\n");
    std::cout << std::format("  Inserted {:>10} orders in {:.2f} ms\n", N, ms);
    std::cout << std::format("  Throughput: {:.0f} orders/sec\n", throughput);
}

// -----------------------------------------------------------------------
// Benchmark 2: Match latency
//   Pre-populate book with buy orders, then insert crossing sell orders
//   and measure each individual AddOrder call
// -----------------------------------------------------------------------
static void BenchMatchLatency()
{
    constexpr int N = 50'000;

    std::vector<int64_t> samples;
    samples.reserve(N);

    for (int i = 0; i < N; ++i)
    {
        Orderbook orderbook;

        // Resting buy
        orderbook.AddOrder(std::make_shared<Order>(
            OrderType::GoodTillCancel, 0, Side::Buy, 100, 10));

        // Crossing sell — this triggers the match
        auto sell = std::make_shared<Order>(
            OrderType::GoodTillCancel, 1, Side::Sell, 100, 10);

        auto t0 = Clock::now();
        orderbook.AddOrder(sell);
        auto t1 = Clock::now();

        samples.push_back(ElapsedNs(t0, t1).count());
    }

    PrintLatencyStats("Match Latency", samples);
}

// -----------------------------------------------------------------------
// Benchmark 3: Cancel latency
//   Pre-populate book then measure each CancelOrder call
// -----------------------------------------------------------------------
static void BenchCancelLatency()
{
    constexpr int N = 50'000;

    std::vector<int64_t> samples;
    samples.reserve(N);

    for (int i = 0; i < N; ++i)
    {
        Orderbook orderbook;

        // Insert an order, then measure how long it takes to cancel it
        orderbook.AddOrder(std::make_shared<Order>(
            OrderType::GoodTillCancel, 42, Side::Buy, 100, 10));

        auto t0 = Clock::now();
        orderbook.CancelOrder(42);
        auto t1 = Clock::now();

        samples.push_back(ElapsedNs(t0, t1).count());
    }

    PrintLatencyStats("Cancel Latency", samples);
}

// -----------------------------------------------------------------------
// Benchmark 4: Mixed workload (insert + cancel + match)
// -----------------------------------------------------------------------
static void BenchMixedWorkload()
{
    constexpr int N = 200'000;

    Orderbook orderbook;
    std::mt19937 rng{ 42 };
    std::uniform_int_distribution<int> priceDist{ 95, 105 };
    std::uniform_int_distribution<int> qtyDist  { 1, 20 };
    std::uniform_int_distribution<int> actionDist{ 0, 2 };

    OrderId nextId = 0;
    std::vector<OrderId> liveOrders;
    liveOrders.reserve(1024);

    auto start = Clock::now();

    for (int i = 0; i < N; ++i)
    {
        int action = actionDist(rng);

        if (action == 0 && !liveOrders.empty())
        {
            // Cancel a random live order
            int idx = rng() % liveOrders.size();
            orderbook.CancelOrder(liveOrders[idx]);
            liveOrders.erase(liveOrders.begin() + idx);
        }
        else
        {
            Side side  = (rng() % 2 == 0) ? Side::Buy : Side::Sell;
            Price price = static_cast<Price>(priceDist(rng));
            Quantity qty = static_cast<Quantity>(qtyDist(rng));

            auto trades = orderbook.AddOrder(std::make_shared<Order>(
                OrderType::GoodTillCancel, nextId, side, price, qty));

            // If it wasn't fully consumed keep track of it
            if (orderbook.Size() > 0 && trades.empty())
                liveOrders.push_back(nextId);

            ++nextId;
        }
    }

    auto end = Clock::now();
    double ms = ElapsedNs(start, end).count() / 1e6;
    double throughput = N / (ms / 1000.0);

    std::cout << std::format("\n=== Mixed Workload ===\n");
    std::cout << std::format("  {} ops in {:.2f} ms\n", N, ms);
    std::cout << std::format("  Throughput: {:.0f} ops/sec\n", throughput);
    std::cout << std::format("  Remaining orders in book: {}\n", orderbook.Size());
}

int main()
{
    std::cout << "Orderbook Benchmark\n";
    std::cout << "===================\n";

    BenchInsertThroughput();
    BenchMatchLatency();
    BenchCancelLatency();
    BenchMixedWorkload();

    std::cout << "\nDone.\n";
    return 0;
}

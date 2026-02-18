#pragma once

#include <cstdint>
#include <limits>

enum class OrderType
{
    GoodTillCancel,
    FillAndKill,
    Market
};

enum class Side
{
    Buy,
    Sell
};

using Price    = int32_t;
using Quantity = uint32_t;
using OrderId  = uint64_t;

struct Constants
{
    static constexpr Price InvalidPrice = std::numeric_limits<Price>::max();
};

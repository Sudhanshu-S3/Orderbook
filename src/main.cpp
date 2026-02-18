#include <iostream>
#include <format>
#include "../include/orderbook.h"

int main()
{
    Orderbook orderbook;

    const OrderId orderId1 = 1;
    const OrderId orderId2 = 2;
    const OrderId orderId3 = 3;
    const OrderId orderId4 = 4;

    // Add a GTC buy order at 100, qty 10
    auto trades = orderbook.AddOrder(std::make_shared<Order>(
        OrderType::GoodTillCancel, orderId1, Side::Buy, 100, 10));
    std::cout << std::format("After buy order: book size = {}\n", orderbook.Size());

    // Add a GTC sell order at 105 — no match (ask > bid)
    orderbook.AddOrder(std::make_shared<Order>(
        OrderType::GoodTillCancel, orderId2, Side::Sell, 105, 5));
    std::cout << std::format("After sell 105: book size = {}\n", orderbook.Size());

    // Add a sell at 100 — this matches the buy
    trades = orderbook.AddOrder(std::make_shared<Order>(
        OrderType::GoodTillCancel, orderId3, Side::Sell, 100, 10));
    std::cout << std::format("After matching sell 100: {} trade(s), book size = {}\n",
        trades.size(), orderbook.Size());

    for (const auto& trade : trades)
        std::cout << std::format("  Trade: bid order {} filled {} @ {}\n",
            trade.GetBidTrade().orderId_,
            trade.GetBidTrade().quantity_,
            trade.GetBidTrade().price_);

    // FillAndKill — no matching order → instantly rejected
    trades = orderbook.AddOrder(std::make_shared<Order>(
        OrderType::FillAndKill, orderId4, Side::Buy, 90, 5));
    std::cout << std::format("FillAndKill with no match: {} trade(s), book size = {}\n",
        trades.size(), orderbook.Size());

    // Cancel the remaining ask at 105
    orderbook.CancelOrder(orderId2);
    std::cout << std::format("After cancel: book size = {}\n", orderbook.Size());

    // Level info
    auto info = orderbook.GetOrderInfos();
    std::cout << std::format("Bid levels: {}, Ask levels: {}\n",
        info.GetBids().size(), info.GetAsks().size());

    return 0;
}

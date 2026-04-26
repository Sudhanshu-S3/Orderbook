#include <iostream>
#include <format>
#include <cassert>
#include "../include/Orderbook.h"

int main()
{
    Orderbook orderbook;

    const OrderId orderId1 = 1;
    const OrderId orderId2 = 2;
    const OrderId orderId3 = 3;
    const OrderId orderId4 = 4;
    const OrderId orderId5 = 5;
    const OrderId orderId6 = 6;
    const OrderId orderId7 = 7;

    // Add a GTC buy order at 100, qty 10
    orderbook.AddOrder(std::make_shared<Order>(
        OrderType::GoodTillCancel, orderId1, Side::Buy, 100, 10));
    assert(orderbook.Size() == 1);
    std::cout << std::format("After buy order: book size = {}\n", orderbook.Size());

    // Add a GTC sell at 105 — no match (ask > bid)
    orderbook.AddOrder(std::make_shared<Order>(
        OrderType::GoodTillCancel, orderId2, Side::Sell, 105, 5));
    assert(orderbook.Size() == 2);
    std::cout << std::format("After sell 105: book size = {}\n", orderbook.Size());

    // Add a sell at 100 — fully matches the resting buy (both removed)
    auto trades = orderbook.AddOrder(std::make_shared<Order>(
        OrderType::GoodTillCancel, orderId3, Side::Sell, 100, 10));
    assert(trades.size() == 1);
    assert(orderbook.Size() == 1);   // only the 105 ask remains
    std::cout << std::format("After matching sell 100: {} trade(s), book size = {}\n",
        trades.size(), orderbook.Size());

    for (const auto& trade : trades)
        std::cout << std::format("  Trade: bid order {} filled {} @ {}\n",
            trade.GetBidTrade().orderId_,
            trade.GetBidTrade().quantity_,
            trade.GetBidTrade().price_);

    // FillAndKill — no opposite-side liquidity → instantly rejected, book unchanged
    trades = orderbook.AddOrder(std::make_shared<Order>(
        OrderType::FillAndKill, orderId4, Side::Buy, 90, 5));
    assert(trades.empty());
    assert(orderbook.Size() == 1);
    std::cout << std::format("FillAndKill with no match: {} trade(s), book size = {}\n",
        trades.size(), orderbook.Size());

    // Cancel the resting ask at 105 — exercises the CancelOrder UAF fix path
    orderbook.CancelOrder(orderId2);
    assert(orderbook.Size() == 0);
    std::cout << std::format("After cancel: book size = {}\n", orderbook.Size());

    // ModifyOrder — exercises the ModifyOrder UAF fix path
    orderbook.AddOrder(std::make_shared<Order>(
        OrderType::GoodTillCancel, orderId5, Side::Buy, 100, 10));
    auto modifyTrades = orderbook.ModifyOrder(OrderModify(orderId5, Side::Buy, 102, 8));
    assert(modifyTrades.empty());          // re-rests, doesn't cross
    assert(orderbook.Size() == 1);
    std::cout << std::format("After modify (100->102): book size = {}\n", orderbook.Size());

    // Partial-fill FAK — buy 10 against only 4 of liquidity. Fills 4, cancels remainder.
    // Exercises the FAK-cleanup-by-id fix.
    orderbook.AddOrder(std::make_shared<Order>(
        OrderType::GoodTillCancel, orderId6, Side::Sell, 105, 4));
    trades = orderbook.AddOrder(std::make_shared<Order>(
        OrderType::FillAndKill, orderId7, Side::Buy, 105, 10));
    assert(trades.size() == 1);            // matched 4
    // After match: id5 buy@102 still resting; id6 sell fully consumed; id7 FAK remainder cancelled.
    assert(orderbook.Size() == 1);
    std::cout << std::format("After partial-fill FAK: {} trade(s), book size = {}\n",
        trades.size(), orderbook.Size());

    auto info = orderbook.GetOrderInfos();
    std::cout << std::format("Bid levels: {}, Ask levels: {}\n",
        info.GetBids().size(), info.GetAsks().size());

    std::cout << "All assertions passed.\n";
    return 0;
}

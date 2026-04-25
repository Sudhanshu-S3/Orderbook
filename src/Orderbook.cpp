#include <algorithm>
#include <numeric>
#include "../include/Orderbook.h"

bool Orderbook::CanMatch( Side side, Price price) const
{
    if ( side == Side::Buy )
    {
        if ( asks_.empty())
            return false;

        const auto& [bestAsk, _] = *asks_.begin();
        return price >= bestAsk;
    }
    else
    {
        if( bids_.empty())
            return false;

        const auto& [bestBid, _] = *bids_.begin();
        return price <= bestBid;
    }
}

Trades Orderbook::MatchOrders()
{
    Trades trades;
    trades.reserve(orders_.size());

    while( true )
    {
        if( bids_.empty() || asks_.empty())
            break;

        auto& [bidPrice, bids] = *bids_.begin();
        auto& [askPrice, asks] = *asks_.begin();

        if(bidPrice < askPrice)
            break;

        while(!bids.empty() && !asks.empty())
        {
            // Copy shared_ptrs — not references — so they stay alive after pop_front()
            auto bid = bids.front();
            auto ask = asks.front();

            Quantity quantity = std::min(bid->GetRemainingQuantity(), ask->GetRemainingQuantity());

            bid->Fill(quantity);
            ask->Fill(quantity);

            trades.push_back(Trade{
                TradeInfo{ bid->GetOrderId(), bid->GetPrice(), quantity },
                TradeInfo{ ask->GetOrderId(), ask->GetPrice(), quantity }
            });

            if(bid->IsFilled())
            {
                bids.pop_front();
                orders_.erase(bid->GetOrderId());
            }

            if(ask->IsFilled())
            {
                asks.pop_front();
                orders_.erase(ask->GetOrderId());
            }
        }

        // Erase price levels outside the inner loop to avoid dangling map references
        if(bids.empty())
            bids_.erase(bidPrice);

        if(asks.empty())
            asks_.erase(askPrice);
    }

    return trades;
}

Trades Orderbook::AddOrder( OrderPointer order)
{
    if (orders_.contains(order->GetOrderId()))
        return { };

    if (order->GetOrderType() == OrderType::FillAndKill && !CanMatch(order->GetSide(), order->GetPrice()))
        return { };

    OrderPointers::iterator iterator;

    if(order->GetSide() == Side::Buy)
    {
        auto& orders = bids_[order->GetPrice()];
        orders.push_back(order);
        iterator = std::prev(orders.end());
    }
    else
    {
        auto& orders = asks_[order->GetPrice()];
        orders.push_back(order);
        iterator = std::prev(orders.end());
    }

    orders_.insert({ order->GetOrderId(), OrderEntry{ order, iterator }});

    auto trades = MatchOrders();

    // FillAndKill: cancel any unfilled remainder of THIS order, by id.
    // (Replaces the old "peek at best level" cleanup, which missed FAKs not at top-of-book.)
    if (order->GetOrderType() == OrderType::FillAndKill && orders_.find(order->GetOrderId()) != orders_.end())
        CancelOrder(order->GetOrderId());

    return trades;
}

void Orderbook::CancelOrder(OrderId orderId)
{
    auto it = orders_.find(orderId);
    if (it == orders_.end())
        return;

    OrderPointer order        = it->second.order_;       // copy: keeps Order alive
    OrderPointers::iterator location = it->second.location_;  // copy: by value
    orders_.erase(it);

    auto price = order->GetPrice();

    if (order->GetSide() == Side::Sell)
    {
        auto& orders = asks_.at(price);
        orders.erase(location);
        if (orders.empty())
            asks_.erase(price);
    }
    else
    {
        auto& orders = bids_.at(price);
        orders.erase(location);
        if (orders.empty())
            bids_.erase(price);
    }
}


Trades Orderbook::ModifyOrder(OrderModify order)
{
    auto it = orders_.find(order.GetOrderId());
    if (it == orders_.end())
        return { };

    // Copy by value BEFORE CancelOrder runs — otherwise the shared_ptr's last
    // refcount lives in the map node CancelOrder is about to erase, and reading
    // existingOrder->GetOrderType() afterwards is a use-after-free.
    OrderType existingType = it->second.order_->GetOrderType();
    CancelOrder(order.GetOrderId());
    return AddOrder(order.ToOrderPointer(existingType));
}

OrderbookLevelInfos Orderbook::GetOrderInfos() const
{
    LevelInfos bidInfos, askInfos;
    bidInfos.reserve(orders_.size());
    askInfos.reserve(orders_.size());

    auto CreateLevelInfos = [](Price price, const OrderPointers& orders)
    {
        return LevelInfo{ price, std::accumulate(orders.begin(), orders.end(), (Quantity)0,
            [](Quantity runningSum, const OrderPointer& order)
            { return runningSum + order->GetRemainingQuantity(); })};
    };

    for(const auto& [price, orders] : bids_)
        bidInfos.push_back(CreateLevelInfos(price, orders));

    for(const auto& [price, orders] : asks_)
        askInfos.push_back(CreateLevelInfos(price, orders));

    return OrderbookLevelInfos( bidInfos, askInfos );
}

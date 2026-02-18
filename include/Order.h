#pragma once

#include <memory>
#include <list>
#include "Types.h"

class Order
{
    public:
        Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity)
            : orderType_{ orderType }
            , orderId_{ orderId }
            , side_{ side }
            , price_{ price }
            , initialQuantity_{ quantity }
            , remainingQuantity_{ quantity }
        { }

        Order(OrderId orderId, Side side, Quantity quantity)
            : Order(OrderType::Market, orderId, side, Constants::InvalidPrice, quantity)
        { }

        OrderId   GetOrderId()           const { return orderId_; }
        Side      GetSide()              const { return side_; }
        Price     GetPrice()             const { return price_; }
        OrderType GetOrderType()         const { return orderType_; }
        Quantity  GetInitialQuantity()   const { return initialQuantity_; }
        Quantity  GetRemainingQuantity() const { return remainingQuantity_; }
        Quantity  GetFilledQuantity()    const { return GetInitialQuantity() - GetRemainingQuantity(); }
        bool      IsFilled()             const { return GetRemainingQuantity() == 0; }

        void Fill( Quantity quantity);

    private:

        OrderType orderType_;
        OrderId   orderId_;
        Side      side_;
        Price     price_;
        Quantity  initialQuantity_;
        Quantity  remainingQuantity_;
};

//Reference Semantics

using OrderPointer  = std::shared_ptr<Order>;

//Think of List and Vector

using OrderPointers = std::list<OrderPointer>;

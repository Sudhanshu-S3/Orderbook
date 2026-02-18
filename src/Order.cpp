#include <stdexcept>
#include <format>
#include "../include/Order.h"

void Order::Fill( Quantity quantity)
{
    if( quantity > GetRemainingQuantity())
        throw std::logic_error( std::format("Order ({}) cannot be filled for more than its remaining quantity.", GetOrderId()));

    remainingQuantity_ -= quantity;
}

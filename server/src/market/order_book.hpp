/**
 * @file
 * @brief
 *
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_ORDER_BOOK_HPP
#define HFT_SERVER_ORDER_BOOK_HPP

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "market_types.hpp"

namespace hft::server {

class OrderBook {
public:
  OrderBook() {}

  void addOrder(const Order &order) {
    if (order.action == OrderAction::Buy) {
      mBids[order.price].push_back(order);
    } else {
      mAsks[order.price].push_back(order);
    }
    matchOrders();
  }

  void matchOrders() {
    while (!mBids.empty() && !mAsks.empty()) {
      auto &bestBid = mBids.begin()->second;
      auto &bestAsk = mAsks.begin()->second;

      if (bestBid.front().price < bestAsk.front().price) {
        break;
      }
      int tradeQuantity = std::min(bestBid.front().quantity, bestAsk.front().quantity);

      bestBid.front().quantity -= tradeQuantity;
      bestAsk.front().quantity -= tradeQuantity;

      if (bestBid.front().quantity == 0) {
        bestBid.erase(bestBid.begin());
        if (bestBid.empty()) {
          mBids.erase(mBids.begin());
        }
      }
      if (bestAsk.front().quantity == 0) {
        bestAsk.erase(bestAsk.begin());
        if (bestAsk.empty()) {
          mAsks.erase(mAsks.begin());
        }
      }
    }
  }

private:
  std::map<double, std::vector<Order>, std::greater<double>> mBids;
  std::map<double, std::vector<Order>> mAsks;
};

} // namespace hft::server

#endif // HFT_SERVER_ORDER_BOOK_HPP
/**
 * @author Vladimir Pavliv
 * @date 2025-08-04
 */

#include <iostream>

#include <gtest/gtest.h>

#include "config/server_config.hpp"
#include "execution/order_book.hpp"

namespace hft::tests {

using namespace server;
using namespace utils;

namespace {
ClientId rCId() { return generateConnectionId(); }
OrderId rOId() { return generateOrderId(); }
Timestamp rTs() { return getTimestamp(); }

const OrderAction BUY = OrderAction::Buy;
const OrderAction SELL = OrderAction::Sell;
const Ticker tkr = generateTicker();
} // namespace

class OrderBookFixture : public ::testing::Test {
protected:
  void SetUp() override {
    ServerConfig::load("test_server_config.ini");
    LOG_INIT(ServerConfig::cfg.logOutput);
    book = std::make_unique<OrderBook>();
  }

  void TearDown() override {}

  void printStatusQ() {
    if (statusq.empty()) {
      std::cout << "StatusQ is empty" << std::endl;
      return;
    }
    for (auto &status : statusq) {
      std::cout << toString(status) << std::endl;
    }
  }

public:
  template <typename EventType>
  void post(CRef<EventType>) {}

  UPtr<OrderBook> book;
  Vector<ServerOrderStatus> statusq;
};

template <>
void OrderBookFixture::post<ServerOrderStatus>(CRef<ServerOrderStatus> event) {
  statusq.push_back(event);
}

TEST_F(OrderBookFixture, OrderBookLimitReqched) {
  ServerConfig::cfg.orderBookLimit = 10;
  book = std::make_unique<OrderBook>();
  for (size_t idx = 0; idx < ServerConfig::cfg.orderBookLimit; ++idx) {
    ASSERT_TRUE(book->add({0, generateOrder()}, *this));
  }
  ASSERT_FALSE(book->add({0, generateOrder()}, *this));
}

TEST_F(OrderBookFixture, OrdersWontMatch) {
  std::cout << "Opened orders: " << book->openedOrders() << std::endl;

  book->add({rCId(), {rOId(), rTs(), tkr, 1, 40, SELL}}, *this);
  book->add({rCId(), {rOId(), rTs(), tkr, 1, 50, SELL}}, *this);
  book->add({rCId(), {rOId(), rTs(), tkr, 1, 60, SELL}}, *this);

  book->add({rCId(), {rOId(), rTs(), tkr, 1, 30, BUY}}, *this);
  book->add({rCId(), {rOId(), rTs(), tkr, 1, 20, BUY}}, *this);
  book->add({rCId(), {rOId(), rTs(), tkr, 1, 10, BUY}}, *this);

  book->match(*this);

  printStatusQ();
  ASSERT_TRUE(statusq.empty());
}

TEST_F(OrderBookFixture, 3Buy3SellMatch) {
  std::cout << "Opened orders: " << book->openedOrders() << std::endl;

  book->add({rCId(), {rOId(), rTs(), tkr, 1, 40, BUY}}, *this);
  book->add({rCId(), {rOId(), rTs(), tkr, 1, 50, BUY}}, *this);
  book->add({rCId(), {rOId(), rTs(), tkr, 1, 60, BUY}}, *this);

  book->add({rCId(), {rOId(), rTs(), tkr, 1, 30, SELL}}, *this);
  book->add({rCId(), {rOId(), rTs(), tkr, 1, 20, SELL}}, *this);
  book->add({rCId(), {rOId(), rTs(), tkr, 1, 10, SELL}}, *this);

  book->match(*this);

  printStatusQ();
  ASSERT_EQ(statusq.size(), 3);
}

TEST_F(OrderBookFixture, 10Buy1SellMatch) {
  std::cout << "Opened orders: " << book->openedOrders() << std::endl;

  Quantity quantity;
  Price price{10};
  for (uint32_t idx = 0; idx < 10; ++idx) {
    book->add({rCId(), {rOId(), rTs(), tkr, idx, price, BUY}}, *this);
    quantity += idx;
  }

  book->add({rCId(), {rOId(), rTs(), tkr, quantity, price, SELL}}, *this);

  book->match(*this);

  printStatusQ();
  ASSERT_EQ(statusq.size(), 10);
}

} // namespace hft::tests

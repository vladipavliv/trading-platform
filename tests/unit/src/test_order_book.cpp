/**
 * @author Vladimir Pavliv
 * @date 2025-08-04
 */

#include <iostream>

#include <gtest/gtest.h>

#include "config/server_config.hpp"
#include "execution/order_book.hpp"
#include "utils/test_utils.hpp"

namespace hft::tests {

using namespace server;
using namespace utils;

namespace {
ClientId cId() { return generateConnectionId(); }
OrderId oId() { return generateOrderId(); }
Timestamp ts() { return getTimestampNs(); }

const OrderAction BUY = OrderAction::Buy;
const OrderAction SELL = OrderAction::Sell;
const Ticker tkr = generateTicker();
} // namespace

class OrderBookFixture : public ::testing::Test {
public:
  UPtr<OrderBook> book;
  Vector<ServerOrderStatus> statusq;

  void SetUp() override {
    ServerConfig::load("utest_server_config.ini");
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

  template <typename EventType>
  void post(CRef<EventType>) {}

  void addOrder(ServerOrder order) {
    book->add(order, *this);
    book->sendAck(order, *this);
  }
};

template <>
void OrderBookFixture::post<ServerOrderStatus>(CRef<ServerOrderStatus> event) {
  statusq.push_back(event);
}

TEST_F(OrderBookFixture, OrderBookLimitReached) {
  for (size_t idx = 0; idx < ServerConfig::cfg.orderBookLimit; ++idx) {
    ASSERT_TRUE(book->add({0, generateOrder()}, *this));
  }
  ASSERT_FALSE(book->add({0, generateOrder()}, *this));
}

TEST_F(OrderBookFixture, OrdersWontMatch) {
  addOrder({cId(), {oId(), ts(), tkr, 1, 40, SELL}});
  addOrder({cId(), {oId(), ts(), tkr, 1, 50, SELL}});
  addOrder({cId(), {oId(), ts(), tkr, 1, 60, SELL}});

  addOrder({cId(), {oId(), ts(), tkr, 1, 30, BUY}});
  addOrder({cId(), {oId(), ts(), tkr, 1, 20, BUY}});
  addOrder({cId(), {oId(), ts(), tkr, 1, 10, BUY}});

  book->match(*this);

  printStatusQ();
  ASSERT_TRUE(statusq.size() == 6);
}

TEST_F(OrderBookFixture, 3Buy3SellMatch) {
  addOrder({cId(), {oId(), ts(), tkr, 1, 40, BUY}});
  addOrder({cId(), {oId(), ts(), tkr, 1, 50, BUY}});
  addOrder({cId(), {oId(), ts(), tkr, 1, 60, BUY}});

  addOrder({cId(), {oId(), ts(), tkr, 1, 30, SELL}});
  addOrder({cId(), {oId(), ts(), tkr, 1, 20, SELL}});
  addOrder({cId(), {oId(), ts(), tkr, 1, 10, SELL}});

  book->match(*this);

  printStatusQ();
  ASSERT_EQ(statusq.size(), 12);
}

TEST_F(OrderBookFixture, 10Buy1SellMatch) {
  Quantity quantity{1};
  Price price{10};

  for (uint32_t idx = 0; idx < 10; ++idx) {
    addOrder({cId(), {oId(), ts(), tkr, idx, price, BUY}});
    quantity += idx;
  }

  addOrder({cId(), {oId(), ts(), tkr, quantity, price, SELL}});
  book->match(*this);

  printStatusQ();
  ASSERT_EQ(statusq.size(), 31);
}

} // namespace hft::tests

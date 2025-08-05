/**
 * @author Vladimir Pavliv
 * @date 2025-08-03
 */

#ifndef HFT_SERVER_MEHMARKETDATA_HPP
#define HFT_SERVER_MEHMARKETDATA_HPP

#include "server_ticker_data.hpp"
#include "ticker.hpp"
#include "types.hpp"

namespace hft::server {

/**
 * @brief Doesnt seem to perform any better then the hashmap that it uses under the hood
 * Initial idea was to have all TickerData assigned to one worker - in contiguous memory block
 * so cache locality could be better, but this implementation isn't showing better benchmark numbers
 */
class MehMarketData {
public:
  explicit MehMarketData(size_t tickerCount) {
    index_.reserve(tickerCount);
    data_.reserve(tickerCount);
  }

  void addTicker(Ticker ticker, ThreadId workerId) {
    data_.emplace_back(workerId);
    index_.emplace(ticker, &data_.back());
  }

  TickerData &operator[](CRef<Ticker> ticker) {
    auto it = index_.find(ticker);
    if (it == index_.end()) [[unlikely]] {
      throw std::out_of_range("Ticker not found");
    }
    return *it->second;
  }

  auto operator[](CRef<Ticker> ticker) const -> CRef<TickerData> {
    auto it = index_.find(ticker);
    if (it == index_.end()) [[unlikely]] {
      throw std::out_of_range("Ticker not found");
    }
    return *it->second;
  }

  bool contains(CRef<Ticker> ticker) const { return index_.find(ticker) != index_.end(); }

  ThreadId workerId(CRef<Ticker> ticker) const {
    auto it = index_.find(ticker);
    if (it == index_.end()) [[unlikely]] {
      throw std::out_of_range("Ticker not found");
    }
    return it->second->getThreadId();
  }

  size_t openedOrders() const {
    size_t result{0};
    for (auto &tkrData : data_) {
      result += tkrData.orderBook.openedOrders();
    }
    return result;
  }

  Vector<TickerData> &data() { return data_; }

  CRef<Vector<TickerData>> data() const { return data_; }

  size_t workersCount() const { return data_.size(); }

  size_t tickersCount() const { return data_.size(); }

private:
  boost::unordered_flat_map<Ticker, TickerData *, TickerHash> index_;
  Vector<TickerData> data_;
};

} // namespace hft::server

#endif // HFT_SERVER_MEHMARKETDATA_HPP
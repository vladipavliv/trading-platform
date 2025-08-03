/**
 * @author Vladimir Pavliv
 * @date 2025-08-03
 */

#ifndef HFT_SERVER_MARKETDATA_HPP
#define HFT_SERVER_MARKETDATA_HPP

#include "server_ticker_data.hpp"
#include "ticker.hpp"
#include "types.hpp"

namespace hft::server {

/**
 * @brief Efficient container to distribute tickers between workers
 * @details Tickers data is indexed and packed tightly in contiguous
 * memory block of vector, separate vector per worker.
 */
class MarketData {
  struct Index {
    ThreadId workerId;
    size_t dataId;
  };

public:
  MarketData(size_t workerCount, size_t tickerCount) {
    index_.reserve(tickerCount);
    data_.reserve(workerCount);
    for (size_t idx = 0; idx < workerCount; ++idx) {
      data_.emplace_back(Vector<TickerData>{});
      // Reserve a bit more space for potential rerouting
      data_[idx].reserve(tickerCount / workerCount + 0.2 * tickerCount);
    }
  }

  void addTicker(Ticker ticker, ThreadId workerId) {
    if (workerId >= data_.size()) {
      throw std::runtime_error("workerId is out of bounds");
    }
    data_[workerId].emplace_back(workerId);
    index_[ticker] = Index{workerId, data_[workerId].size() - 1};
  }

  TickerData &operator[](CRef<Ticker> ticker) {
    auto it = index_.find(ticker);
    if (it == index_.end()) {
      throw std::out_of_range("Ticker not found");
    }
    const auto &idx = it->second;
    return data_[idx.workerId][idx.dataId];
  }

  auto operator[](CRef<Ticker> ticker) const -> CRef<TickerData> {
    auto it = index_.find(ticker);
    if (it == index_.end()) {
      throw std::out_of_range("Ticker not found");
    }
    const auto &idx = it->second;
    return data_[idx.workerId][idx.dataId];
  }

  bool contains(CRef<Ticker> ticker) const { return index_.find(ticker) != index_.end(); }

  size_t workers() const { return data_.size(); }

  size_t tickers() const {
    size_t result{0};
    for (auto &data : data_) {
      result += data.size();
    }
    return result;
  }

  auto getWorkerData(ThreadId workerId) const -> CRef<Vector<TickerData>> {
    // TODO() make proper iterating to avoid this
    if (workerId >= data_.size()) {
      throw std::runtime_error("workerId is out of bounds");
    }
    return data_[workerId];
  }

private:
  HashMap<Ticker, Index, TickerHash> index_;
  Vector<Vector<TickerData>> data_;
};

} // namespace hft::server

#endif // HFT_SERVER_MARKETDATA_HPP
/**
 * @author Vladimir Pavliv
 * @date 2025-04-07
 */

#ifndef HFT_BENCHSERVER_HPP
#define HFT_BENCHSERVER_HPP

#include <benchmark/benchmark.h>
#include <iostream>

#include "adapters/postgres/postgres_adapter.hpp"
#include "commands/server_command.hpp"
#include "config/server_config.hpp"
#include "execution/coordinator.hpp"
#include "server_types.hpp"
#include "storage/storage.hpp"
#include "types.hpp"
#include "types/constants.hpp"
#include "utils/test_data.hpp"
#include "utils/utils.hpp"

namespace hft::benchmarks {

class BM_Sys_ServerFix : public benchmark::Fixture {
public:
  BM_Sys_ServerFix();
  ~BM_Sys_ServerFix();

  UPtr<server::ServerBus> bus;

  size_t tickerCount{10};
  size_t workerCount{1};
  size_t orderLimit{ORDER_BOOK_LIMIT};

  TestTickerData tickers;
  TestOrderData orders;
  TestMarketData marketData;

  UPtr<server::Coordinator> coordinator;

  std::atomic_flag flag{ATOMIC_FLAG_INIT};
  std::jthread systemThread;

  void SetUp(const ::benchmark::State &state) override;
  void TearDown(const ::benchmark::State &state) override;

  void setupBus();
  void setupCoordinator();
};

} // namespace hft::benchmarks

#endif // HFT_BENCHSERVER_HPP

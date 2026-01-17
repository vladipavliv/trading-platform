/**
 * @author Vladimir Pavliv
 * @date 2025-04-07
 */

#ifndef HFT_BENCHSERVER_HPP
#define HFT_BENCHSERVER_HPP

#include <benchmark/benchmark.h>
#include <iostream>

#include "adapters/postgres/postgres_adapter.hpp"
#include "commands/command.hpp"
#include "config/server_config.hpp"
#include "events.hpp"
#include "execution/coordinator.hpp"
#include "primitive_types.hpp"
#include "storage/storage.hpp"
#include "types/constants.hpp"
#include "utils/data_generator.hpp"

namespace hft::benchmarks {

class BM_Sys_ServerFix : public benchmark::Fixture {
public:
  BM_Sys_ServerFix();
  ~BM_Sys_ServerFix();

  UPtr<server::ServerBus> bus;

  size_t tickerCount{10};
  size_t workerCount{1};

  tests::GenTickerData tickers;
  tests::GenOrderData orders;
  tests::GenMarketData marketData;

  ALIGN_CL AtomicUInt64 processed;
  ALIGN_CL AtomicBool error;
  ALIGN_CL std::atomic_flag flag{ATOMIC_FLAG_INIT};

  UPtr<server::Coordinator> coordinator;
  std::jthread systemThread;

  void SetUp(const ::benchmark::State &state) override;
  void TearDown(const ::benchmark::State &state) override;

  void setupBus();
  void setupCoordinator();

  void post(const server::ServerEvent &s);
  void post(const server::InternalOrderStatus &s);
};

} // namespace hft::benchmarks

#endif // HFT_BENCHSERVER_HPP

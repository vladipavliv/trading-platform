/**
 * @author Vladimir Pavliv
 * @date 2026-01-15
 */

#ifndef HFT_SERVER_PRICELEVELORDERBOOK_HPP
#define HFT_SERVER_PRICELEVELORDERBOOK_HPP

#include "id/slot_id.hpp"
#include "id/slot_id_pool.hpp"
#include "primitive_types.hpp"
#include "schema.hpp"
#include "utils/huge_array.hpp"

namespace hft::server {

class PriceLevelOrderBook {
  struct Node {
    BookOrderId localId;
    SystemOrderId internalId;

    BookOrderId next;
    BookOrderId prev;

    uint32_t priceIdx;
    uint32_t qty;
  };

  struct PriceLevel {
    uint32_t head = 0;
    uint32_t tail = 0;
    uint64_t volume = 0;
  };

public:
  PriceLevelOrderBook();

private:
  uint32_t nextFreeIdx_;

  ALIGN_CL HugeArray<Node, MAX_BOOK_ORDERS> nodePool_;
};
} // namespace hft::server

#endif // HFT_SERVER_PRICELEVELORDERBOOK_HPP
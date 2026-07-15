#pragma once

#include <vector>

#include "Model/Order/Order.h"
#include "Model/Sample/Sample.h"

namespace Model {

// PRD 7.4's "승인 시점 가용 재고": physical stock minus the quantity already
// allocated to CONFIRMED/PRODUCING orders for this sample, clamped to 0 (an
// over-allocated sample has no available stock left, never a negative one).
// RESERVED/REJECTED orders aren't allocated yet; RELEASE orders are already
// deducted from physical stock (Phase 4), so neither is subtracted again.
int CalculateAvailableStock(const Sample& sample, const std::vector<Order>& orders);

}  // namespace Model

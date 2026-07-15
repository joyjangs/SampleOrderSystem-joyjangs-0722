#pragma once

#include <string>

#include "Model/JsonFileRepository.h"
#include "Model/Order/Order.h"

namespace Model {

class OrderRepository : public JsonFileRepository<Order> {
public:
    explicit OrderRepository(std::string filePath = "data/orders.json");
};

}  // namespace Model

#include "Model/OrderRepository.h"

namespace Model {

OrderRepository::OrderRepository(std::string filePath) : JsonFileRepository<Order>(std::move(filePath)) {}

}  // namespace Model

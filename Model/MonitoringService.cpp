#include "Model/MonitoringService.h"

#include <map>

#include "Model/OrderStatus.h"

namespace Model {

MonitoringService::MonitoringService(const SampleRepository& sampleRepository, const OrderRepository& orderRepository,
                                      const ProductionJobRepository& productionJobRepository)
    : sampleRepository_(sampleRepository),
      orderRepository_(orderRepository),
      productionJobRepository_(productionJobRepository) {}

OrderStatusSummary MonitoringService::GetOrderStatusSummary() const {
    OrderStatusSummary summary;
    for (const Order& order : orderRepository_.FindAll()) {
        switch (order.GetStatus()) {
            case OrderStatus::Reserved:
                ++summary.reservedCount;
                break;
            case OrderStatus::Confirmed:
                ++summary.confirmedCount;
                break;
            case OrderStatus::Producing:
                ++summary.producingCount;
                break;
            case OrderStatus::Release:
                ++summary.releaseCount;
                break;
            case OrderStatus::Rejected:
                break;  // excluded from monitoring aggregates
        }
    }
    return summary;
}

std::vector<SampleStockView> MonitoringService::GetSampleStockViews() const {
    std::map<std::string, int> pendingQuantityBySampleId;
    for (const Order& order : orderRepository_.FindAll()) {
        if (order.GetStatus() == OrderStatus::Producing) {
            pendingQuantityBySampleId[order.GetSampleId()] += order.GetQuantity();
        }
    }

    std::vector<SampleStockView> views;
    for (const Sample& sample : sampleRepository_.FindAll()) {
        SampleStockView view;
        view.sampleId = sample.GetId();
        view.sampleName = sample.GetName();
        view.stock = sample.GetStock();
        view.pendingOrderQuantity = pendingQuantityBySampleId[sample.GetId()];
        view.status = JudgeStockStatus(view.stock, view.pendingOrderQuantity);
        views.push_back(view);
    }
    return views;
}

MainMenuSummary MonitoringService::GetMainMenuSummary() const {
    MainMenuSummary summary;
    for (const Sample& sample : sampleRepository_.FindAll()) {
        ++summary.registeredSampleCount;
        summary.totalStock += sample.GetStock();
    }
    for (const Order& order : orderRepository_.FindAll()) {
        if (order.GetStatus() != OrderStatus::Rejected) {
            ++summary.totalOrderCount;
        }
    }
    summary.productionQueueCount = static_cast<int>(productionJobRepository_.FindAll().size());
    return summary;
}

StockStatus MonitoringService::JudgeStockStatus(int stock, int pendingOrderQuantity) const {
    if (stock == 0) return StockStatus::Depleted;
    if (stock < pendingOrderQuantity) return StockStatus::Shortage;
    return StockStatus::Sufficient;
}

}  // namespace Model

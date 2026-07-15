#include <gtest/gtest.h>

#include <filesystem>

#include "Model/OrderRepository.h"
#include "Model/ProductionJobRepository.h"
#include "Model/SampleRepository.h"

namespace {

class SampleRepositoryTest : public ::testing::Test {
protected:
    std::string path = "data/test_samples.json";
    void TearDown() override { std::filesystem::remove(path); }
};

TEST_F(SampleRepositoryTest, AddThenFindByIdReturnsIt) {
    Model::SampleRepository repository(path);

    repository.Add(Model::Sample("S-001", "SampleA", 10.0, 0.9, 50));

    auto found = repository.FindById("S-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->GetName(), "SampleA");
}

TEST_F(SampleRepositoryTest, FindByIdReturnsNulloptWhenMissing) {
    Model::SampleRepository repository(path);
    EXPECT_FALSE(repository.FindById("does-not-exist").has_value());
}

TEST_F(SampleRepositoryTest, UpdateReplacesMatchingEntity) {
    Model::SampleRepository repository(path);
    repository.Add(Model::Sample("S-001", "SampleA", 10.0, 0.9, 50));

    repository.Update(Model::Sample("S-001", "SampleA", 10.0, 0.9, 80));

    EXPECT_EQ(repository.FindById("S-001")->GetStock(), 80);
}

TEST_F(SampleRepositoryTest, RemoveDeletesEntity) {
    Model::SampleRepository repository(path);
    repository.Add(Model::Sample("S-001", "SampleA", 10.0, 0.9, 50));

    repository.Remove("S-001");

    EXPECT_FALSE(repository.FindById("S-001").has_value());
    EXPECT_TRUE(repository.FindAll().empty());
}

TEST_F(SampleRepositoryTest, LoadOnMissingFileYieldsEmptyCollection) {
    Model::SampleRepository repository(path);
    repository.Load();
    EXPECT_TRUE(repository.FindAll().empty());
}

TEST_F(SampleRepositoryTest, SurvivesReloadFromNewInstance) {
    {
        Model::SampleRepository repository(path);
        repository.Add(Model::Sample("S-001", "SampleA", 10.0, 0.9, 50));
    }

    Model::SampleRepository reloaded(path);
    reloaded.Load();

    auto found = reloaded.FindById("S-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->GetStock(), 50);
}

TEST_F(SampleRepositoryTest, PreservesInsertionOrderAcrossMultipleEntities) {
    {
        Model::SampleRepository repository(path);
        repository.Add(Model::Sample("S-001", "A", 1.0, 1.0, 1));
        repository.Add(Model::Sample("S-002", "B", 1.0, 1.0, 2));
        repository.Add(Model::Sample("S-003", "C", 1.0, 1.0, 3));
    }

    Model::SampleRepository reloaded(path);
    reloaded.Load();

    auto all = reloaded.FindAll();
    ASSERT_EQ(all.size(), 3u);
    EXPECT_EQ(all[0].GetId(), "S-001");
    EXPECT_EQ(all[1].GetId(), "S-002");
    EXPECT_EQ(all[2].GetId(), "S-003");
}

class OrderProductionRestartTest : public ::testing::Test {
protected:
    std::string ordersPath = "data/test_orders.json";
    std::string jobsPath = "data/test_production_jobs.json";
    void TearDown() override {
        std::filesystem::remove(ordersPath);
        std::filesystem::remove(jobsPath);
    }
};

TEST_F(OrderProductionRestartTest, ProducingOrderAndQueuedJobSurviveRestart) {
    {
        Model::OrderRepository orders(ordersPath);
        Model::ProductionJobRepository jobs(jobsPath);
        orders.Add(Model::Order("ORD-1", "S-001", "Acme", 50, Model::OrderStatus::Producing,
                                 "2026-04-16T10:00:00", "2026-04-16T10:00:00"));
        jobs.Add(Model::ProductionJob("ORD-1", "S-001", 50, 100, 1000.0));  // still queued: startedAt = null
    }

    Model::OrderRepository reloadedOrders(ordersPath);
    Model::ProductionJobRepository reloadedJobs(jobsPath);
    reloadedOrders.Load();
    reloadedJobs.Load();

    auto order = reloadedOrders.FindById("ORD-1");
    ASSERT_TRUE(order.has_value());
    EXPECT_EQ(order->GetStatus(), Model::OrderStatus::Producing);

    auto job = reloadedJobs.FindById("ORD-1");
    ASSERT_TRUE(job.has_value());
    EXPECT_FALSE(job->GetStartedAt().has_value());
}

TEST_F(OrderProductionRestartTest, StartedProductionJobKeepsStartedAtAfterRestart) {
    {
        Model::ProductionJobRepository jobs(jobsPath);
        jobs.Add(Model::ProductionJob("ORD-2", "S-001", 50, 100, 1000.0, std::string("2026-04-16T10:00:00")));
    }

    Model::ProductionJobRepository reloadedJobs(jobsPath);
    reloadedJobs.Load();

    auto job = reloadedJobs.FindById("ORD-2");
    ASSERT_TRUE(job.has_value());
    ASSERT_TRUE(job->GetStartedAt().has_value());
    EXPECT_EQ(*job->GetStartedAt(), "2026-04-16T10:00:00");
}

}  // namespace

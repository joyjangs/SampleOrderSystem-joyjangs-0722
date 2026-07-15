#include <gtest/gtest.h>

#include <filesystem>

#include "Model/OrderRepository.h"
#include "Model/ProductionJobRepository.h"
#include "Model/SampleRepository.h"
#include "Tests/TestSupport/TempFileFixture.h"

namespace {

class SampleRepositoryTest : public Tests::TempFileFixture {
protected:
    SampleRepositoryTest() : TempFileFixture("data/test_samples.json") {}
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

TEST_F(SampleRepositoryTest, AddDoesNotRejectDuplicateId) {
    // Regression test (Phase1 §2.1/§6.2): IRepository::Add is a plain push_back
    // shared with Order/ProductionJob; it must NOT start rejecting duplicate
    // ids on its own. Duplicate-id rejection is SampleController::HandleRegister's
    // responsibility (via a FindById check before calling Add), not Repository's.
    Model::SampleRepository repository(path);
    repository.Add(Model::Sample("S-001", "SampleA", 10.0, 0.9, 50));

    repository.Add(Model::Sample("S-001", "SampleA-Duplicate", 5.0, 0.5, 10));

    EXPECT_EQ(repository.FindAll().size(), 2u);
}

TEST_F(SampleRepositoryTest, SearchByNameReturnsCaseInsensitivePartialMatches) {
    Model::SampleRepository repository(path);
    repository.Add(Model::Sample("S-001", "Wafer-A", 10.0, 0.9, 50));
    repository.Add(Model::Sample("S-002", "wafer-B", 10.0, 0.9, 30));
    repository.Add(Model::Sample("S-003", "Chip-C", 10.0, 0.9, 20));

    auto results = repository.SearchByName("WAFER");

    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].GetId(), "S-001");
    EXPECT_EQ(results[1].GetId(), "S-002");
}

TEST_F(SampleRepositoryTest, SearchByNameMatchesSubstringNotJustPrefix) {
    Model::SampleRepository repository(path);
    repository.Add(Model::Sample("S-001", "Premium-Wafer-A", 10.0, 0.9, 50));

    auto results = repository.SearchByName("wafer");

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].GetId(), "S-001");
}

TEST_F(SampleRepositoryTest, SearchByNameReturnsEmptyWhenNoMatch) {
    Model::SampleRepository repository(path);
    repository.Add(Model::Sample("S-001", "Wafer-A", 10.0, 0.9, 50));

    auto results = repository.SearchByName("nonexistent");

    EXPECT_TRUE(results.empty());
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

class OrderRepositoryTest : public Tests::TempFileFixture {
protected:
    OrderRepositoryTest() : TempFileFixture("data/test_order_crud.json") {}
};

TEST_F(OrderRepositoryTest, AddThenFindByIdReturnsIt) {
    Model::OrderRepository repository(path);

    repository.Add(Model::Order("ORD-1", "S-001", "Acme", 50, Model::OrderStatus::Reserved,
                                 "2026-04-16T10:00:00", "2026-04-16T10:00:00"));

    auto found = repository.FindById("ORD-1");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->GetCustomerName(), "Acme");
    EXPECT_EQ(found->GetStatus(), Model::OrderStatus::Reserved);
}

TEST_F(OrderRepositoryTest, FindByIdReturnsNulloptWhenMissing) {
    Model::OrderRepository repository(path);
    EXPECT_FALSE(repository.FindById("does-not-exist").has_value());
}

TEST_F(OrderRepositoryTest, FindAllCountMatchesAddedEntities) {
    Model::OrderRepository repository(path);
    repository.Add(Model::Order("ORD-1", "S-001", "Acme", 50, Model::OrderStatus::Reserved,
                                 "2026-04-16T10:00:00", "2026-04-16T10:00:00"));
    repository.Add(Model::Order("ORD-2", "S-001", "Globex", 20, Model::OrderStatus::Reserved,
                                 "2026-04-16T10:05:00", "2026-04-16T10:05:00"));

    EXPECT_EQ(repository.FindAll().size(), 2u);
}

TEST_F(OrderRepositoryTest, UpdateReplacesMatchingEntity) {
    Model::OrderRepository repository(path);
    repository.Add(Model::Order("ORD-1", "S-001", "Acme", 50, Model::OrderStatus::Reserved,
                                 "2026-04-16T10:00:00", "2026-04-16T10:00:00"));

    repository.Update(Model::Order("ORD-1", "S-001", "Acme", 50, Model::OrderStatus::Confirmed,
                                    "2026-04-16T10:00:00", "2026-04-16T12:00:00"));

    auto found = repository.FindById("ORD-1");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->GetStatus(), Model::OrderStatus::Confirmed);
    EXPECT_EQ(found->GetUpdatedAt(), "2026-04-16T12:00:00");
}

TEST_F(OrderRepositoryTest, RemoveDeletesEntity) {
    Model::OrderRepository repository(path);
    repository.Add(Model::Order("ORD-1", "S-001", "Acme", 50, Model::OrderStatus::Reserved,
                                 "2026-04-16T10:00:00", "2026-04-16T10:00:00"));

    repository.Remove("ORD-1");

    EXPECT_FALSE(repository.FindById("ORD-1").has_value());
    EXPECT_TRUE(repository.FindAll().empty());
}

TEST_F(OrderRepositoryTest, LoadOnMissingFileYieldsEmptyCollection) {
    Model::OrderRepository repository(path);
    repository.Load();
    EXPECT_TRUE(repository.FindAll().empty());
}

TEST_F(OrderRepositoryTest, PreservesInsertionOrderAcrossMultipleEntitiesAfterReload) {
    {
        Model::OrderRepository repository(path);
        repository.Add(Model::Order("ORD-1", "S-001", "Acme", 10, Model::OrderStatus::Reserved,
                                     "2026-04-16T10:00:00", "2026-04-16T10:00:00"));
        repository.Add(Model::Order("ORD-2", "S-001", "Globex", 20, Model::OrderStatus::Reserved,
                                     "2026-04-16T10:05:00", "2026-04-16T10:05:00"));
        repository.Add(Model::Order("ORD-3", "S-001", "Initech", 30, Model::OrderStatus::Reserved,
                                     "2026-04-16T10:10:00", "2026-04-16T10:10:00"));
    }

    Model::OrderRepository reloaded(path);
    reloaded.Load();

    auto all = reloaded.FindAll();
    ASSERT_EQ(all.size(), 3u);
    EXPECT_EQ(all[0].GetOrderId(), "ORD-1");
    EXPECT_EQ(all[1].GetOrderId(), "ORD-2");
    EXPECT_EQ(all[2].GetOrderId(), "ORD-3");
}

class ProductionJobRepositoryTest : public Tests::TempFileFixture {
protected:
    ProductionJobRepositoryTest() : TempFileFixture("data/test_production_job_crud.json") {}
};

TEST_F(ProductionJobRepositoryTest, AddThenFindByIdReturnsIt) {
    Model::ProductionJobRepository repository(path);

    repository.Add(Model::ProductionJob("ORD-1", "S-001", 50, 100, 1000.0));

    auto found = repository.FindById("ORD-1");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->GetActualQuantity(), 100);
}

TEST_F(ProductionJobRepositoryTest, FindByIdReturnsNulloptWhenMissing) {
    Model::ProductionJobRepository repository(path);
    EXPECT_FALSE(repository.FindById("does-not-exist").has_value());
}

TEST_F(ProductionJobRepositoryTest, UpdateReplacesMatchingEntity) {
    Model::ProductionJobRepository repository(path);
    repository.Add(Model::ProductionJob("ORD-1", "S-001", 50, 100, 1000.0));

    repository.Update(Model::ProductionJob("ORD-1", "S-001", 50, 100, 1000.0, std::string("2026-04-16T10:00:00")));

    auto found = repository.FindById("ORD-1");
    ASSERT_TRUE(found.has_value());
    ASSERT_TRUE(found->GetStartedAt().has_value());
    EXPECT_EQ(*found->GetStartedAt(), "2026-04-16T10:00:00");
}

TEST_F(ProductionJobRepositoryTest, RemoveDeletesEntity) {
    Model::ProductionJobRepository repository(path);
    repository.Add(Model::ProductionJob("ORD-1", "S-001", 50, 100, 1000.0));

    repository.Remove("ORD-1");

    EXPECT_FALSE(repository.FindById("ORD-1").has_value());
    EXPECT_TRUE(repository.FindAll().empty());
}

TEST_F(ProductionJobRepositoryTest, LoadOnMissingFileYieldsEmptyCollection) {
    Model::ProductionJobRepository repository(path);
    repository.Load();
    EXPECT_TRUE(repository.FindAll().empty());
}

TEST_F(ProductionJobRepositoryTest, PreservesInsertionOrderAcrossMultipleEntitiesAfterReload) {
    {
        Model::ProductionJobRepository repository(path);
        repository.Add(Model::ProductionJob("ORD-1", "S-001", 10, 20, 100.0));
        repository.Add(Model::ProductionJob("ORD-2", "S-001", 15, 30, 150.0));
        repository.Add(Model::ProductionJob("ORD-3", "S-001", 5, 10, 50.0));
    }

    Model::ProductionJobRepository reloaded(path);
    reloaded.Load();

    auto all = reloaded.FindAll();
    ASSERT_EQ(all.size(), 3u);
    EXPECT_EQ(all[0].GetOrderId(), "ORD-1");
    EXPECT_EQ(all[1].GetOrderId(), "ORD-2");
    EXPECT_EQ(all[2].GetOrderId(), "ORD-3");
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

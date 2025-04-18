#define _USE_MATH_DEFINES

#include <catch2/catch_test_macros.hpp>

#include "../src/collision_detector.h"

using namespace collision_detector;

class TestingProvider : public collision_detector::ItemGathererProvider {
public:
	TestingProvider() = default;

	TestingProvider(std::vector<Item>&& items, std::vector<Gatherer>&& gatherers)
		:items_(std::move(items)), gatherers_(std::move(gatherers)) {
	}

	size_t ItemsCount() const override
	{
		return items_.size();
	}

	Item GetItem(size_t idx) const override
	{
		return items_.at(idx);
	}

	size_t GatherersCount() const override
	{
		return gatherers_.size();
	}

	Gatherer GetGatherer(size_t idx) const override
	{
		return gatherers_.at(idx);
	}

private:
	std::vector<Item> items_;
	std::vector<Gatherer> gatherers_;
};

struct BadProviders {
	TestingProvider empty_provider;
	TestingProvider no_items_provider{ {}, {{}, {}, {}} };
	TestingProvider no_gatherers_provider{ {{}, {}, {}}, {} };
};

TEST_CASE_METHOD(BadProviders, "Test no events") {
	CHECK(FindGatherEvents(empty_provider).size() == 0);
	CHECK(FindGatherEvents(no_items_provider).size() == 0);
	CHECK(FindGatherEvents(no_gatherers_provider).size() == 0);
}

// Напишите здесь тесты для функции collision_detector::FindGatherEvents

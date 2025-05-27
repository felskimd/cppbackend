#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <cmath>
#include <catch2/catch_test_macros.hpp>

#include "../src/model.h"
#include "../src/model_serialization.h"
#include "../src/loot_generator.h"
#include "../src/collision_detector.h"

using namespace std::literals;
using namespace collision_detector;
using namespace model;

namespace {

using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

struct Fixture {
    std::stringstream strm;
    OutputArchive output_archive{strm};
};

}  // namespace

SCENARIO("Loot generation") {
    using loot_gen::LootGenerator;
    using TimeInterval = LootGenerator::TimeInterval;

    GIVEN("a loot generator") {
        LootGenerator gen{1s, 1.0};

        constexpr TimeInterval TIME_INTERVAL = 1s;

        WHEN("loot count is enough for every looter") {
            THEN("no loot is generated") {
                for (unsigned looters = 0; looters < 10; ++looters) {
                    for (unsigned loot = looters; loot < looters + 10; ++loot) {
                        INFO("loot count: " << loot << ", looters: " << looters);
                        REQUIRE(gen.Generate(TIME_INTERVAL, loot, looters) == 0);
                    }
                }
            }
        }

        WHEN("number of looters exceeds loot count") {
            THEN("number of loot is proportional to loot difference") {
                for (unsigned loot = 0; loot < 10; ++loot) {
                    for (unsigned looters = loot; looters < loot + 10; ++looters) {
                        INFO("loot count: " << loot << ", looters: " << looters);
                        REQUIRE(gen.Generate(TIME_INTERVAL, loot, looters) == looters - loot);
                    }
                }
            }
        }
    }

    GIVEN("a loot generator with some probability") {
        constexpr TimeInterval BASE_INTERVAL = 1s;
        LootGenerator gen{BASE_INTERVAL, 0.5};

        WHEN("time is greater than base interval") {
            THEN("number of generated loot is increased") {
                CHECK(gen.Generate(BASE_INTERVAL * 2, 0, 4) == 3);
            }
        }

        WHEN("time is less than base interval") {
            THEN("number of generated loot is decreased") {
                const auto time_interval
                    = std::chrono::duration_cast<TimeInterval>(std::chrono::duration<double>{
                        1.0 / (std::log(1 - 0.5) / std::log(1.0 - 0.25))});
                CHECK(gen.Generate(time_interval, 0, 4) == 1);
            }
        }
    }

    GIVEN("a loot generator with custom random generator") {
        LootGenerator gen{1s, 0.5, [] {
                              return 0.5;
                          }};
        WHEN("loot is generated") {
            THEN("number of loot is proportional to random generated values") {
                const auto time_interval
                    = std::chrono::duration_cast<TimeInterval>(std::chrono::duration<double>{
                        1.0 / (std::log(1 - 0.5) / std::log(1.0 - 0.25))});
                CHECK(gen.Generate(time_interval, 0, 4) == 0);
                CHECK(gen.Generate(time_interval, 0, 4) == 1);
            }
        }
    }
}

class TestingProvider : public collision_detector::ItemGathererProvider {
public:
	TestingProvider() = default;

	TestingProvider(std::vector<collision_detector::Item>&& items, std::vector<Gatherer>&& gatherers)
		:items_(std::move(items)), gatherers_(std::move(gatherers)) {
	}

	size_t ItemsCount() const override
	{
		return items_.size();
	}

    collision_detector::Item GetItem(size_t idx) const override
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
	std::vector<collision_detector::Item> items_;
	std::vector<Gatherer> gatherers_;
};

struct BadProviders {
	TestingProvider empty_provider;
	TestingProvider no_items_provider{ {}, {{}, {}, {}} };
	TestingProvider no_gatherers_provider{ {{}, {}, {}}, {} };
};

struct GoodProviders {
	TestingProvider one_gatherer_provider{
		{
			{{0,1}, 2.},
			{{-2,0}, 1.},
			{{5,0}, 0.2},
			{{11,0}, 0.9}
		},
		{
			{{0, 0}, {10, 0}, 1.}
		}
	};
	TestingProvider one_item_provider{
		{
			{{0,0}, 1.}
		},
		{
			{{0, 0}, {10, 0}, 1.},
			{{0, 10}, {0, 0}, 1.},
			{{-10, 0}, {0, 0}, 1.}
		}
	};
};

TEST_CASE_METHOD(BadProviders, "Test bad providers with no events") {
	CHECK(FindGatherEvents(empty_provider).size() == 0);
	CHECK(FindGatherEvents(no_items_provider).size() == 0);
	CHECK(FindGatherEvents(no_gatherers_provider).size() == 0);
}

TEST_CASE_METHOD(GoodProviders, "Test good providers") {
	auto one_gatherer_result = FindGatherEvents(one_gatherer_provider);
	CHECK(one_gatherer_result.size() == 2);
	CHECK(one_gatherer_result[0].gatherer_id == 0);
	CHECK(one_gatherer_result[0].item_id == 0);
	CHECK(one_gatherer_result[1].item_id == 2);
	auto one_item_result = FindGatherEvents(one_item_provider);
	CHECK(one_item_result.size() == 3);
	CHECK(one_item_result[0].gatherer_id == 0);
	CHECK(one_item_result[1].gatherer_id == 1);
	CHECK(one_item_result[2].gatherer_id == 2);
}

SCENARIO_METHOD(Fixture, "Point serialization") {
    GIVEN("A point") {
        const geom::Point2D p{10, 20};
        WHEN("point is serialized") {
            output_archive << p;

            THEN("it is equal to point after serialization") {
                InputArchive input_archive{strm};
                geom::Point2D restored_point;
                input_archive >> restored_point;
                CHECK(p == restored_point);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Dog Serialization") {
    GIVEN("a dog") {
        const auto dog = [] {
            Dog dog{"Pluto"s, 3};
            dog.AddScore(42);
            CHECK(dog.CanTakeLoot());
            dog.TakeLoot({0, 0});
            dog.Move(Direction::EAST, 12);
            return dog;
        }();

        WHEN("dog is serialized") {
            {
                serialization::DogRepr repr{dog};
                output_archive << repr;
            }

            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::DogRepr repr;
                input_archive >> repr;
                const auto restored = repr.Restore();

                CHECK(dog.GetId() == restored.GetId());
                CHECK(dog.GetName() == restored.GetName());
                CHECK(dog.GetPosition() == restored.GetPosition());
                CHECK(dog.GetSpeed() == restored.GetSpeed());
                CHECK(dog.GetPocketsCapacity() == restored.GetPocketsCapacity());
                CHECK(dog.GetItems() == restored.GetItems());
            }
        }
    }
}
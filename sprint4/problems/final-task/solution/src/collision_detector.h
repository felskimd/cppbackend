#pragma once

#include <algorithm>
#include <vector>
#include <compare>

namespace geom {

    struct Vec2D {
        Vec2D() = default;
        Vec2D(double x, double y)
            : x(x)
            , y(y) {
        }

        Vec2D& operator*=(double scale) {
            x *= scale;
            y *= scale;
            return *this;
        }

        auto operator<=>(const Vec2D&) const = default;

        double x = 0;
        double y = 0;
    };

    inline Vec2D operator*(Vec2D lhs, double rhs) {
        return lhs *= rhs;
    }

    inline Vec2D operator*(double lhs, Vec2D rhs) {
        return rhs *= lhs;
    }

    struct Point2D {
        Point2D() = default;
        Point2D(double x, double y)
            : x(x)
            , y(y) {
        }

        Point2D& operator+=(const Vec2D& rhs) {
            x += rhs.x;
            y += rhs.y;
            return *this;
        }

        auto operator<=>(const Point2D&) const = default;

        double x = 0;
        double y = 0;
    };

    inline Point2D operator+(Point2D lhs, const Vec2D& rhs) {
        return lhs += rhs;
    }

    inline Point2D operator+(const Vec2D& lhs, Point2D rhs) {
        return rhs += lhs;
    }

}  // namespace geom

namespace collision_detector {

struct CollectionResult {
    bool IsCollected(double collect_radius) const {
        return proj_ratio >= 0 && proj_ratio <= 1 && sq_distance <= collect_radius * collect_radius;
    }

    // квадрат расстояния до точки
    double sq_distance;

    // доля пройденного отрезка
    double proj_ratio;
};

// Движемся из точки a в точку b и пытаемся подобрать точку c.
// Эта функция реализована в уроке.
CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c);

struct Item {
    geom::Point2D position;
    double width;
};

struct Gatherer {
    geom::Point2D start_pos;
    geom::Point2D end_pos;
    double width;
};

class ItemGathererProvider {
protected:
    ~ItemGathererProvider() = default;

public:
    virtual size_t ItemsCount() const = 0;
    virtual Item GetItem(size_t idx) const = 0;
    virtual size_t GatherersCount() const = 0;
    virtual Gatherer GetGatherer(size_t idx) const = 0;
};

struct GatheringEvent {
    size_t item_id;
    size_t gatherer_id;
    double sq_distance;
    double time;
};

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider);

}  // namespace collision_detector
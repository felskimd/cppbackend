#include "collision_detector.h"

#include <stdexcept>

namespace collision_detector {

CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c) {
    // Проверим, что перемещение ненулевое.
    // Тут приходится использовать строгое равенство, а не приближённое,
    // поскольку при сборе заказов придётся учитывать перемещение даже на небольшое
    // расстояние.
    if (b.x == a.x && b.y == a.y) {
        throw std::logic_error("Wrong collision logic");
    }
    const double u_x = c.x - a.x;
    const double u_y = c.y - a.y;
    const double v_x = b.x - a.x;
    const double v_y = b.y - a.y;
    const double u_dot_v = u_x * v_x + u_y * v_y;
    const double u_len2 = u_x * u_x + u_y * u_y;
    const double v_len2 = v_x * v_x + v_y * v_y;
    const double proj_ratio = u_dot_v / v_len2;
    const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;
    return CollectionResult(sq_distance, proj_ratio);
}

// В задании на разработку тестов реализовывать следующую функцию не нужно -
// она будет линковаться извне.
std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider) {
    size_t gatherers = provider.GatherersCount();
    size_t items = provider.ItemsCount();
    std::vector<GatheringEvent> events;
    for (size_t i = 0; i < gatherers; ++i) {
        auto gatherer = provider.GetGatherer(i);
        if (gatherer.start_pos.x == gatherer.end_pos.x && gatherer.start_pos.y == gatherer.end_pos.y) {
            continue;
        }
        for (size_t j = 0; j < items; ++j) {
            auto item = provider.GetItem(j);
            auto collect_result = TryCollectPoint(gatherer.start_pos, gatherer.end_pos, item.position);
            if (collect_result.IsCollected(gatherer.width + item.width)) {
                events.push_back({j, i, collect_result.sq_distance, collect_result.proj_ratio });
            }
        }
    }

    std::sort(events.begin(), events.end(),
        [](const GatheringEvent& e_l, const GatheringEvent& e_r) {
            return e_l.time < e_r.time;
        });
    return events;
}
}  // namespace collision_detector
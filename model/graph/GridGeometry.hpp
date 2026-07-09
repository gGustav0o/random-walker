#pragma once

#include <cassert>

namespace random_walker::graph {

    struct GridPoint {
        int row = 0;
        int column = 0;
        bool operator==(const GridPoint&) const = default;
    };

    [[nodiscard]] constexpr bool is_valid(GridPoint point) noexcept {
        return point.row >= 0 && point.column >= 0;
    }

    [[nodiscard]] constexpr int chebyshev_distance(
        GridPoint first
        , GridPoint second
    ) noexcept {
        assert(is_valid(first));
        assert(is_valid(second));
        const int row_distance = first.row > second.row
            ? first.row - second.row
            : second.row - first.row;
        const int column_distance = first.column > second.column
            ? first.column - second.column
            : second.column - first.column;
        return row_distance > column_distance
            ? row_distance
            : column_distance;
    }
}

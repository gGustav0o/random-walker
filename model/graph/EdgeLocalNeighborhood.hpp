#pragma once

#include "model/domain/RandomWalkerParameters.hpp"
#include "model/graph/GridGeometry.hpp"

#include <cassert>

namespace random_walker::graph {

    struct GridEdge {
        GridPoint first;
        GridPoint second;
        bool operator==(const GridEdge&) const = default;
    };

    [[nodiscard]] constexpr bool is_valid(GridEdge edge) noexcept {
        return is_valid(edge.first)
            && is_valid(edge.second)
            && edge.first != edge.second
            && chebyshev_distance(edge.first, edge.second) == 1;
    }

    [[nodiscard]] constexpr GridPoint doubled_midpoint(
        GridEdge edge
    ) noexcept {
        assert(is_valid(edge));
        return {
            .row = edge.first.row + edge.second.row
            , .column = edge.first.column + edge.second.column
        };
    }

    [[nodiscard]] constexpr bool belongs_to_edge_local_neighborhood(
        GridEdge center
        , GridEdge candidate
        , int radius
    ) noexcept {
        assert(is_valid(center));
        assert(is_valid(candidate));
        assert(radius >= domain::kMinimumEdgeLocalContrastRadius);
        assert(radius <= domain::kMaximumEdgeLocalContrastRadius);

        return chebyshev_distance(
            doubled_midpoint(center)
            , doubled_midpoint(candidate)
        ) <= 2 * radius;
    }
}

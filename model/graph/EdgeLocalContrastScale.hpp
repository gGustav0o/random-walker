#pragma once

#include "model/domain/GrayImage.hpp"
#include "model/domain/RandomWalkerParameters.hpp"
#include "model/graph/EdgeLocalNeighborhood.hpp"

#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace random_walker::graph {

    struct EdgeLocalContrastScaleKey {
        std::uint64_t value = 0;
        bool operator==(const EdgeLocalContrastScaleKey&) const = default;
    };

    struct EdgeLocalContrastScaleKeyHash {
        [[nodiscard]] std::size_t operator()(
            EdgeLocalContrastScaleKey key
        ) const noexcept {
            return std::hash<std::uint64_t> {}(key.value);
        }
    };

    [[nodiscard]] constexpr bool point_is_inside(
        GridPoint point
        , int width
        , int height
    ) noexcept {
        return point.row >= 0
            && point.row < height
            && point.column >= 0
            && point.column < width;
    }

    [[nodiscard]] constexpr bool edge_is_inside(
        GridEdge edge
        , int width
        , int height
    ) noexcept {
        return is_valid(edge)
            && point_is_inside(edge.first, width, height)
            && point_is_inside(edge.second, width, height);
    }

    [[nodiscard]] constexpr int flattened_grid_point(
        GridPoint point
        , int width
    ) noexcept {
        assert(is_valid(point));
        assert(width > 0);
        assert(point.column < width);
        return point.row * width + point.column;
    }

    [[nodiscard]] inline EdgeLocalContrastScaleKey edge_key(
        GridEdge edge
        , int width
    ) noexcept {
        assert(is_valid(edge));
        const auto first = static_cast<std::uint64_t>(
            flattened_grid_point(edge.first, width)
        );
        const auto second = static_cast<std::uint64_t>(
            flattened_grid_point(edge.second, width)
        );
        const std::uint64_t lower = std::min(first, second);
        const std::uint64_t upper = std::max(first, second);
        assert(upper <= UINT32_MAX);
        return EdgeLocalContrastScaleKey {
            .value = (lower << 32U) | upper
        };
    }

    [[nodiscard]] inline double edge_intensity_difference(
        const domain::GrayImage& image
        , GridEdge edge
    ) noexcept {
        assert(!image.empty());
        assert(edge_is_inside(edge, image.width(), image.height()));
        const double first = static_cast<double>(
            image.at(edge.first.row, edge.first.column)
        );
        const double second = static_cast<double>(
            image.at(edge.second.row, edge.second.column)
        );
        return std::abs(first - second);
    }

    [[nodiscard]] inline double edge_difference_quantile(
        std::vector<double> differences
        , double quantile
    ) {
        assert(!differences.empty());
        assert(std::isfinite(quantile));
        assert(quantile >= domain::kMinimumEdgeLocalContrastQuantile);
        assert(quantile <= domain::kMaximumEdgeLocalContrastQuantile);

        std::sort(differences.begin(), differences.end());
        const double position =
            quantile
            * static_cast<double>(differences.size() - std::size_t {1});
        const auto lower_index = static_cast<std::size_t>(std::floor(position));
        const auto upper_index = static_cast<std::size_t>(std::ceil(position));
        assert(lower_index < differences.size());
        assert(upper_index < differences.size());
        const double weight = position - static_cast<double>(lower_index);
        return differences[lower_index] * (1.0 - weight)
            + differences[upper_index] * weight;
    }

    [[nodiscard]] inline std::vector<GridEdge> forward_edges_for_point(
        GridPoint point
        , int width
        , int height
        , domain::PixelConnectivity connectivity
    ) {
        assert(point_is_inside(point, width, height));
        assert(domain::is_valid(connectivity));

        std::vector<GridEdge> edges;
        edges.reserve(connectivity == domain::PixelConnectivity::Eight ? 4U : 2U);

        const auto append_if_inside = [&](int row_delta, int column_delta) {
            const GridPoint neighbor {
                .row = point.row + row_delta
                , .column = point.column + column_delta
            };
            if (point_is_inside(neighbor, width, height)) {
                edges.push_back(GridEdge {
                    .first = point
                    , .second = neighbor
                });
            }
        };

        append_if_inside(0, 1);
        append_if_inside(1, 0);
        if (connectivity == domain::PixelConnectivity::Eight) {
            append_if_inside(1, 1);
            append_if_inside(1, -1);
        }

        return edges;
    }

    [[nodiscard]] inline std::vector<GridEdge> edge_local_neighborhood_edges(
        const domain::GrayImage& image
        , GridEdge center
        , domain::PixelConnectivity connectivity
        , int radius
    ) {
        assert(!image.empty());
        assert(edge_is_inside(center, image.width(), image.height()));
        assert(domain::is_valid(connectivity));
        assert(radius >= domain::kMinimumEdgeLocalContrastRadius);
        assert(radius <= domain::kMaximumEdgeLocalContrastRadius);

        const int first_row = std::max(
            0
            , std::min(center.first.row, center.second.row) - radius - 1
        );
        const int last_row = std::min(
            image.height() - 1
            , std::max(center.first.row, center.second.row) + radius + 1
        );
        const int first_column = std::max(
            0
            , std::min(center.first.column, center.second.column) - radius - 1
        );
        const int last_column = std::min(
            image.width() - 1
            , std::max(center.first.column, center.second.column) + radius + 1
        );

        std::vector<GridEdge> result;
        const int side = 2 * radius + 3;
        result.reserve(
            static_cast<std::size_t>(side)
            * static_cast<std::size_t>(side)
            * (connectivity == domain::PixelConnectivity::Eight ? 4U : 2U)
        );

        for (int row = first_row; row <= last_row; ++row) {
            for (int column = first_column; column <= last_column; ++column) {
                for (const GridEdge edge : forward_edges_for_point(
                    {.row = row, .column = column}
                    , image.width()
                    , image.height()
                    , connectivity
                )) {
                    if (belongs_to_edge_local_neighborhood(
                            center
                            , edge
                            , radius
                        )
                    ) {
                        result.push_back(edge);
                    }
                }
            }
        }

        assert(!result.empty());
        return result;
    }

    [[nodiscard]] inline double estimate_edge_local_contrast_scale(
        const domain::GrayImage& image
        , GridEdge center
        , domain::PixelConnectivity connectivity
        , const domain::EdgeLocalContrastScaleParameters& parameters
    ) {
        assert(!image.empty());
        assert(edge_is_inside(center, image.width(), image.height()));
        assert(domain::is_valid(connectivity));
        assert(domain::is_valid(parameters));

        const std::vector<GridEdge> neighborhood = edge_local_neighborhood_edges(
            image
            , center
            , connectivity
            , parameters.radius
        );
        std::vector<double> differences;
        differences.reserve(neighborhood.size());
        for (const GridEdge edge : neighborhood) {
            differences.push_back(edge_intensity_difference(image, edge));
        }

        const double scale = std::max(
            edge_difference_quantile(std::move(differences), parameters.quantile)
            , parameters.minimum_scale
        );
        assert(std::isfinite(scale));
        assert(scale >= domain::kMinimumEdgeLocalContrastScale);
        assert(scale <= domain::kMaximumEdgeLocalContrastScale);
        return scale;
    }

    class EdgeLocalContrastScaleMap final {
    public:
        using ScaleByEdge = std::unordered_map<
            EdgeLocalContrastScaleKey
            , double
            , EdgeLocalContrastScaleKeyHash
        >;

        EdgeLocalContrastScaleMap() = default;
        EdgeLocalContrastScaleMap(
            int width
            , int height
            , ScaleByEdge scales
        )
            : width_(width)
            , height_(height)
            , scales_(std::move(scales)) {
            assert(width_ > 0);
            assert(height_ > 0);
            for (const auto& [key, scale] : scales_) {
                static_cast<void>(key);
                assert(std::isfinite(scale));
                assert(scale >= domain::kMinimumEdgeLocalContrastScale);
                assert(scale <= domain::kMaximumEdgeLocalContrastScale);
            }
        }

        [[nodiscard]] bool empty() const noexcept {
            return scales_.empty();
        }

        [[nodiscard]] int width() const noexcept {
            return width_;
        }

        [[nodiscard]] int height() const noexcept {
            return height_;
        }

        [[nodiscard]] double scale_at(GridEdge edge) const {
            assert(!empty());
            assert(edge_is_inside(edge, width_, height_));
            const auto position = scales_.find(edge_key(edge, width_));
            assert(position != scales_.end());
            const double scale = position->second;
            assert(std::isfinite(scale));
            assert(scale >= domain::kMinimumEdgeLocalContrastScale);
            assert(scale <= domain::kMaximumEdgeLocalContrastScale);
            return scale;
        }

        [[nodiscard]] const ScaleByEdge& scales() const noexcept {
            return scales_;
        }

    private:
        int width_ = 0;
        int height_ = 0;
        ScaleByEdge scales_;
    };
}

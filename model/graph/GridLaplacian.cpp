#include "GridLaplacian.hpp"

#include "model/algorithm/IterationPolicy.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <Eigen/Core>

namespace random_walker::graph {
    namespace {
        enum class ForwardGridDirection {
            Right
            , Down
        };

        struct GridNodeIndex {
            int value = 0;
        };

        constexpr std::array<ForwardGridDirection, 2> kForwardDirections = {
            ForwardGridDirection::Right
            , ForwardGridDirection::Down
        };

        [[nodiscard]]
        std::pair<int, int> offset(
            ForwardGridDirection direction
        ) noexcept {
            switch (direction) {
            case ForwardGridDirection::Right:
                return { 0, 1 };
            case ForwardGridDirection::Down:
                return { 1, 0 };
            }

            assert(false && "Unhandled grid direction");
            return { 0, 0 };
        }

        [[nodiscard]]
        constexpr bool is_inside(
            int row
            , int column
            , int height
            , int width) noexcept {
            return row >= 0 && row < height && column >= 0 && column < width;
        }

        [[nodiscard]]
        GridNodeIndex flatten(
            int row
            , int column
            , int width
        ) noexcept {
            assert(row >= 0);
            assert(column >= 0);
            assert(width > 0);
            assert(column < width);
            return GridNodeIndex {
                .value = row * width + column
            };
        }

        [[nodiscard]]
        double compute_weight(
            std::uint8_t first_intensity
            , std::uint8_t second_intensity
            , double beta
        ) noexcept {
            assert(std::isfinite(beta));
            assert(beta > 0.0);
            const double difference =
                static_cast<double>(first_intensity)
                - static_cast<double>(second_intensity);
            const double weight = std::exp(-beta * difference * difference);
            assert(std::isfinite(weight));
            assert(weight >= 0.0);
            assert(weight <= 1.0);
            return weight;
        }

        void add_undirected_edge(
            std::vector<Eigen::Triplet<double>>& triplets
            , Eigen::VectorXd& degrees
            , GridNodeIndex first_index
            , GridNodeIndex second_index
            , double weight
        ) {
            assert(first_index.value >= 0);
            assert(static_cast<Eigen::Index>(first_index.value) < degrees.size());
            assert(second_index.value >= 0);
            assert(static_cast<Eigen::Index>(second_index.value) < degrees.size());
            assert(std::isfinite(weight));
            assert(weight >= 0.0);

            triplets.emplace_back(first_index.value, second_index.value, -weight);
            triplets.emplace_back(second_index.value, first_index.value, -weight);
            degrees[first_index.value] += weight;
            degrees[second_index.value] += weight;
        }
    }

    GridLaplacianOutcome build_grid_laplacian(
        const domain::GrayImage& image
        , double beta
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    ) {
        assert(!image.empty());
        assert(std::isfinite(beta));
        assert(beta > 0.0);

        const int height = image.height();
        const int width = image.width();
        assert(height > 0);
        assert(width > 0);
        const int pixel_count = width * height;
        assert(pixel_count > 0);

        std::vector<Eigen::Triplet<double>> triplets;
        triplets.reserve(static_cast<std::size_t>(pixel_count) * 5);

        Eigen::VectorXd degrees = Eigen::VectorXd::Zero(pixel_count);

        progress.report(domain::SegmentationStage::BuildingGraph, 0.0);
        for (int row = 0; row < height; ++row) {
            if (cancellation.stop_requested()) {
                return domain::Cancelled {};
            }

            for (int column = 0; column < width; ++column) {
                if (algorithm::should_poll_cancellation(static_cast<std::size_t>(column))
                    && cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }

                const GridNodeIndex source_index = flatten(row, column, width);
                assert(source_index.value >= 0);
                assert(source_index.value < pixel_count);
                const std::uint8_t source_intensity = image.at(row, column);

                for (const ForwardGridDirection direction : kForwardDirections) {
                    const auto [row_offset, column_offset] = offset(direction);
                    const int neighbor_row = row + row_offset;
                    const int neighbor_column = column + column_offset;

                    if (!is_inside(neighbor_row, neighbor_column, height, width)) {
                        continue;
                    }

                    const GridNodeIndex target_index =
                        flatten(neighbor_row, neighbor_column, width);
                    assert(target_index.value >= 0);
                    assert(target_index.value < pixel_count);
                    const double weight = compute_weight(
                        source_intensity
                        , image.at(neighbor_row, neighbor_column)
                        , beta
                    );

                    add_undirected_edge(
                        triplets
                        , degrees
                        , source_index
                        , target_index
                        , weight
                    );
                }
            }

            progress.report(
                domain::SegmentationStage::BuildingGraph
                , static_cast<double>(row + 1)
                    / static_cast<double>(height)
            );
        }

        for (int index = 0; index < pixel_count; ++index) {
            if (
                algorithm::should_poll_cancellation(static_cast<std::size_t>(index))
                && cancellation.stop_requested()
            ) {
                return domain::Cancelled {};
            }
            const GridNodeIndex diagonal_index {
                .value = index
            };
            assert(diagonal_index.value >= 0);
            assert(static_cast<Eigen::Index>(diagonal_index.value) < degrees.size());
            triplets.emplace_back(
                diagonal_index.value
                , diagonal_index.value
                , degrees[diagonal_index.value]
            );
        }

        if (cancellation.stop_requested()) {
            return domain::Cancelled {};
        }

        Eigen::SparseMatrix<double> laplacian(pixel_count, pixel_count);
        laplacian.setFromTriplets(triplets.begin(), triplets.end());
        assert(laplacian.rows() == pixel_count);
        assert(laplacian.cols() == pixel_count);

        if (cancellation.stop_requested()) {
            return domain::Cancelled {};
        }

        progress.report(domain::SegmentationStage::BuildingGraph, 1.0);
        return laplacian;
    }
}

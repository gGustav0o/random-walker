#include "GridLaplacian.hpp"

#include <array>
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

        [[nodiscard]] constexpr std::pair<int, int> offset(
            ForwardGridDirection direction) noexcept {
            switch (direction) {
            case ForwardGridDirection::Right:
                return { 0, 1 };
            case ForwardGridDirection::Down:
                return { 1, 0 };
            }

            return { 0, 0 };
        }

        [[nodiscard]] constexpr bool is_inside(
            int row
            , int column
            , int height
            , int width) noexcept {
            return row >= 0 && row < height && column >= 0 && column < width;
        }

        [[nodiscard]] constexpr GridNodeIndex flatten(
            int row
            , int column
            , int width) noexcept {
            return GridNodeIndex {
                .value = row * width + column
            };
        }

        [[nodiscard]] double compute_weight(
            std::uint8_t first_intensity
            , std::uint8_t second_intensity
            , double beta) noexcept {
            const double difference =
                static_cast<double>(first_intensity)
                - static_cast<double>(second_intensity);
            return std::exp(-beta * difference * difference);
        }

        void add_undirected_edge(
            std::vector<Eigen::Triplet<double>>& triplets
            , Eigen::VectorXd& degrees
            , GridNodeIndex first_index
            , GridNodeIndex second_index
            , double weight) {
            triplets.emplace_back(first_index.value, second_index.value, -weight);
            triplets.emplace_back(second_index.value, first_index.value, -weight);
            degrees[first_index.value] += weight;
            degrees[second_index.value] += weight;
        }

        [[nodiscard]] GridLaplacianOutcome compute_grid_laplacian(
            const domain::GrayImage& image
            , double beta
            , const domain::CancellationToken& cancellation
            , const domain::ProgressReporter& progress
        ) {
            const int height = image.height();
            const int width = image.width();
            const int pixel_count = width * height;

            std::vector<Eigen::Triplet<double>> triplets;
            triplets.reserve(static_cast<std::size_t>(pixel_count) * 5);

            Eigen::VectorXd degrees = Eigen::VectorXd::Zero(pixel_count);

            progress.report(domain::SegmentationStage::BuildingGraph, 0.0);
            for (int row = 0; row < height; ++row) {
                if (cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }

                for (int column = 0; column < width; ++column) {
                    if ((column & 0x0fff) == 0
                        && cancellation.stop_requested()) {
                        return domain::Cancelled {};
                    }

                    const GridNodeIndex source_index = flatten(row, column, width);
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
                if ((index & 0x0fff) == 0
                    && cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }
                const GridNodeIndex diagonal_index {
                    .value = index
                };
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

            if (cancellation.stop_requested()) {
                return domain::Cancelled {};
            }

            progress.report(domain::SegmentationStage::BuildingGraph, 1.0);
            return laplacian;
        }
    }

    GridLaplacianOutcome build_grid_laplacian(
        const domain::GrayImage& image
        , double beta
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    ) {
        return compute_grid_laplacian(
            image
            , beta
            , cancellation
            , progress
        );
    }
}

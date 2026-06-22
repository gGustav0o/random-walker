#include "PixelGraph.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace random_walker::graph
{
    namespace
    {
        constexpr double kBeta = 0.001;

        enum class Direction
        {
            Right,
            Down,
            Left,
            Up
        };

        constexpr std::array<Direction, 4> kDirections = {
            Direction::Right,
            Direction::Down,
            Direction::Left,
            Direction::Up
        };

        [[nodiscard]] constexpr std::pair<int, int> offset(Direction direction) noexcept
        {
            switch (direction) {
            case Direction::Right:
                return { 0, 1 };
            case Direction::Down:
                return { 1, 0 };
            case Direction::Left:
                return { 0, -1 };
            case Direction::Up:
                return { -1, 0 };
            }

            return { 0, 0 };
        }

        [[nodiscard]] constexpr bool is_inside(
            int row,
            int column,
            int height,
            int width) noexcept
        {
            return row >= 0 && row < height && column >= 0 && column < width;
        }

        [[nodiscard]] double compute_weight(
            std::uint8_t first_intensity,
            std::uint8_t second_intensity) noexcept
        {
            const double difference =
                static_cast<double>(first_intensity) - static_cast<double>(second_intensity);
            return std::exp(-kBeta * difference * difference);
        }

        [[nodiscard]] LaplacianOutcome compute_laplacian(
            const domain::GrayImage& image,
            const domain::CancellationToken& cancellation,
            const domain::ProgressReporter& progress)
        {
            const int height = image.height();
            const int width = image.width();
            const int pixel_count = width * height;

            const auto index_at = [width](int row, int column) noexcept {
                return row * width + column;
            };

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
                    const int source_index = index_at(row, column);
                    const std::uint8_t source_intensity = image.at(row, column);

                    for (const Direction direction : kDirections) {
                        const auto [row_offset, column_offset] = offset(direction);
                        const int neighbor_row = row + row_offset;
                        const int neighbor_column = column + column_offset;

                        if (!is_inside(neighbor_row, neighbor_column, height, width)) {
                            continue;
                        }

                        const int target_index = index_at(neighbor_row, neighbor_column);
                        const double weight = compute_weight(
                            source_intensity,
                            image.at(neighbor_row, neighbor_column));

                        triplets.emplace_back(source_index, target_index, -weight);
                        degrees[source_index] += weight;
                    }
                }

                progress.report(
                    domain::SegmentationStage::BuildingGraph,
                    static_cast<double>(row + 1)
                        / static_cast<double>(height));
            }

            for (int index = 0; index < pixel_count; ++index) {
                if ((index & 0x0fff) == 0
                    && cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }
                triplets.emplace_back(index, index, degrees[index]);
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

    LaplacianOutcome build_laplacian(
        const domain::GrayImage& image,
        const domain::CancellationToken& cancellation,
        const domain::ProgressReporter& progress)
    {
        return compute_laplacian(image, cancellation, progress);
    }
}

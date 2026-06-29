#include "ProbabilityField.hpp"

#include "IterationPolicy.hpp"

#include <cassert>
#include <cstddef>

namespace random_walker::algorithm {
    ProbabilityMapOutcome assemble_probability_map(
        const BoundaryConditions& boundary_conditions
        , const NodePartition& node_partition
        , const Eigen::VectorXd& unknown_values
        , int width
        , int height
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    ) {
        assert(width > 0);
        assert(height > 0);
        assert(
            unknown_values.size()
            == static_cast<Eigen::Index>(node_partition.unknown_pixels.size())
        );
        assert(
            node_partition.unknown_pixels.size()
            == node_partition.unknown_index_by_pixel.size()
        );
        assert(
            node_partition.boundary_pixels.size()
            == node_partition.boundary_index_by_pixel.size()
        );
        assert(
            node_partition.unknown_pixels.size()
                + node_partition.boundary_pixels.size()
            == static_cast<std::size_t>(width * height)
        );
        assert(
            boundary_conditions.pixels.size()
            == boundary_conditions.value_by_pixel.size()
        );

        domain::ProbabilityMap result(height, width);
        const int pixel_count = width * height;

        progress.report(
            domain::SegmentationStage::AssemblingProbabilities
            , 0.0
        );
        for (int index = 0; index < pixel_count; ++index) {
            if (should_poll_cancellation(static_cast<std::size_t>(index))
                && cancellation.stop_requested()) {
                return domain::Cancelled {};
            }
            const int row = index / width;
            const int column = index % width;
            const PixelIndex pixel_index {
                .value = index
            };

            if (boundary_conditions.contains(pixel_index)) {
                result(row, column) = boundary_conditions.value_at(pixel_index);
            } else {
                const auto unknown_position =
                    node_partition.unknown_index_by_pixel.find(pixel_index);
                assert(
                    unknown_position
                    != node_partition.unknown_index_by_pixel.end()
                );
                assert(unknown_position->second.value >= 0);
                assert(
                    static_cast<Eigen::Index>(unknown_position->second.value)
                    < unknown_values.size()
                );
                result(row, column) =
                    unknown_values[unknown_position->second.value];
            }

            if (should_report_progress(static_cast<std::size_t>(index))) {
                progress.report(
                    domain::SegmentationStage::AssemblingProbabilities
                    , static_cast<double>(index + 1)
                        / static_cast<double>(pixel_count)
                );
            }
        }

        progress.report(
            domain::SegmentationStage::AssemblingProbabilities
            , 1.0
        );
        return result;
    }

    BinaryMaskOutcome threshold_probabilities(
        const domain::ProbabilityMap& probabilities
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    ) {
        const int height = static_cast<int>(probabilities.rows());
        const int width = static_cast<int>(probabilities.cols());
        assert(height > 0);
        assert(width > 0);
        domain::BinaryMask mask(height, width);

        progress.report(domain::SegmentationStage::Thresholding, 0.0);
        for (int row = 0; row < height; ++row) {
            if (cancellation.stop_requested()) {
                return domain::Cancelled {};
            }
            for (int column = 0; column < width; ++column) {
                mask(row, column) =
                    probabilities(row, column) >= 0.5 ? 1 : 0;
            }

            progress.report(
                domain::SegmentationStage::Thresholding
                , static_cast<double>(row + 1)
                    / static_cast<double>(height)
            );
        }

        return mask;
    }
}

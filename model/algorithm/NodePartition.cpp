#include "NodePartition.hpp"

#include <cstddef>

namespace random_walker::algorithm {
    NodePartitionOutcome partition_nodes(
        int pixel_count
        , const BoundaryConditions& boundary_conditions
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    ) {
        NodePartition result;
        result.boundary_pixels = boundary_conditions.pixels;
        result.unknown_pixels.reserve(
            static_cast<std::size_t>(pixel_count) - result.boundary_pixels.size()
        );
        result.boundary_index_by_pixel.reserve(result.boundary_pixels.size());

        for (int index = 0; index < pixel_count; ++index) {
            if (
                (index & 0x0fff) == 0
                && cancellation.stop_requested()
            ) {
                return domain::Cancelled {};
            }

            const PixelIndex pixel_index {
                .value = index
            };
            if (boundary_conditions.contains(pixel_index)) {
                continue;
            }

            const UnknownIndex unknown_index {
                .value = static_cast<int>(result.unknown_pixels.size())
            };
            result.unknown_pixels.push_back(pixel_index);
            result.unknown_index_by_pixel.emplace(pixel_index, unknown_index);

            if ((index & 0x0fff) == 0) {
                progress.report(
                    domain::SegmentationStage::PartitioningSystem
                    , 0.25 * static_cast<double>(index + 1)
                        / static_cast<double>(pixel_count)
                );
            }
        }

        progress.report(
            domain::SegmentationStage::PartitioningSystem
            , 0.25
        );

        for (std::size_t index = 0; index < result.boundary_pixels.size(); ++index) {
            if ((index & 0x0fff) == 0
                && cancellation.stop_requested()) {
                return domain::Cancelled {};
            }
            result.boundary_index_by_pixel.emplace(
                result.boundary_pixels[index]
                , BoundaryIndex {
                    .value = static_cast<int>(index)
                }
            );

            if ((index & 0x0fff) == 0) {
                progress.report(
                    domain::SegmentationStage::PartitioningSystem
                    , result.boundary_pixels.empty()
                        ? 0.4
                        : 0.25 + 0.15 * static_cast<double>(index + 1)
                            / static_cast<double>(result.boundary_pixels.size())
                );
            }
        }

        progress.report(
            domain::SegmentationStage::PartitioningSystem
            , 0.4
        );
        return result;
    }
}

#include "BoundaryConditions.hpp"

#include <cassert>
#include <cstddef>

namespace random_walker::algorithm {
    namespace {
        [[nodiscard]] PixelIndex flatten(
            int row
            , int column
            , int width
        ) noexcept {
            assert(row >= 0);
            assert(column >= 0);
            assert(width > 0);
            assert(column < width);
            return PixelIndex {
                .value = row * width + column
            };
        }

        void assign_boundary_value(
            BoundaryConditions& conditions
            , PixelIndex pixel_index
            , double value)
        {
            assert(pixel_index.value >= 0);
            if (!conditions.value_by_pixel.contains(pixel_index)) {
                conditions.pixels.push_back(pixel_index);
            }
            conditions.value_by_pixel[pixel_index] = value;
        }
    }

    BoundaryConditionsOutcome build_boundary_conditions(
        const domain::SegmentationInput& input
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    ) {
        assert(!input.image.empty());
        const int width = input.image.width();
        assert(width > 0);

        BoundaryConditions result;

        std::size_t completed = 0;
        const std::size_t total = input.seeds.size() * 2;
        const auto apply_seeds = [&](domain::SeedLabel label, double value) {
            for (const domain::Seed& seed : input.seeds) {
                if (cancellation.stop_requested()) {
                    return false;
                }

                assert(seed.position.x >= 0);
                assert(seed.position.y >= 0);
                assert(seed.position.x < input.image.width());
                assert(seed.position.y < input.image.height());
                if (seed.label == label) {
                    assign_boundary_value(
                        result
                        , flatten(seed.position.y, seed.position.x, width)
                        , value
                    );
                }

                ++completed;
                if ((completed & 0x0fff) == 0) {
                    progress.report(
                        domain::SegmentationStage::BuildingBoundaryConditions
                        , total == 0
                            ? 1.0
                            : static_cast<double>(completed)
                                / static_cast<double>(total)
                    );
                }
            }
            return true;
        };

        if (
            !apply_seeds(domain::SeedLabel::Background, 0.0)
            || !apply_seeds(domain::SeedLabel::Object, 1.0)
        ) {
            return domain::Cancelled {};
        }

        progress.report(
            domain::SegmentationStage::BuildingBoundaryConditions
            , 1.0
        );
        return result;
    }

    BoundaryValuesOutcome build_boundary_value_vector(
        const BoundaryConditions& boundary_conditions
        , const std::vector<PixelIndex>& boundary_pixels
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    ) {
        assert(boundary_conditions.pixels.size()
            == boundary_conditions.value_by_pixel.size());
        Eigen::VectorXd values(static_cast<int>(boundary_pixels.size()));

        for (std::size_t index = 0; index < boundary_pixels.size(); ++index) {
            if ((index & 0x0fff) == 0
                && cancellation.stop_requested()) {
                return domain::Cancelled {};
            }
            assert(boundary_conditions.contains(boundary_pixels[index]));
            values[static_cast<int>(index)] =
                boundary_conditions.value_at(boundary_pixels[index]);

            if ((index & 0x0fff) == 0) {
                progress.report(
                    domain::SegmentationStage::PartitioningSystem
                    , boundary_pixels.empty()
                        ? 1.0
                        : 0.85 + 0.15 * static_cast<double>(index + 1)
                            / static_cast<double>(boundary_pixels.size())
                );
            }
        }

        progress.report(
            domain::SegmentationStage::PartitioningSystem
            , 1.0
        );
        return values;
    }
}

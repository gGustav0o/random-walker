#include "BoundaryConditions.hpp"

#include "IterationPolicy.hpp"

#include <cassert>
#include <cstddef>
#include <optional>

namespace random_walker::algorithm {
    namespace {

        using BoundaryAssignmentOutcome =
            std::optional<domain::SegmentationError>;

        [[nodiscard]] double boundary_value_for(
            domain::SeedLabel label
        ) noexcept {
            switch (label) {
            case domain::SeedLabel::Background:
                return 0.0;
            case domain::SeedLabel::Object:
                return 1.0;
            }

            assert(false);
            return 0.0;
        }

        [[nodiscard]] BoundaryAssignmentOutcome assign_boundary_value(
            BoundaryConditions& conditions
            , PixelIndex pixel_index
            , double value)
        {
            assert(pixel_index.value >= 0);
            const auto [position, inserted] =
                conditions.value_by_pixel.emplace(pixel_index, value);
            if (inserted) {
                conditions.pixels.push_back(pixel_index);
                return std::nullopt;
            }
            if (position->second != value) {
                return domain::SegmentationError::ConflictingSeedLabels;
            }
            return std::nullopt;
        }

        [[nodiscard]] double boundary_progress_fraction(
            std::size_t completed
            , std::size_t total
        ) noexcept {
            return total == 0
                ? 1.0
                : static_cast<double>(completed) / static_cast<double>(total);
        }

        [[nodiscard]] BoundaryAssignmentOutcome apply_seed(
            BoundaryConditions& result
            , const domain::Seed& seed
            , int image_width
            , int image_height
        ) {
            assert(image_width > 0);
            assert(image_height > 0);
            assert(seed.position.x >= 0);
            assert(seed.position.y >= 0);
            assert(seed.position.x < image_width);
            assert(seed.position.y < image_height);

            const BoundaryAssignmentOutcome assignment_outcome =
                assign_boundary_value(
                    result
                    , flatten_pixel_index(
                        seed.position.y
                        , seed.position.x
                        , image_width
                    )
                    , boundary_value_for(seed.label)
                );
            if (assignment_outcome.has_value()) {
                return *assignment_outcome;
            }

            return std::nullopt;
        }
    }

    BoundaryConditionsOutcome build_boundary_conditions(
        const domain::SegmentationInput& input
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    ) {
        assert(!input.image.empty());
        const int width = input.image.width();
        const int height = input.image.height();
        assert(width > 0);
        assert(height > 0);

        BoundaryConditions result;
        result.pixels.reserve(input.seeds.size());
        result.value_by_pixel.reserve(input.seeds.size());

        for (std::size_t index = 0; index < input.seeds.size(); ++index) {
            if (cancellation.stop_requested()) {
                return domain::Cancelled {};
            }

            if (const BoundaryAssignmentOutcome outcome = apply_seed(
                result
                , input.seeds[index]
                , width
                , height
            ); outcome.has_value()
            ) {
                return *outcome;
            }

            if (should_report_progress(index + 1)) {
                progress.report(
                    domain::SegmentationStage::BuildingBoundaryConditions
                    , boundary_progress_fraction(index + 1, input.seeds.size())
                );
            }
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
            if (should_poll_cancellation(index)
                && cancellation.stop_requested()) {
                return domain::Cancelled {};
            }
            assert(boundary_conditions.contains(boundary_pixels[index]));
            values[static_cast<int>(index)] =
                boundary_conditions.value_at(boundary_pixels[index]);

            if (should_report_progress(index)) {
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

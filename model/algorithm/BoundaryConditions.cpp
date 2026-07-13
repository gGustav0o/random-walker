#include "BoundaryConditions.hpp"

#include "IterationPolicy.hpp"

#include <cassert>
#include <cstddef>
#include <optional>
#include <span>
#include <variant>

namespace random_walker::algorithm {
    namespace {

        using BoundaryAssignmentOutcome =
            std::optional<domain::SegmentationError>;

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

        using BoundaryBuildStepOutcome = std::variant<
            std::monostate,
            domain::SegmentationError,
            domain::Cancelled
        >;

        [[nodiscard]] double boundary_progress_fraction(
            std::size_t completed
            , std::size_t total
        ) noexcept {
            return total == 0
                ? 1.0
                : static_cast<double>(completed) / static_cast<double>(total);
        }

        [[nodiscard]] BoundaryBuildStepOutcome apply_seed_label(
            BoundaryConditions& result
            , std::span<const domain::Seed> seeds
            , domain::SeedLabel label
            , double value
            , int image_width
            , int image_height
            , std::size_t& completed
            , std::size_t total
            , const domain::CancellationToken& cancellation
            , const domain::ProgressReporter& progress
        ) {
            assert(image_width > 0);
            assert(image_height > 0);

            for (const domain::Seed& seed : seeds) {
                if (cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }

                assert(seed.position.x >= 0);
                assert(seed.position.y >= 0);
                assert(seed.position.x < image_width);
                assert(seed.position.y < image_height);
                if (seed.label == label) {
                    const BoundaryAssignmentOutcome assignment_outcome =
                        assign_boundary_value(
                        result
                        , flatten_pixel_index(
                            seed.position.y
                            , seed.position.x
                            , image_width
                        )
                        , value
                    );
                    if (assignment_outcome.has_value()) {
                        return *assignment_outcome;
                    }
                }

                ++completed;
                if (should_report_progress(completed)) {
                    progress.report(
                        domain::SegmentationStage::BuildingBoundaryConditions
                        , boundary_progress_fraction(completed, total)
                    );
                }
            }

            return std::monostate {};
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

        std::size_t completed = 0;
        const std::size_t total = input.seeds.size() * 2;
        if (const BoundaryBuildStepOutcome outcome = apply_seed_label(
                result
                , input.seeds
                , domain::SeedLabel::Background
                , 0.0
                , width
                , height
                , completed
                , total
                , cancellation
                , progress
            ); !std::holds_alternative<std::monostate>(outcome)
        ) {
            if (const auto* error =
                    std::get_if<domain::SegmentationError>(&outcome)) {
                return *error;
            }
            return domain::Cancelled {};
        }

        if (const BoundaryBuildStepOutcome outcome = apply_seed_label(
                result
                , input.seeds
                , domain::SeedLabel::Object
                , 1.0
                , width
                , height
                , completed
                , total
                , cancellation
                , progress
            ); !std::holds_alternative<std::monostate>(outcome)
        ) {
            if (const auto* error =
                    std::get_if<domain::SegmentationError>(&outcome)) {
                return *error;
            }
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

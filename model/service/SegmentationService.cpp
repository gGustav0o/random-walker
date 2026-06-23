#include "SegmentationService.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

#include "model/algorithm/RandomWalkerAlgorithm.hpp"

namespace random_walker::service {
    namespace {
        enum class BoundaryMarker : std::uint8_t {
            Empty
            , Background
            , Object
        };

        struct SeedPixelIndex {
            int value = 0;
        };

        [[nodiscard]] constexpr BoundaryMarker marker_for(
            domain::SeedLabel label) noexcept {
            return label == domain::SeedLabel::Object
                ? BoundaryMarker::Object
                : BoundaryMarker::Background;
        }

        [[nodiscard]] constexpr SeedPixelIndex flatten(
            int row
            , int column
            , int width) noexcept {
            return SeedPixelIndex {
                .value = row * width + column
            };
        }

        [[nodiscard]] bool is_out_of_bounds(
            const domain::PixelRectangle& area
            , const domain::GrayImage& image) noexcept {
            return area.width <= 0
                || area.height <= 0
                || area.x < 0
                || area.y < 0
                || area.x > image.width() - area.width
                || area.y > image.height() - area.height;
        }

        [[nodiscard]] std::optional<domain::SegmentationError>
        mark_seed_region(
            std::vector<BoundaryMarker>& markers
            , const domain::SeedRegion& region
            , int image_width) {
            const BoundaryMarker marker = marker_for(region.label);
            const domain::PixelRectangle& area = region.area;

            for (int row = area.y; row < area.y + area.height; ++row) {
                for (int column = area.x; column < area.x + area.width; ++column) {
                    const SeedPixelIndex pixel_index =
                        flatten(row, column, image_width);
                    BoundaryMarker& current =
                        markers[static_cast<std::size_t>(pixel_index.value)];
                    if (current != BoundaryMarker::Empty
                        && current != marker) {
                        return domain::SegmentationError::ConflictingSeedLabels;
                    }
                    current = marker;
                }
            }

            return std::nullopt;
        }
    }

    std::optional<domain::SegmentationError> SegmentationService::validate(
        const domain::SegmentationRequest& request) {
        if (request.image().empty()) {
            return domain::SegmentationError::EmptyImage;
        }

        if (!domain::is_valid(request.parameters())) {
            return domain::SegmentationError::InvalidBeta;
        }

        bool has_background_seed = false;
        bool has_object_seed = false;
        std::vector<BoundaryMarker> markers(
            static_cast<std::size_t>(request.image().width())
                * static_cast<std::size_t>(request.image().height())
            , BoundaryMarker::Empty
        );

        for (const domain::SeedRegion& region : request.seed_regions()) {
            const domain::PixelRectangle& area = region.area;
            if (is_out_of_bounds(area, request.image())) {
                return domain::SegmentationError::SeedOutOfBounds;
            }

            if (const auto error = mark_seed_region(
                    markers
                    , region
                    , request.image().width()
                ); error.has_value()) {
                return *error;
            }

            has_background_seed =
                has_background_seed
                || region.label == domain::SeedLabel::Background;
            has_object_seed =
                has_object_seed
                || region.label == domain::SeedLabel::Object;
        }

        if (!has_background_seed) {
            return domain::SegmentationError::MissingBackgroundSeeds;
        }
        if (!has_object_seed) {
            return domain::SegmentationError::MissingObjectSeeds;
        }

        return std::nullopt;
    }

    domain::SegmentationOutcome SegmentationService::segment(
        const domain::SegmentationRequest& request
        , domain::CancellationToken cancellation
        , const domain::ProgressReporter& progress) const {
        if (cancellation.stop_requested()) {
            return domain::Cancelled {};
        }

        progress.report(domain::SegmentationStage::ValidatingInput, 0.0);
        if (const auto error = validate(request); error.has_value()) {
            return *error;
        }
        progress.report(domain::SegmentationStage::ValidatingInput, 1.0);

        domain::SeedExpansionOutcome expansion =
            domain::expand_seed_regions(
                request.seed_regions()
                , cancellation
                , progress
            );
        if (std::holds_alternative<domain::Cancelled>(expansion)) {
            return domain::Cancelled {};
        }
        const std::vector<domain::Seed>& seeds =
            std::get<std::vector<domain::Seed>>(expansion);
        const domain::SegmentationInput input {
            request.image()
            , seeds
        };

        return algorithm::detail::run_validated_random_walker(
            input
            , request.parameters().beta
            , cancellation
            , progress
        );
    }
}

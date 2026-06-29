#include "SegmentationService.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

#include "model/algorithm/RandomWalkerAlgorithm.hpp"
#include "model/algorithm/SeedExpansion.hpp"

namespace random_walker::service {
    namespace {
        enum class BoundaryMarker : std::uint8_t {
            Background
            , Object
        };

        struct SeedPixelIndex {
            int value = 0;

            bool operator==(const SeedPixelIndex&) const = default;
        };

        struct SeedPixelIndexHash {
            [[nodiscard]]
            std::size_t operator()(
                SeedPixelIndex index
            ) const noexcept {
                return std::hash<int> {}(index.value);
            }
        };

        using BoundaryMarkerMap =
            std::unordered_map<SeedPixelIndex, BoundaryMarker, SeedPixelIndexHash>;

        [[nodiscard]]
        constexpr BoundaryMarker marker_for(
            domain::SeedLabel label
        ) noexcept {
            return label == domain::SeedLabel::Object
                ? BoundaryMarker::Object
                : BoundaryMarker::Background;
        }

        [[nodiscard]]
        SeedPixelIndex flatten(
            int row
            , int column
            , int width
        ) noexcept {
            assert(row >= 0);
            assert(column >= 0);
            assert(width > 0);
            assert(column < width);
            return SeedPixelIndex {
                .value = row * width + column
            };
        }

        [[nodiscard]]
        bool is_out_of_bounds(
            const domain::PixelRectangle& area
            , const domain::GrayImage& image
        ) noexcept {
            return area.width <= 0
                || area.height <= 0
                || area.x < 0
                || area.y < 0
                || area.x > image.width() - area.width
                || area.y > image.height() - area.height;
        }

        [[nodiscard]]
        std::optional<domain::SegmentationError>
        mark_seed_region(
            BoundaryMarkerMap& markers
            , const domain::SeedRegion& region
            , int image_width
        ) {
            assert(image_width > 0);
            const BoundaryMarker marker = marker_for(region.label);
            const domain::PixelRectangle& area = region.area;
            assert(area.width > 0);
            assert(area.height > 0);
            assert(area.x >= 0);
            assert(area.y >= 0);
            assert(area.x <= image_width - area.width);

            for (int row = area.y; row < area.y + area.height; ++row) {
                for (int column = area.x; column < area.x + area.width; ++column) {
                    const SeedPixelIndex pixel_index =
                        flatten(row, column, image_width);

                    const auto [position, inserted] =
                        markers.emplace(pixel_index, marker);

                    if (!inserted && position->second != marker) {
                        return domain::SegmentationError::ConflictingSeedLabels;
                    }
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

        BoundaryMarkerMap markers;
        markers.reserve(domain::valid_seed_pixel_count(request.seed_regions()));

        for (const domain::SeedRegion& region : request.seed_regions()) {
            const domain::PixelRectangle& area = region.area;
            if (is_out_of_bounds(area, request.image())) {
                return domain::SegmentationError::SeedOutOfBounds;
            }

            if (const auto error = mark_seed_region(
                    markers
                    , region
                    , request.image().width()
                ); error.has_value()
            ) {
                return *error;
            }
        }

        if (!domain::has_seed_label(
                request.seed_regions()
                , domain::SeedLabel::Background
            )
        ) {
            return domain::SegmentationError::MissingBackgroundSeeds;
        }
        if (!domain::has_seed_label(
                request.seed_regions()
                , domain::SeedLabel::Object
            )
        ) {
            return domain::SegmentationError::MissingObjectSeeds;
        }

        return std::nullopt;
    }

    domain::SegmentationOutcome SegmentationService::segment(
        const domain::SegmentationRequest& request
        , domain::CancellationToken cancellation
        , const domain::ProgressReporter& progress
    ) const {
        if (cancellation.stop_requested()) {
            return domain::Cancelled {};
        }

        progress.report(domain::SegmentationStage::ValidatingInput, 0.0);
        if (const auto error = validate(request); error.has_value()) {
            return *error;
        }
        progress.report(domain::SegmentationStage::ValidatingInput, 1.0);

        algorithm::SeedExpansionOutcome expansion =
            algorithm::expand_seed_regions(
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

        return algorithm::run_random_walker(
            input
            , request.parameters().beta
            , cancellation
            , progress
        );
    }
}

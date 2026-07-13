#include "SegmentationConstraintsValidation.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_map>

#include "ImageGeometry.hpp"

namespace random_walker::domain {
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

        struct ConstraintLabelPresence {
            bool background = false;
            bool object = false;
        };

        [[nodiscard]]
        constexpr BoundaryMarker marker_for(
            SeedLabel label
        ) noexcept {
            return label == SeedLabel::Object
                ? BoundaryMarker::Object
                : BoundaryMarker::Background;
        }

        [[nodiscard]]
        constexpr BoundaryMarker marker_for(
            MarkerLabel label
        ) noexcept {
            assert(is_seed_marker_label(label));
            return label == MarkerLabel::Object
                ? BoundaryMarker::Object
                : BoundaryMarker::Background;
        }

        void include_marker(
            ConstraintLabelPresence& presence
            , BoundaryMarker marker
        ) noexcept {
            switch (marker) {
            case BoundaryMarker::Background:
                presence.background = true;
                return;
            case BoundaryMarker::Object:
                presence.object = true;
                return;
            }

            assert(false);
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
                .value = linear_pixel_index(row, column, width)
            };
        }

        [[nodiscard]]
        bool is_out_of_bounds(
            const PixelRectangle& area
            , const GrayImage& image
        ) noexcept {
            return area.width <= 0
                || area.height <= 0
                || area.x < 0
                || area.y < 0
                || area.x > image.width() - area.width
                || area.y > image.height() - area.height;
        }

        [[nodiscard]]
        bool has_expected_marker_mask_geometry(
            const MarkerLabelMask& mask
            , const GrayImage& image
        ) noexcept {
            return mask.empty()
                || (mask.width() == image.width()
                    && mask.height() == image.height());
        }

        [[nodiscard]]
        std::optional<SegmentationError> mark_seed_region(
            BoundaryMarkerMap& markers
            , const SeedRegion& region
            , int image_width
        ) {
            assert(image_width > 0);
            const BoundaryMarker marker = marker_for(region.label);
            const PixelRectangle& area = region.area;
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
                        return SegmentationError::ConflictingSeedLabels;
                    }
                }
            }

            return std::nullopt;
        }

        void include_manual_label_presence(
            ConstraintLabelPresence& presence
            , const BoundaryMarkerMap& markers
        ) noexcept {
            for (const auto& [pixel_index, marker] : markers) {
                static_cast<void>(pixel_index);
                include_marker(presence, marker);
            }
        }

        void include_automatic_label_presence(
            ConstraintLabelPresence& presence
            , const MarkerLabelMask& mask
            , const BoundaryMarkerMap& manual_markers
            , int image_width
        ) {
            if (mask.empty()) {
                return;
            }

            assert(mask.width() == image_width);
            for (int row = 0; row < mask.height(); ++row) {
                for (int column = 0; column < mask.width(); ++column) {
                    const MarkerLabel label = mask.at(row, column);
                    if (!is_seed_marker_label(label)) {
                        continue;
                    }

                    const SeedPixelIndex pixel_index =
                        flatten(row, column, image_width);
                    if (manual_markers.contains(pixel_index)) {
                        continue;
                    }

                    include_marker(presence, marker_for(label));
                }
            }
        }

        [[nodiscard]]
        ConstraintLabelPresence constraint_label_presence(
            const BoundaryMarkerMap& manual_markers
            , const MarkerLabelMask& automatic_markers
            , int image_width
        ) {
            ConstraintLabelPresence presence;
            include_manual_label_presence(presence, manual_markers);
            include_automatic_label_presence(
                presence
                , automatic_markers
                , manual_markers
                , image_width
            );
            return presence;
        }
    }

    std::optional<SegmentationError> validate(
        const SegmentationConstraints& constraints
        , const GrayImage& image
    ) {
        if (!has_expected_marker_mask_geometry(
                constraints.automatic_markers
                , image
            )
        ) {
            return SegmentationError::SeedOutOfBounds;
        }

        BoundaryMarkerMap manual_markers;
        manual_markers.reserve(
            valid_seed_pixel_count(constraints.manual_seed_regions)
        );

        for (const SeedRegion& region : constraints.manual_seed_regions) {
            const PixelRectangle& area = region.area;
            if (is_out_of_bounds(area, image)) {
                return SegmentationError::SeedOutOfBounds;
            }

            if (const auto error = mark_seed_region(
                    manual_markers
                    , region
                    , image.width()
                ); error.has_value()
            ) {
                return *error;
            }
        }

        const ConstraintLabelPresence presence =
            constraint_label_presence(
                manual_markers
                , constraints.automatic_markers
                , image.width()
            );
        if (!presence.background) {
            return SegmentationError::MissingBackgroundSeeds;
        }
        if (!presence.object) {
            return SegmentationError::MissingObjectSeeds;
        }

        return std::nullopt;
    }
}

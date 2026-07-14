#include "SegmentationConstraintsBuilder.hpp"

namespace random_walker::viewmodel {
    domain::SegmentationConstraints make_segmentation_constraints(
        std::span<const domain::SeedRegion> manual_seed_regions
        , const domain::MarkerLabelMask* automatic_markers
    ) {
        domain::SegmentationConstraints result {
            .manual_seed_regions = {
                manual_seed_regions.begin()
                , manual_seed_regions.end()
            }
        };
        if (automatic_markers != nullptr) {
            result.automatic_markers = *automatic_markers;
        }

        return result;
    }

    ConstraintCounts count_constraints(
        const domain::SegmentationConstraints& constraints
    ) noexcept {
        return count_constraints(
            constraints.manual_seed_regions
            , &constraints.automatic_markers
        );
    }

    ConstraintCounts count_constraints(
        std::span<const domain::SeedRegion> manual_seed_regions
        , const domain::MarkerLabelMask* automatic_markers
    ) noexcept {
        const domain::MarkerLabelMask empty_automatic_markers;
        const domain::MarkerLabelMask& marker_mask =
            automatic_markers != nullptr
                ? *automatic_markers
                : empty_automatic_markers;

        return {
            .background =
                domain::seed_pixel_count(
                    manual_seed_regions
                    , domain::SeedLabel::Background
                )
                + domain::marker_pixel_count(
                    marker_mask
                    , domain::SeedLabel::Background
                )
            , .object =
                domain::seed_pixel_count(
                    manual_seed_regions
                    , domain::SeedLabel::Object
                )
                + domain::marker_pixel_count(
                    marker_mask
                    , domain::SeedLabel::Object
                )
        };
    }

    bool has_required_constraints(
        ConstraintCounts counts
    ) noexcept {
        return counts.background > 0 && counts.object > 0;
    }
}

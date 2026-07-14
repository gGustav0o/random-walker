#pragma once

#include <span>

#include "model/domain/AutoMarkers.hpp"
#include "model/domain/Seed.hpp"
#include "model/domain/SegmentationConstraints.hpp"

namespace random_walker::viewmodel {
    struct ConstraintCounts {
        int background = 0;
        int object = 0;

        bool operator==(const ConstraintCounts&) const = default;
    };

    [[nodiscard]] domain::SegmentationConstraints make_segmentation_constraints(
        std::span<const domain::SeedRegion> manual_seed_regions
        , const domain::MarkerLabelMask* automatic_markers
    );

    [[nodiscard]] ConstraintCounts count_constraints(
        const domain::SegmentationConstraints& constraints
    ) noexcept;

    [[nodiscard]] ConstraintCounts count_constraints(
        std::span<const domain::SeedRegion> manual_seed_regions
        , const domain::MarkerLabelMask* automatic_markers
    ) noexcept;

    [[nodiscard]] bool has_required_constraints(
        ConstraintCounts counts
    ) noexcept;
}

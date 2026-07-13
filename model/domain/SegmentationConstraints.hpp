#pragma once

#include <vector>

#include "AutoMarkers.hpp"
#include "Seed.hpp"

namespace random_walker::domain {

    struct SegmentationConstraints {
        std::vector<SeedRegion> manual_seed_regions;
        MarkerLabelMask automatic_markers;

        bool operator==(const SegmentationConstraints&) const = default;
    };
}

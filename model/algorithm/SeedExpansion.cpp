#include "SeedExpansion.hpp"

#include <cstddef>

namespace random_walker::algorithm {
    SeedExpansionOutcome expand_seed_regions(
        std::span<const domain::SeedRegion> regions
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    ) {
        std::size_t seed_count = 0;
        for (const domain::SeedRegion& region : regions) {
            if (cancellation.stop_requested()) {
                return domain::Cancelled {};
            }
            seed_count += static_cast<std::size_t>(region.area.width)
                * static_cast<std::size_t>(region.area.height);
        }

        std::vector<domain::Seed> result;
        result.reserve(seed_count);
        std::size_t expanded_count = 0;
        progress.report(domain::SegmentationStage::ExpandingSeeds, 0.0);

        for (const domain::SeedRegion& region : regions) {
            if (cancellation.stop_requested()) {
                return domain::Cancelled {};
            }
            for (int row = region.area.y;
                 row < region.area.y + region.area.height;
                 ++row) {
                if (cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }
                for (int column = region.area.x;
                     column < region.area.x + region.area.width;
                     ++column) {
                    if ((column & 0x0fff) == 0
                        && cancellation.stop_requested()) {
                        return domain::Cancelled {};
                    }
                    result.push_back({
                        .position = { .x = column, .y = row }
                        , .label = region.label
                    });
                    ++expanded_count;
                }

                progress.report(
                    domain::SegmentationStage::ExpandingSeeds
                    , seed_count == 0
                        ? 1.0
                        : static_cast<double>(expanded_count)
                            / static_cast<double>(seed_count)
                );
            }
        }

        progress.report(domain::SegmentationStage::ExpandingSeeds, 1.0);
        return result;
    }
}

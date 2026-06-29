#include "SeedExpansion.hpp"

#include "IterationPolicy.hpp"

#include <cassert>
#include <cstddef>
#include <variant>

namespace random_walker::algorithm {
    namespace {
        using SeedCountOutcome = std::variant<std::size_t, domain::Cancelled>;

        [[nodiscard]] std::size_t seed_pixel_count(
            const domain::SeedRegion& region
        ) noexcept {
            assert(region.area.width > 0);
            assert(region.area.height > 0);
            return static_cast<std::size_t>(region.area.width)
                * static_cast<std::size_t>(region.area.height);
        }

        [[nodiscard]] SeedCountOutcome count_seed_pixels(
            std::span<const domain::SeedRegion> regions
            , const domain::CancellationToken& cancellation
        ) {
            std::size_t result = 0;
            for (const domain::SeedRegion& region : regions) {
                if (cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }
                result += seed_pixel_count(region);
            }

            return result;
        }
    }

    SeedExpansionOutcome expand_seed_regions(
        std::span<const domain::SeedRegion> regions
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    ) {
        const SeedCountOutcome seed_count_outcome = count_seed_pixels(
            regions
            , cancellation
        );
        if (std::holds_alternative<domain::Cancelled>(seed_count_outcome)) {
            return domain::Cancelled {};
        }
        const std::size_t seed_count = std::get<std::size_t>(seed_count_outcome);

        std::vector<domain::Seed> result;
        result.reserve(seed_count);
        std::size_t expanded_count = 0;
        progress.report(domain::SegmentationStage::ExpandingSeeds, 0.0);

        for (const domain::SeedRegion& region : regions) {
            if (cancellation.stop_requested()) {
                return domain::Cancelled {};
            }
            assert(region.area.width > 0);
            assert(region.area.height > 0);
            assert(region.area.x >= 0);
            assert(region.area.y >= 0);
            for (int row = region.area.y;
                 row < region.area.y + region.area.height;
                 ++row
            ) {
                if (cancellation.stop_requested()) {
                    return domain::Cancelled {};
                }
                for (int column = region.area.x;
                     column < region.area.x + region.area.width;
                     ++column
                ) {
                    if (should_poll_cancellation(static_cast<std::size_t>(column))
                        && cancellation.stop_requested()
                    ) {
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

        assert(result.size() == seed_count);
        progress.report(domain::SegmentationStage::ExpandingSeeds, 1.0);
        return result;
    }
}

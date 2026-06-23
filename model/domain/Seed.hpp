#pragma once

#include <cstddef>
#include <span>
#include <variant>
#include <vector>

#include "Cancellation.hpp"
#include "ProgressReporter.hpp"

namespace random_walker::domain {
    struct PixelCoordinate {
        int x = 0;
        int y = 0;

        bool operator==(const PixelCoordinate&) const = default;
    };

    enum class SeedLabel {
        Background
        , Object
    };

    struct PixelRectangle {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;

        bool operator==(const PixelRectangle&) const = default;
    };

    struct SeedRegion {
        PixelRectangle area;
        SeedLabel label = SeedLabel::Background;
    };

    struct Seed {
        PixelCoordinate position;
        SeedLabel label = SeedLabel::Background;
    };

    [[nodiscard]] inline int seed_pixel_count(
        std::span<const SeedRegion> regions
        , SeedLabel label) noexcept {
        int result = 0;
        for (const SeedRegion& region : regions) {
            if (region.label == label) {
                result += region.area.width * region.area.height;
            }
        }
        return result;
    }

    using SeedExpansionOutcome = std::variant<std::vector<Seed>, Cancelled>;

    [[nodiscard]] inline SeedExpansionOutcome expand_seed_regions(
        std::span<const SeedRegion> regions
        , const CancellationToken& cancellation
        , const ProgressReporter& progress
    ) {
        std::size_t seed_count = 0;
        for (const SeedRegion& region : regions) {
            if (cancellation.stop_requested()) {
                return Cancelled {};
            }
            seed_count += static_cast<std::size_t>(region.area.width)
                * static_cast<std::size_t>(region.area.height);
        }

        std::vector<Seed> result;
        result.reserve(seed_count);
        std::size_t expanded_count = 0;
        progress.report(SegmentationStage::ExpandingSeeds, 0.0);

        for (const SeedRegion& region : regions) {
            if (cancellation.stop_requested()) {
                return Cancelled {};
            }
            for (int row = region.area.y;
                 row < region.area.y + region.area.height;
                 ++row) {
                if (cancellation.stop_requested()) {
                    return Cancelled {};
                }
                for (int column = region.area.x;
                     column < region.area.x + region.area.width;
                     ++column) {
                    if ((column & 0x0fff) == 0
                        && cancellation.stop_requested()) {
                        return Cancelled {};
                    }
                    result.push_back({
                        .position = { .x = column, .y = row }
                        , .label = region.label
                    });
                    ++expanded_count;
                }

                progress.report(
                    SegmentationStage::ExpandingSeeds
                    , seed_count == 0
                        ? 1.0
                        : static_cast<double>(expanded_count)
                            / static_cast<double>(seed_count));
            }
        }

        progress.report(SegmentationStage::ExpandingSeeds, 1.0);
        return result;
    }
}

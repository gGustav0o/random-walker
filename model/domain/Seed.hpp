#pragma once

#include <cstddef>
#include <span>
#include <variant>
#include <vector>

#include "Cancellation.hpp"

namespace random_walker::domain
{
    struct PixelCoordinate
    {
        int x = 0;
        int y = 0;

        bool operator==(const PixelCoordinate&) const = default;
    };

    enum class SeedLabel
    {
        Background,
        Object
    };

    struct PixelRectangle
    {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;

        bool operator==(const PixelRectangle&) const = default;
    };

    struct SeedRegion
    {
        PixelRectangle area;
        SeedLabel label = SeedLabel::Background;
    };

    struct Seed
    {
        PixelCoordinate position;
        SeedLabel label = SeedLabel::Background;
    };

    [[nodiscard]] inline int seed_pixel_count(
        std::span<const SeedRegion> regions,
        SeedLabel label) noexcept
    {
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
        std::span<const SeedRegion> regions,
        const CancellationToken& cancellation)
    {
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
                        .position = { .x = column, .y = row },
                        .label = region.label
                    });
                }
            }
        }

        return result;
    }
}

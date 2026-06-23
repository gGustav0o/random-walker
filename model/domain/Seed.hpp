#pragma once

#include <span>

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
}

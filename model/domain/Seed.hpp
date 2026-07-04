#pragma once

#include <algorithm>
#include <cstddef>
#include <numeric>
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

    enum class SeedSource {
        User
        , Automatic
    };


    [[nodiscard]] constexpr bool is_user_seed(
        SeedSource source
    ) noexcept {
        return source == SeedSource::User;
    }

    [[nodiscard]] constexpr bool is_automatic_seed(
        SeedSource source
    ) noexcept {
        return source == SeedSource::Automatic;
    }

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
        SeedSource source = SeedSource::User;
        double confidence = 1.0;

        bool operator==(const SeedRegion&) const = default;
    };

    struct Seed {
        PixelCoordinate position;
        SeedLabel label = SeedLabel::Background;
        SeedSource source = SeedSource::User;
        double confidence = 1.0;

        bool operator==(const Seed&) const = default;
    };

    [[nodiscard]] inline bool has_seed_label(
        std::span<const SeedRegion> regions
        , SeedLabel label
    ) noexcept {
        return std::any_of(
            regions.begin()
            , regions.end()
            , [label](const SeedRegion& region) {
                return region.label == label;
            }
        );
    }

    [[nodiscard]] inline int seed_pixel_count(
        std::span<const SeedRegion> regions
        , SeedLabel label
    ) noexcept {
        return std::accumulate(
            regions.begin()
            , regions.end()
            , 0
            , [label](int result, const SeedRegion& region) {
                if (region.label != label) {
                    return result;
                }

                return result + region.area.width * region.area.height;
            }
        );
    }

    [[nodiscard]] inline std::size_t valid_seed_pixel_count(
        std::span<const SeedRegion> regions
    ) noexcept {
        return std::accumulate(
            regions.begin()
            , regions.end()
            , std::size_t {0}
            , [](std::size_t result, const SeedRegion& region) {
                if (region.area.width <= 0 || region.area.height <= 0) {
                    return result;
                }

                return result + static_cast<std::size_t>(region.area.width)
                    * static_cast<std::size_t>(region.area.height);
            }
        );
    }
}

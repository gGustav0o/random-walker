#pragma once

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

    struct Seed
    {
        PixelCoordinate position;
        SeedLabel label = SeedLabel::Background;
    };
}

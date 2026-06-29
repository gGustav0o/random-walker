#pragma once

#include <cassert>
#include <cstddef>
#include <functional>

namespace random_walker::algorithm {
    struct PixelIndex {
        int value = 0;

        bool operator==(const PixelIndex&) const = default;
    };

    struct UnknownIndex {
        int value = 0;

        bool operator==(const UnknownIndex&) const = default;
    };

    struct BoundaryIndex {
        int value = 0;

        bool operator==(const BoundaryIndex&) const = default;
    };

    [[nodiscard]] inline PixelIndex flatten_pixel_index(
        int row
        , int column
        , int width
    ) noexcept {
        assert(row >= 0);
        assert(column >= 0);
        assert(width > 0);
        assert(column < width);
        return PixelIndex {
            .value = row * width + column
        };
    }

    struct PixelIndexHash {
        [[nodiscard]] std::size_t operator()(PixelIndex index) const noexcept {
            return std::hash<int> {}(index.value);
        }
    };
}

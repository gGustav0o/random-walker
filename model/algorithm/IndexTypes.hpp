#pragma once

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

    struct PixelIndexHash {
        [[nodiscard]] std::size_t operator()(PixelIndex index) const noexcept {
            return std::hash<int> {}(index.value);
        }
    };
}

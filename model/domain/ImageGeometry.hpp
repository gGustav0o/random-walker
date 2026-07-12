#pragma once

#include <cassert>
#include <cstddef>
#include <limits>

namespace random_walker::domain {

    inline constexpr int kMaximumSupportedImagePixels =
        std::numeric_limits<int>::max();

    [[nodiscard]] inline bool has_representable_pixel_count(
        int width
        , int height
    ) noexcept {
        if (width < 0 || height < 0) {
            return false;
        }

        if (width == 0 || height == 0) {
            return true;
        }

        return static_cast<std::size_t>(width)
            <= static_cast<std::size_t>(kMaximumSupportedImagePixels)
                / static_cast<std::size_t>(height);
    }

    [[nodiscard]] inline bool is_supported_non_empty_image_geometry(
        int width
        , int height
    ) noexcept {
        return width > 0
            && height > 0
            && has_representable_pixel_count(width, height);
    }

    [[nodiscard]] inline std::size_t pixel_count(
        int width
        , int height
    ) noexcept {
        assert(width >= 0);
        assert(height >= 0);
        assert(has_representable_pixel_count(width, height));

        return static_cast<std::size_t>(width)
            * static_cast<std::size_t>(height);
    }

    [[nodiscard]] inline int pixel_count_as_int(
        int width
        , int height
    ) noexcept {
        assert(is_supported_non_empty_image_geometry(width, height));

        return static_cast<int>(pixel_count(width, height));
    }

    [[nodiscard]] inline int linear_pixel_index(
        int row
        , int column
        , int width
    ) noexcept {
        assert(row >= 0);
        assert(column >= 0);
        assert(width > 0);
        assert(column < width);

        const std::size_t index =
            static_cast<std::size_t>(row) * static_cast<std::size_t>(width)
            + static_cast<std::size_t>(column);
        assert(index <= static_cast<std::size_t>(kMaximumSupportedImagePixels));

        return static_cast<int>(index);
    }
}

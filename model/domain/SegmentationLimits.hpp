#pragma once

#include <cstddef>

#include "ImageGeometry.hpp"

namespace random_walker::domain {

    inline constexpr int kMaximumRandomWalkerSolvablePixels = 1024 * 1024;

    [[nodiscard]] inline bool is_supported_segmentation_image_geometry(
        int width
        , int height
    ) noexcept {
        return is_supported_non_empty_image_geometry(width, height)
            && pixel_count(width, height)
                <= static_cast<std::size_t>(kMaximumRandomWalkerSolvablePixels);
    }
}

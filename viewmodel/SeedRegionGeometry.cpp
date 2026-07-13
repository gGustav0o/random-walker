#include "SeedRegionGeometry.hpp"

#include <algorithm>

namespace random_walker::viewmodel {

    std::optional<domain::PixelRectangle> clipped_seed_rectangle(
        int x
        , int y
        , int width
        , int height
        , int image_width
        , int image_height
    ) noexcept {
        if (width <= 0 || height <= 0) {
            return std::nullopt;
        }

        const int left = std::clamp(x, 0, image_width);
        const int top = std::clamp(y, 0, image_height);
        const auto right_edge = static_cast<long long>(x) + width;
        const auto bottom_edge = static_cast<long long>(y) + height;
        const int right = static_cast<int>(std::clamp(
            right_edge
            , 0LL
            , static_cast<long long>(image_width))
        );
        const int bottom = static_cast<int>(std::clamp(
            bottom_edge
            , 0LL
            , static_cast<long long>(image_height))
        );

        if (left >= right || top >= bottom) {
            return std::nullopt;
        }

        return domain::PixelRectangle {
            .x = left
            , .y = top
            , .width = right - left
            , .height = bottom - top
        };
    }
}

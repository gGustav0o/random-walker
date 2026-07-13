#pragma once

#include <optional>

#include "model/domain/Seed.hpp"

namespace random_walker::viewmodel {

    [[nodiscard]] std::optional<domain::PixelRectangle>
    clipped_seed_rectangle(
        int x
        , int y
        , int width
        , int height
        , int image_width
        , int image_height
    ) noexcept;
}

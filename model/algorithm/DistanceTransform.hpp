#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace random_walker::algorithm {

    [[nodiscard]] std::vector<double> squared_distance_transform_1d(
        std::span<const double> samples
    );

    [[nodiscard]] std::vector<double> squared_euclidean_distance_transform(
        int width
        , int height
        , std::span<const std::uint8_t> source_mask
    );
}

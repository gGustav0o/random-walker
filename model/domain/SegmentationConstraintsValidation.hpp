#pragma once

#include <optional>

#include "GrayImage.hpp"
#include "Segmentation.hpp"

namespace random_walker::domain {

    [[nodiscard]] std::optional<SegmentationError> validate(
        const SegmentationConstraints& constraints
        , const GrayImage& image
    );
}

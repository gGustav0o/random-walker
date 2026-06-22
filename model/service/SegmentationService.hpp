#pragma once

#include <optional>

#include "model/domain/Segmentation.hpp"

namespace random_walker::service
{
    class SegmentationService final
    {
    public:
        [[nodiscard]] domain::SegmentationOutcome segment(
            const domain::SegmentationInput& input) const;

        [[nodiscard]] static std::optional<domain::SegmentationError> validate(
            const domain::SegmentationInput& input) noexcept;
    };
}

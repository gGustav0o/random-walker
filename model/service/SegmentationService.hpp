#pragma once

#include <optional>

#include "model/domain/ProgressReporter.hpp"
#include "model/domain/Segmentation.hpp"

namespace random_walker::service {
    class SegmentationService final {
    public:
        [[nodiscard]] domain::SegmentationOutcome segment(
            const domain::SegmentationRequest& request
            , domain::CancellationToken cancellation = {}
            , const domain::ProgressReporter& progress = {}
        ) const;

        [[nodiscard]] static std::optional<domain::SegmentationError> validate(
            const domain::SegmentationRequest& request) noexcept;
    };
}

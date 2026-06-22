#pragma once

#include "model/domain/Segmentation.hpp"

namespace random_walker::algorithm::detail
{
    // Precondition: the input has been validated by SegmentationService.
    [[nodiscard]] domain::SegmentationOutcome run_validated_random_walker(
        const domain::SegmentationInput& input,
        const domain::CancellationToken& cancellation);
}

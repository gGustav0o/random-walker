#pragma once

#include "model/domain/ProgressReporter.hpp"
#include "model/domain/Segmentation.hpp"

namespace random_walker::algorithm {
    // Precondition: the input has been validated by SegmentationService.
    [[nodiscard]] domain::SegmentationOutcome run_random_walker(
        const domain::SegmentationInput& input
        , const domain::RandomWalkerParameters& parameters
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    );
}

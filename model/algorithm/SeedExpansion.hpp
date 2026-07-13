#pragma once

#include <variant>
#include <vector>

#include "model/domain/Cancellation.hpp"
#include "model/domain/ProgressReporter.hpp"
#include "model/domain/Seed.hpp"
#include "model/domain/SegmentationConstraints.hpp"

namespace random_walker::algorithm {

    using SeedExpansionOutcome = std::variant<
        std::vector<domain::Seed>
        , domain::Cancelled
    >;

    [[nodiscard]] SeedExpansionOutcome expand_seed_constraints(
        const domain::SegmentationConstraints& constraints
        , int image_width
        , int image_height
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    );
}

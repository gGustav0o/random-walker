#pragma once

#include <span>
#include <variant>
#include <vector>

#include "model/domain/Cancellation.hpp"
#include "model/domain/ProgressReporter.hpp"
#include "model/domain/Seed.hpp"

namespace random_walker::algorithm {
    using SeedExpansionOutcome = std::variant<
        std::vector<domain::Seed>
        , domain::Cancelled
    >;

    [[nodiscard]] SeedExpansionOutcome expand_seed_regions(
        std::span<const domain::SeedRegion> regions
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    );
}

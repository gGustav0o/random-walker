#pragma once

#include <span>
#include <variant>

#include "application/error/UserError.hpp"
#include "model/domain/AutoMarkers.hpp"
#include "model/domain/GrayImage.hpp"
#include "model/domain/Seed.hpp"

namespace random_walker::application {
    using AutoMarkerProposalOutcome =
        std::variant<domain::MarkerProposal, AutoMarkerError>;

    class AutoMarkerService final {
    public:
        [[nodiscard]] AutoMarkerProposalOutcome propose(
            const domain::GrayImage& image
            , const domain::AutoMarkerParameters& parameters
            , std::span<const domain::SeedRegion> manual_seed_regions
        ) const;
    };
}

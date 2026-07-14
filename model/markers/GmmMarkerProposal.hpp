#pragma once

#include "model/domain/AutoMarkers.hpp"
#include "model/domain/GrayImage.hpp"

#include <span>
#include <variant>

namespace random_walker::markers {

    enum class MarkerProposalError {
        EmptyImage
        , InvalidParameters
        , GmmFitFailed
    };

    using MarkerProposalOutcome =
        std::variant<domain::MarkerProposal, MarkerProposalError>;

    [[nodiscard]] MarkerProposalOutcome propose_gmm_markers(
        const domain::GrayImage& image
        , const domain::AutoMarkerParameters& parameters
    );

    [[nodiscard]] domain::MarkerProposal remove_manual_conflicts(
        domain::MarkerProposal proposal
        , std::span<const domain::SeedRegion> manual_regions
    );
}

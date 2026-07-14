#pragma once

#include <variant>

#include "PartitionedLaplacian.hpp"
#include "model/domain/Cancellation.hpp"
#include "model/domain/Segmentation.hpp"

namespace random_walker::algorithm {
    struct DirichletAnchoringValidated {
        bool operator==(const DirichletAnchoringValidated&) const = default;
    };

    using DirichletAnchoringOutcome = std::variant<
        DirichletAnchoringValidated
        , domain::SegmentationError
        , domain::Cancelled
    >;

    [[nodiscard]] DirichletAnchoringOutcome validate_dirichlet_anchoring(
        const PartitionedLaplacian& laplacian
        , const domain::CancellationToken& cancellation
    );
}

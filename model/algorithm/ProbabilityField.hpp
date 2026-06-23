#pragma once

#include <variant>

#include <Eigen/Core>

#include "BoundaryConditions.hpp"
#include "NodePartition.hpp"
#include "model/domain/Cancellation.hpp"
#include "model/domain/ProgressReporter.hpp"
#include "model/domain/Segmentation.hpp"

namespace random_walker::algorithm {
    using ProbabilityMapOutcome =
        std::variant<domain::ProbabilityMap, domain::Cancelled>;

    using BinaryMaskOutcome =
        std::variant<domain::BinaryMask, domain::Cancelled>;

    [[nodiscard]] ProbabilityMapOutcome assemble_probability_map(
        const BoundaryConditions& boundary_conditions
        , const NodePartition& node_partition
        , const Eigen::VectorXd& unknown_values
        , int width
        , int height
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    );

    [[nodiscard]] BinaryMaskOutcome threshold_probabilities(
        const domain::ProbabilityMap& probabilities
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    );
}

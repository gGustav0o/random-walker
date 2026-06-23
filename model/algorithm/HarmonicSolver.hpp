#pragma once

#include <variant>

#include <Eigen/Core>

#include "PartitionedLaplacian.hpp"
#include "model/domain/Cancellation.hpp"
#include "model/domain/ProgressReporter.hpp"
#include "model/domain/Segmentation.hpp"

namespace random_walker::algorithm {
    using HarmonicSolveOutcome = std::variant<
        Eigen::VectorXd
        , domain::SegmentationError
        , domain::Cancelled
    >;

    [[nodiscard]] HarmonicSolveOutcome solve_harmonic_dirichlet_problem(
        const PartitionedLaplacian& laplacian
        , const Eigen::VectorXd& boundary_values
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    );
}

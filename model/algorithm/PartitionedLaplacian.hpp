#pragma once

#include <variant>

#include <Eigen/Sparse>

#include "NodePartition.hpp"
#include "model/domain/Cancellation.hpp"
#include "model/domain/ProgressReporter.hpp"

namespace random_walker::algorithm {
    struct PartitionedLaplacian {
        Eigen::SparseMatrix<double> unknown_unknown_block;
        Eigen::SparseMatrix<double> unknown_boundary_block;
    };

    using PartitionedLaplacianOutcome =
        std::variant<PartitionedLaplacian, domain::Cancelled>;

    [[nodiscard]] PartitionedLaplacianOutcome partition_laplacian(
        const Eigen::SparseMatrix<double>& laplacian
        , const NodePartition& node_partition
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    );
}

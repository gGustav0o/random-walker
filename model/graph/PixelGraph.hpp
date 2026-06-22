#pragma once

#include <variant>

#include <Eigen/Sparse>

#include "model/domain/Cancellation.hpp"
#include "model/domain/GrayImage.hpp"

namespace random_walker::graph
{
    using LaplacianOutcome =
        std::variant<Eigen::SparseMatrix<double>, domain::Cancelled>;

    [[nodiscard]] LaplacianOutcome build_laplacian(
        const domain::GrayImage& image,
        const domain::CancellationToken& cancellation);
}

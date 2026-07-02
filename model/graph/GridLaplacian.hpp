#pragma once

#include <variant>

#include <Eigen/Sparse>

#include "model/domain/Cancellation.hpp"
#include "model/domain/GrayImage.hpp"
#include "model/domain/ProgressReporter.hpp"
#include "model/domain/RandomWalkerParameters.hpp"

namespace random_walker::graph {
    using GridLaplacianOutcome =
        std::variant<Eigen::SparseMatrix<double>, domain::Cancelled>;

    [[nodiscard]] GridLaplacianOutcome build_grid_laplacian(
        const domain::GrayImage& image
        , double beta
        , domain::PixelConnectivity connectivity
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    );
}

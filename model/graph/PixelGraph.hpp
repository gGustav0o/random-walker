#pragma once

#include <Eigen/Sparse>

#include "model/domain/GrayImage.hpp"

namespace random_walker::graph
{
    [[nodiscard]] Eigen::SparseMatrix<double> build_laplacian(
        const domain::GrayImage& image);
}

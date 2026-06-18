#pragma once

#include <Eigen/Core>
#include <Eigen/Sparse>

#include <QPoint>
#include <vector>

#include "../../MetaAnnotations.hpp"

namespace algorithm
{
    [[nodiscard]] Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic> run_random_walker(
        const Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic>& image,
        const std::vector<QPoint>& background_seeds,
        const std::vector<QPoint>& object_seeds);

    [[nodiscard]] Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> run_random_walker_probabilities(
        const Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic>& image,
        const std::vector<QPoint>& background_seeds,
        const std::vector<QPoint>& object_seeds);

} // namespace algorithm

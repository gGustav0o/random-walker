#pragma once

#include <cassert>
#include <unordered_map>
#include <variant>
#include <vector>

#include <Eigen/Core>

#include "IndexTypes.hpp"
#include "model/domain/Cancellation.hpp"
#include "model/domain/ProgressReporter.hpp"
#include "model/domain/Segmentation.hpp"

namespace random_walker::algorithm {
    struct BoundaryConditions {
        std::vector<PixelIndex> pixels;
        std::unordered_map<PixelIndex, double, PixelIndexHash> value_by_pixel;

        [[nodiscard]] bool contains(PixelIndex pixel_index) const {
            return value_by_pixel.contains(pixel_index);
        }

        [[nodiscard]] double value_at(PixelIndex pixel_index) const {
            assert(contains(pixel_index));
            return value_by_pixel.at(pixel_index);
        }
    };

    using BoundaryConditionsOutcome =
        std::variant<BoundaryConditions, domain::Cancelled>;

    using BoundaryValuesOutcome =
        std::variant<Eigen::VectorXd, domain::Cancelled>;

    [[nodiscard]] BoundaryConditionsOutcome build_boundary_conditions(
        const domain::SegmentationInput& input
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    );

    [[nodiscard]] BoundaryValuesOutcome build_boundary_value_vector(
        const BoundaryConditions& boundary_conditions
        , const std::vector<PixelIndex>& boundary_pixels
        , const domain::CancellationToken& cancellation
        , const domain::ProgressReporter& progress
    );
}

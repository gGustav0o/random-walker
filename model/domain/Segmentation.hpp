#pragma once

#include <cstdint>
#include <span>
#include <variant>

#include <Eigen/Core>

#include "GrayImage.hpp"
#include "Seed.hpp"

namespace random_walker::domain
{
    using ProbabilityMap = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>;
    using BinaryMask = Eigen::Matrix<std::uint8_t, Eigen::Dynamic, Eigen::Dynamic>;

    struct SegmentationInput
    {
        const GrayImage& image;
        std::span<const Seed> seeds;
    };

    struct SegmentationResult
    {
        ProbabilityMap probabilities;
        BinaryMask mask;
    };

    enum class SegmentationError
    {
        EmptyImage,
        MissingBackgroundSeeds,
        MissingObjectSeeds,
        SeedOutOfBounds,
        LaplacianDecompositionFailed,
        LinearSystemSolveFailed,
        NonFiniteSolution
    };

    using SegmentationOutcome = std::variant<SegmentationResult, SegmentationError>;
}

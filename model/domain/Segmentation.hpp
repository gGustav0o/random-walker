#pragma once

#include <cstdint>
#include <span>
#include <utility>
#include <variant>
#include <vector>

#include <Eigen/Core>

#include "Cancellation.hpp"
#include "GrayImage.hpp"
#include "Seed.hpp"

namespace random_walker::domain
{
    using ProbabilityMap = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>;
    using BinaryMask = Eigen::Matrix<std::uint8_t, Eigen::Dynamic, Eigen::Dynamic>;
    using SegmentationRequestId = std::uint64_t;

    class SegmentationRequest final
    {
    public:
        SegmentationRequest(
            SegmentationRequestId request_id,
            GrayImage image,
            std::vector<SeedRegion> seed_regions)
            : request_id_(request_id)
            , image_(std::move(image))
            , seed_regions_(std::move(seed_regions))
        {
        }

        SegmentationRequest(const SegmentationRequest&) = default;
        SegmentationRequest(SegmentationRequest&&) noexcept = default;
        SegmentationRequest& operator=(const SegmentationRequest&) = delete;
        SegmentationRequest& operator=(SegmentationRequest&&) = delete;

        [[nodiscard]] SegmentationRequestId request_id() const noexcept
        {
            return request_id_;
        }

        [[nodiscard]] const GrayImage& image() const noexcept
        {
            return image_;
        }

        [[nodiscard]] std::span<const SeedRegion> seed_regions() const noexcept
        {
            return seed_regions_;
        }

    private:
        SegmentationRequestId request_id_;
        GrayImage image_;
        std::vector<SeedRegion> seed_regions_;
    };

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

    using SegmentationOutcome =
        std::variant<SegmentationResult, SegmentationError, Cancelled>;
}

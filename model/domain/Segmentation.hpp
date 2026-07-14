#pragma once

#include <span>
#include <utility>
#include <variant>
#include <vector>

#include <Eigen/Core>

#include "Cancellation.hpp"
#include "GrayImage.hpp"
#include "ProgressReporter.hpp"
#include "RandomWalkerParameters.hpp"
#include "Seed.hpp"
#include "SegmentationConstraints.hpp"

namespace random_walker::domain {

    using ProbabilityMap = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>;
    using BinaryMask = Eigen::Matrix<std::uint8_t, Eigen::Dynamic, Eigen::Dynamic>;

    class SegmentationRequest final {
    public:
        SegmentationRequest(
            SegmentationRequestId request_id
            , GrayImage image
            , SegmentationConstraints constraints
            , RandomWalkerParameters parameters
        )
            : request_id_(request_id)
            , image_(std::move(image))
            , constraints_(std::move(constraints))
            , parameters_(parameters) {}

        SegmentationRequest(const SegmentationRequest&) = default;
        SegmentationRequest(SegmentationRequest&&) noexcept = default;
        SegmentationRequest& operator=(const SegmentationRequest&) = delete;
        SegmentationRequest& operator=(SegmentationRequest&&) = delete;

        [[nodiscard]] SegmentationRequestId request_id() const noexcept {
            return request_id_;
        }

        [[nodiscard]] const GrayImage& image() const noexcept {
            return image_;
        }

        [[nodiscard]] const SegmentationConstraints& constraints()
            const noexcept {
            return constraints_;
        }

        [[nodiscard]] std::span<const SeedRegion> manual_seed_regions()
            const noexcept {
            return constraints_.manual_seed_regions;
        }

        [[nodiscard]] const MarkerLabelMask& automatic_markers()
            const noexcept {
            return constraints_.automatic_markers;
        }

        [[nodiscard]] const RandomWalkerParameters& parameters() const noexcept {
            return parameters_;
        }

    private:
        SegmentationRequestId request_id_;
        GrayImage image_;
        SegmentationConstraints constraints_;
        RandomWalkerParameters parameters_;
    };

    struct SegmentationInput {
        const GrayImage& image;
        std::span<const Seed> seeds;
    };

    struct SegmentationResult {
        ProbabilityMap probabilities;
        BinaryMask mask;
    };

    enum class SegmentationError {
        EmptyImage
        , ImageTooLarge
        , InvalidBeta
        , InvalidDistancePower
        , InvalidConnectivity
        , MissingBackgroundSeeds
        , MissingObjectSeeds
        , SeedOutOfBounds
        , ConflictingSeedLabels
        , UnanchoredUnknownRegion
        , LaplacianDecompositionFailed
        , LinearSystemSolveFailed
        , NonFiniteSolution
    };

    using SegmentationOutcome =
        std::variant<SegmentationResult, SegmentationError, Cancelled>;
}

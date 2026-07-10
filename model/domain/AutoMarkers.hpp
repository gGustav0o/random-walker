#pragma once

#include "Seed.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace random_walker::domain {

    inline constexpr int kGmmIntensityComponentCount = 2;
    inline constexpr int kMinimumGmmIterations = 1;
    inline constexpr int kMaximumGmmIterations = 500;
    inline constexpr int kDefaultGmmIterations = 100;
    inline constexpr double kMinimumGmmConvergenceTolerance = 1e-12;
    inline constexpr double kMaximumGmmConvergenceTolerance = 1e-2;
    inline constexpr double kDefaultGmmConvergenceTolerance = 1e-6;
    inline constexpr double kMinimumGmmVariance = 1e-6;
    inline constexpr double kMaximumGmmVariance = 255.0 * 255.0;
    inline constexpr double kDefaultGmmMinimumVariance = 1.0;
    inline constexpr double kMinimumAutoMarkerConfidenceThreshold = 0.5;
    inline constexpr double kMaximumAutoMarkerConfidenceThreshold = 1.0;
    inline constexpr double kDefaultAutoMarkerConfidenceThreshold = 0.98;
    inline constexpr int kMinimumAutoMarkerComponentArea = 1;
    inline constexpr int kMaximumAutoMarkerComponentArea = 1'000'000;
    inline constexpr int kDefaultAutoMarkerComponentArea = 16;
    inline constexpr int kMinimumAutoMarkerBoundaryDistance = 0;
    inline constexpr int kMaximumAutoMarkerBoundaryDistance = 64;
    inline constexpr int kDefaultAutoMarkerBoundaryDistance = 2;

    enum class ForegroundPolarity {
        DarkObject
        , BrightObject
    };

    [[nodiscard]] constexpr bool is_valid(
        ForegroundPolarity polarity
    ) noexcept {
        switch (polarity) {
        case ForegroundPolarity::DarkObject:
        case ForegroundPolarity::BrightObject:
            return true;
        }

        return false;
    }

    struct GmmParameters {
        int component_count = kGmmIntensityComponentCount;
        int max_iterations = kDefaultGmmIterations;
        double convergence_tolerance = kDefaultGmmConvergenceTolerance;
        double minimum_variance = kDefaultGmmMinimumVariance;
        bool operator==(const GmmParameters&) const = default;
    };

    [[nodiscard]] inline bool is_valid(
        const GmmParameters& parameters
    ) noexcept {
        return parameters.component_count == kGmmIntensityComponentCount
            && parameters.max_iterations >= kMinimumGmmIterations
            && parameters.max_iterations <= kMaximumGmmIterations
            && std::isfinite(parameters.convergence_tolerance)
            && parameters.convergence_tolerance >= kMinimumGmmConvergenceTolerance
            && parameters.convergence_tolerance <= kMaximumGmmConvergenceTolerance
            && std::isfinite(parameters.minimum_variance)
            && parameters.minimum_variance >= kMinimumGmmVariance
            && parameters.minimum_variance <= kMaximumGmmVariance;
    }

    struct AutoMarkerParameters {
        GmmParameters gmm;
        double confidence_threshold = kDefaultAutoMarkerConfidenceThreshold;
        int minimum_component_area = kDefaultAutoMarkerComponentArea;
        int minimum_boundary_distance = kDefaultAutoMarkerBoundaryDistance;
        ForegroundPolarity foreground_polarity = ForegroundPolarity::BrightObject;
        bool operator==(const AutoMarkerParameters&) const = default;
    };

    [[nodiscard]] inline bool is_valid(
        const AutoMarkerParameters& parameters
    ) noexcept {
        return is_valid(parameters.gmm)
            && std::isfinite(parameters.confidence_threshold)
            && parameters.confidence_threshold >= kMinimumAutoMarkerConfidenceThreshold
            && parameters.confidence_threshold <= kMaximumAutoMarkerConfidenceThreshold
            && parameters.minimum_component_area >= kMinimumAutoMarkerComponentArea
            && parameters.minimum_component_area <= kMaximumAutoMarkerComponentArea
            && parameters.minimum_boundary_distance >= kMinimumAutoMarkerBoundaryDistance
            && parameters.minimum_boundary_distance <= kMaximumAutoMarkerBoundaryDistance
            && is_valid(parameters.foreground_polarity);
    }

    enum class MarkerLabel : std::uint8_t {
        None = 0
        , Background = 1
        , Object = 2
    };

    [[nodiscard]] constexpr bool is_seed_marker_label(
        MarkerLabel label
    ) noexcept {
        return label == MarkerLabel::Background || label == MarkerLabel::Object;
    }

    [[nodiscard]] constexpr MarkerLabel marker_label_from_seed_label(
        SeedLabel label
    ) noexcept {
        switch (label) {
        case SeedLabel::Background:
            return MarkerLabel::Background;
        case SeedLabel::Object:
            return MarkerLabel::Object;
        }

        return MarkerLabel::None;
    }

    [[nodiscard]] inline SeedLabel seed_label_from_marker_label(
        MarkerLabel label
    ) noexcept {
        assert(is_seed_marker_label(label));
        return label == MarkerLabel::Object
            ? SeedLabel::Object
            : SeedLabel::Background;
    }

    class MarkerLabelMask final {
    public:
        MarkerLabelMask() = default;
        MarkerLabelMask(int width, int height)
            : width_(width)
            , height_(height) {
            assert(width >= 0);
            assert(height >= 0);
            labels_.resize(
                static_cast<std::size_t>(width)
                * static_cast<std::size_t>(height)
            );
        }

        [[nodiscard]] bool empty() const noexcept {
            return labels_.empty();
        }

        [[nodiscard]] int width() const noexcept {
            return width_;
        }

        [[nodiscard]] int height() const noexcept {
            return height_;
        }

        [[nodiscard]] std::size_t size() const noexcept {
            return labels_.size();
        }

        [[nodiscard]] MarkerLabel at(int row, int column) const noexcept {
            assert(row >= 0);
            assert(column >= 0);
            assert(row < height_);
            assert(column < width_);
            return labels_[flatten(row, column)];
        }

        void set(int row, int column, MarkerLabel label) noexcept {
            assert(row >= 0);
            assert(column >= 0);
            assert(row < height_);
            assert(column < width_);
            labels_[flatten(row, column)] = label;
        }

        [[nodiscard]] std::size_t count(MarkerLabel label) const noexcept {
            return static_cast<std::size_t>(std::count(
                labels_.begin()
                , labels_.end()
                , label
            ));
        }

        [[nodiscard]] std::size_t seed_count() const noexcept {
            return labels_.size() - count(MarkerLabel::None);
        }

        [[nodiscard]] const std::vector<MarkerLabel>& labels() const noexcept {
            return labels_;
        }

        bool operator==(const MarkerLabelMask&) const = default;

    private:
        [[nodiscard]] std::size_t flatten(int row, int column) const noexcept {
            return static_cast<std::size_t>(row) * static_cast<std::size_t>(width_)
                + static_cast<std::size_t>(column);
        }

        int width_ = 0;
        int height_ = 0;
        std::vector<MarkerLabel> labels_;
    };


    [[nodiscard]] inline int marker_pixel_count(
        const MarkerLabelMask& mask
        , MarkerLabel label
    ) noexcept {
        assert(is_seed_marker_label(label));
        return static_cast<int>(mask.count(label));
    }

    [[nodiscard]] inline int marker_pixel_count(
        const MarkerLabelMask& mask
        , SeedLabel label
    ) noexcept {
        return marker_pixel_count(mask, marker_label_from_seed_label(label));
    }

    [[nodiscard]] inline std::vector<SeedRegion> seed_regions_from_marker_mask(
        const MarkerLabelMask& mask
    ) {
        std::vector<SeedRegion> regions;
        regions.reserve(mask.seed_count());

        for (int row = 0; row < mask.height(); ++row) {
            for (int column = 0; column < mask.width(); ++column) {
                const MarkerLabel marker = mask.at(row, column);
                if (!is_seed_marker_label(marker)) {
                    continue;
                }

                regions.push_back({
                    .area = {
                        .x = column
                        , .y = row
                        , .width = 1
                        , .height = 1
                    }
                    , .label = seed_label_from_marker_label(marker)
                    , .source = SeedSource::Automatic
                });
            }
        }

        return regions;
    }

    struct MarkerProposalDiagnostics {
        std::size_t proposed_seed_count = 0;
        std::size_t rejected_low_confidence_count = 0;
        std::size_t rejected_small_component_count = 0;
        std::size_t rejected_boundary_distance_count = 0;
        std::size_t rejected_manual_conflict_count = 0;
        std::size_t gmm_iterations = 0;
        bool gmm_converged = false;
        bool operator==(const MarkerProposalDiagnostics&) const = default;
    };

    struct MarkerProposal {
        MarkerLabelMask mask;
        MarkerProposalDiagnostics diagnostics;
        bool operator==(const MarkerProposal&) const = default;
    };
}

#pragma once

#include <cmath>

namespace random_walker::domain {

    inline constexpr double kMinimumRandomWalkerBeta = 1e-6;
    inline constexpr double kMaximumRandomWalkerBeta = 1e-1;
    inline constexpr double kDefaultRandomWalkerBeta = 0.001;
    inline constexpr double kMinimumRandomWalkerDistancePower = 0.0;
    inline constexpr double kMaximumRandomWalkerDistancePower = 2.0;
    inline constexpr double kDefaultRandomWalkerDistancePower = 0.0;
    inline constexpr int kMinimumLocalContrastRadius = 1;
    inline constexpr int kMaximumLocalContrastRadius = 16;
    inline constexpr int kDefaultLocalContrastRadius = 1;
    inline constexpr double kMinimumLocalContrastVariance = 1e-6;
    inline constexpr double kMaximumLocalContrastVariance = 255.0 * 255.0;
    inline constexpr double kDefaultLocalContrastMinimumVariance = 1.0;

    enum class PixelConnectivity {
        Four
        , Eight
    };

    enum class EdgeWeightModel {
        GlobalBeta
        , LocalVarianceNormalized
    };

    inline constexpr PixelConnectivity kDefaultPixelConnectivity =
        PixelConnectivity::Four;
    inline constexpr EdgeWeightModel kDefaultEdgeWeightModel =
        EdgeWeightModel::GlobalBeta;

    [[nodiscard]] constexpr bool is_valid(
        PixelConnectivity connectivity
    ) noexcept {
        switch (connectivity) {
        case PixelConnectivity::Four:
        case PixelConnectivity::Eight:
            return true;
        }

        return false;
    }

    [[nodiscard]] constexpr bool is_valid(
        EdgeWeightModel edge_weight_model
    ) noexcept {
        switch (edge_weight_model) {
        case EdgeWeightModel::GlobalBeta:
        case EdgeWeightModel::LocalVarianceNormalized:
            return true;
        }

        return false;
    }

    struct LocalContrastScaleParameters {
        int radius = kDefaultLocalContrastRadius;
        double minimum_variance = kDefaultLocalContrastMinimumVariance;
        bool operator==(const LocalContrastScaleParameters&) const = default;
    };

    [[nodiscard]] inline bool is_valid(
        const LocalContrastScaleParameters& parameters
    ) noexcept {
        return parameters.radius >= kMinimumLocalContrastRadius
            && parameters.radius <= kMaximumLocalContrastRadius
            && std::isfinite(parameters.minimum_variance)
            && parameters.minimum_variance >= kMinimumLocalContrastVariance
            && parameters.minimum_variance <= kMaximumLocalContrastVariance;
    }

    struct RandomWalkerParameters {
        double beta = kDefaultRandomWalkerBeta;
        double distance_power = kDefaultRandomWalkerDistancePower;
        PixelConnectivity connectivity = kDefaultPixelConnectivity;
        EdgeWeightModel edge_weight_model = kDefaultEdgeWeightModel;
        LocalContrastScaleParameters local_contrast_scale;
        bool operator==(const RandomWalkerParameters&) const = default;
    };

    [[nodiscard]] inline bool is_valid(
        const RandomWalkerParameters& parameters
    ) noexcept {
        return std::isfinite(parameters.beta)
            && parameters.beta >= kMinimumRandomWalkerBeta
            && parameters.beta <= kMaximumRandomWalkerBeta
            && std::isfinite(parameters.distance_power)
            && parameters.distance_power >= kMinimumRandomWalkerDistancePower
            && parameters.distance_power <= kMaximumRandomWalkerDistancePower
            && is_valid(parameters.connectivity)
            && is_valid(parameters.edge_weight_model)
            && is_valid(parameters.local_contrast_scale);
    }
}

#pragma once

#include <cassert>
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
    inline constexpr double kMinimumLocalContrastAutoQuantile = 0.01;
    inline constexpr double kMaximumLocalContrastAutoQuantile = 0.50;
    inline constexpr double kDefaultLocalContrastAutoQuantile = 0.10;
    inline constexpr int kMinimumEdgeLocalContrastRadius = 1;
    inline constexpr int kMaximumEdgeLocalContrastRadius = 16;
    inline constexpr int kDefaultEdgeLocalContrastRadius = 1;
    inline constexpr double kMinimumEdgeLocalContrastScale = 1e-6;
    inline constexpr double kMaximumEdgeLocalContrastScale = 255.0;
    inline constexpr double kDefaultEdgeLocalContrastMinimumScale = 1.0;
    inline constexpr double kMinimumEdgeLocalContrastQuantile = 0.25;
    inline constexpr double kMaximumEdgeLocalContrastQuantile = 0.50;
    inline constexpr double kDefaultEdgeLocalContrastQuantile = 0.35;

    enum class PixelConnectivity {
        Four
        , Eight
    };

    enum class EdgeWeightModel {
        GlobalBeta
        , LocalVarianceNormalized
        , EdgeLocalContrastNormalized
    };

    enum class MinimumVarianceMode {
        Manual
        , Auto
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
        case EdgeWeightModel::EdgeLocalContrastNormalized:
            return true;
        }

        return false;
    }

    [[nodiscard]] constexpr bool is_valid(
        MinimumVarianceMode mode
    ) noexcept {
        switch (mode) {
        case MinimumVarianceMode::Manual:
        case MinimumVarianceMode::Auto:
            return true;
        }

        return false;
    }

    inline constexpr MinimumVarianceMode kDefaultMinimumVarianceMode =
        MinimumVarianceMode::Manual;


    struct LocalContrastScaleParameters {
        int radius = kDefaultLocalContrastRadius;
        double minimum_variance = kDefaultLocalContrastMinimumVariance;
        MinimumVarianceMode minimum_variance_mode = kDefaultMinimumVarianceMode;
        double auto_minimum_variance_quantile =
            kDefaultLocalContrastAutoQuantile;
        bool operator==(const LocalContrastScaleParameters&) const = default;
    };

    struct EffectiveLocalContrastScale {
        int radius = kDefaultLocalContrastRadius;
        double minimum_variance = kDefaultLocalContrastMinimumVariance;
        bool operator==(const EffectiveLocalContrastScale&) const = default;
    };

    struct EdgeLocalContrastScaleParameters {
        int radius = kDefaultEdgeLocalContrastRadius;
        double quantile = kDefaultEdgeLocalContrastQuantile;
        double minimum_scale = kDefaultEdgeLocalContrastMinimumScale;
        bool operator==(const EdgeLocalContrastScaleParameters&) const = default;
    };

    [[nodiscard]] inline bool is_valid(
        const LocalContrastScaleParameters& parameters
    ) noexcept {
        return parameters.radius >= kMinimumLocalContrastRadius
            && parameters.radius <= kMaximumLocalContrastRadius
            && std::isfinite(parameters.minimum_variance)
            && parameters.minimum_variance >= kMinimumLocalContrastVariance
            && parameters.minimum_variance <= kMaximumLocalContrastVariance
            && is_valid(parameters.minimum_variance_mode)
            && std::isfinite(parameters.auto_minimum_variance_quantile)
            && parameters.auto_minimum_variance_quantile
                >= kMinimumLocalContrastAutoQuantile
            && parameters.auto_minimum_variance_quantile
                <= kMaximumLocalContrastAutoQuantile;
    }

    [[nodiscard]] inline bool is_valid(
        const EffectiveLocalContrastScale& scale
    ) noexcept {
        return scale.radius >= kMinimumLocalContrastRadius
            && scale.radius <= kMaximumLocalContrastRadius
            && std::isfinite(scale.minimum_variance)
            && scale.minimum_variance >= kMinimumLocalContrastVariance
            && scale.minimum_variance <= kMaximumLocalContrastVariance;
    }

    [[nodiscard]] inline bool is_valid(
        const EdgeLocalContrastScaleParameters& parameters
    ) noexcept {
        return parameters.radius >= kMinimumEdgeLocalContrastRadius
            && parameters.radius <= kMaximumEdgeLocalContrastRadius
            && std::isfinite(parameters.quantile)
            && parameters.quantile >= kMinimumEdgeLocalContrastQuantile
            && parameters.quantile <= kMaximumEdgeLocalContrastQuantile
            && std::isfinite(parameters.minimum_scale)
            && parameters.minimum_scale >= kMinimumEdgeLocalContrastScale
            && parameters.minimum_scale <= kMaximumEdgeLocalContrastScale;
    }

    [[nodiscard]] inline EffectiveLocalContrastScale manual_effective_scale(
        const LocalContrastScaleParameters& parameters
    ) noexcept {
        assert(is_valid(parameters));
        return {
            .radius = parameters.radius
            , .minimum_variance = parameters.minimum_variance
        };
    }

    struct RandomWalkerParameters {
        double beta = kDefaultRandomWalkerBeta;
        double distance_power = kDefaultRandomWalkerDistancePower;
        PixelConnectivity connectivity = kDefaultPixelConnectivity;
        EdgeWeightModel edge_weight_model = kDefaultEdgeWeightModel;
        LocalContrastScaleParameters local_contrast_scale;
        EdgeLocalContrastScaleParameters edge_local_contrast_scale;
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
            && is_valid(parameters.local_contrast_scale)
            && is_valid(parameters.edge_local_contrast_scale);
    }
}

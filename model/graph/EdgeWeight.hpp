#pragma once

#include "model/domain/RandomWalkerParameters.hpp"
#include "model/graph/LocalContrastScale.hpp"

#include <cassert>
#include <cmath>
#include <concepts>
#include <cstdint>

namespace random_walker::graph {

    struct EdgeWeightInput {
        std::uint8_t first_intensity = 0;
        std::uint8_t second_intensity = 0;
        GridPoint first_position;
        GridPoint second_position;
        double distance = 1.0;
        bool operator==(const EdgeWeightInput&) const = default;
    };

    struct GlobalBetaEdgeWeightParameters {
        double beta = domain::kDefaultRandomWalkerBeta;
        double distance_power = domain::kDefaultRandomWalkerDistancePower;
        bool operator==(const GlobalBetaEdgeWeightParameters&) const = default;
    };

    struct LocalVarianceNormalizedEdgeWeightParameters {
        double distance_power = domain::kDefaultRandomWalkerDistancePower;
        bool operator==(const LocalVarianceNormalizedEdgeWeightParameters&) const = default;
    };

    [[nodiscard]] inline bool is_valid(
        const EdgeWeightInput& input
    ) noexcept {
        return input.first_position.row >= 0
            && input.first_position.column >= 0
            && input.second_position.row >= 0
            && input.second_position.column >= 0
            && std::isfinite(input.distance)
            && input.distance >= 1.0;
    }

    [[nodiscard]] inline bool is_valid(
        const GlobalBetaEdgeWeightParameters& parameters
    ) noexcept {
        return std::isfinite(parameters.beta)
            && parameters.beta >= domain::kMinimumRandomWalkerBeta
            && parameters.beta <= domain::kMaximumRandomWalkerBeta
            && std::isfinite(parameters.distance_power)
            && parameters.distance_power >= domain::kMinimumRandomWalkerDistancePower
            && parameters.distance_power <= domain::kMaximumRandomWalkerDistancePower;
    }

    [[nodiscard]] inline bool is_valid(
        const LocalVarianceNormalizedEdgeWeightParameters& parameters
    ) noexcept {
        return std::isfinite(parameters.distance_power)
            && parameters.distance_power >= domain::kMinimumRandomWalkerDistancePower
            && parameters.distance_power <= domain::kMaximumRandomWalkerDistancePower;
    }

    [[nodiscard]] inline GlobalBetaEdgeWeightParameters
    global_beta_edge_weight_parameters_from(
        const domain::RandomWalkerParameters& parameters
    ) noexcept {
        assert(domain::is_valid(parameters));
        assert(parameters.edge_weight_model == domain::EdgeWeightModel::GlobalBeta);
        return {
            .beta = parameters.beta
            , .distance_power = parameters.distance_power
        };
    }

    [[nodiscard]] inline LocalVarianceNormalizedEdgeWeightParameters
    local_variance_normalized_edge_weight_parameters_from(
        const domain::RandomWalkerParameters& parameters
    ) noexcept {
        assert(domain::is_valid(parameters));
        assert(parameters.edge_weight_model
            == domain::EdgeWeightModel::LocalVarianceNormalized);
        return {
            .distance_power = parameters.distance_power
        };
    }

    template<typename Function>
    concept EdgeWeightFunction = requires(
        const Function& function
        , const EdgeWeightInput& input
    ) {
        { function(input) } -> std::convertible_to<double>;
    };

    [[nodiscard]] inline double distance_weight(
        double distance
        , double distance_power
    ) noexcept {
        assert(std::isfinite(distance));
        assert(distance >= 1.0);
        assert(std::isfinite(distance_power));
        assert(distance_power >= domain::kMinimumRandomWalkerDistancePower);
        assert(distance_power <= domain::kMaximumRandomWalkerDistancePower);

        const double result = std::pow(distance, distance_power);
        assert(std::isfinite(result));
        assert(result >= 1.0);
        return result;
    }

    [[nodiscard]] inline double squared_intensity_difference(
        std::uint8_t first_intensity
        , std::uint8_t second_intensity
    ) noexcept {
        const double difference =
            static_cast<double>(first_intensity)
            - static_cast<double>(second_intensity);
        return difference * difference;
    }

    [[nodiscard]] inline double compute_global_beta_edge_weight(
        const EdgeWeightInput& input
        , const GlobalBetaEdgeWeightParameters& parameters
    ) noexcept {
        assert(is_valid(input));
        assert(is_valid(parameters));

        const double contrast_weight = std::exp(
            -parameters.beta
            * squared_intensity_difference(
                input.first_intensity
                , input.second_intensity
            )
        );
        assert(std::isfinite(contrast_weight));

        const double weight =
            contrast_weight / distance_weight(input.distance, parameters.distance_power);
        assert(std::isfinite(weight));
        assert(weight >= 0.0);
        assert(weight <= 1.0);
        return weight;
    }

    [[nodiscard]] inline double edge_local_variance(
        const LocalContrastScaleMap& contrast_scale
        , const EdgeWeightInput& input
    ) noexcept {
        assert(is_valid(input));
        assert(!contrast_scale.empty());
        assert(input.first_position.row < contrast_scale.height());
        assert(input.first_position.column < contrast_scale.width());
        assert(input.second_position.row < contrast_scale.height());
        assert(input.second_position.column < contrast_scale.width());

        const double first_variance = contrast_scale.variance_at(input.first_position);
        const double second_variance = contrast_scale.variance_at(input.second_position);
        const double variance = (first_variance + second_variance) / 2.0;
        assert(std::isfinite(variance));
        assert(variance >= domain::kMinimumLocalContrastVariance);
        return variance;
    }

    [[nodiscard]] inline double compute_local_variance_normalized_edge_weight(
        const EdgeWeightInput& input
        , const LocalContrastScaleMap& contrast_scale
        , const LocalVarianceNormalizedEdgeWeightParameters& parameters
    ) noexcept {
        assert(is_valid(input));
        assert(is_valid(parameters));

        const double variance = edge_local_variance(contrast_scale, input);
        const double contrast_weight = std::exp(
            -squared_intensity_difference(
                input.first_intensity
                , input.second_intensity
            ) / (2.0 * variance)
        );
        assert(std::isfinite(contrast_weight));

        const double weight =
            contrast_weight / distance_weight(input.distance, parameters.distance_power);
        assert(std::isfinite(weight));
        assert(weight >= 0.0);
        assert(weight <= 1.0);
        return weight;
    }

    struct GlobalBetaEdgeWeight {
        GlobalBetaEdgeWeightParameters parameters;

        [[nodiscard]] double operator()(
            const EdgeWeightInput& input
        ) const noexcept {
            return compute_global_beta_edge_weight(input, parameters);
        }
    };

    struct LocalVarianceNormalizedEdgeWeight {
        const LocalContrastScaleMap& contrast_scale;
        LocalVarianceNormalizedEdgeWeightParameters parameters;

        [[nodiscard]] double operator()(
            const EdgeWeightInput& input
        ) const noexcept {
            return compute_local_variance_normalized_edge_weight(
                input
                , contrast_scale
                , parameters
            );
        }
    };

    static_assert(EdgeWeightFunction<GlobalBetaEdgeWeight>);
    static_assert(EdgeWeightFunction<LocalVarianceNormalizedEdgeWeight>);
}

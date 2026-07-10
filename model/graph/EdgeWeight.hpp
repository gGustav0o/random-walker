#pragma once

#include "model/domain/RandomWalkerParameters.hpp"
#include "model/graph/GridGeometry.hpp"

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

    [[nodiscard]] inline bool is_valid_distance_power(
        double distance_power
    ) noexcept {
        return std::isfinite(distance_power)
            && distance_power >= domain::kMinimumRandomWalkerDistancePower
            && distance_power <= domain::kMaximumRandomWalkerDistancePower;
    }

    [[nodiscard]] inline bool is_valid(
        const GlobalBetaEdgeWeightParameters& parameters
    ) noexcept {
        return std::isfinite(parameters.beta)
            && parameters.beta >= domain::kMinimumRandomWalkerBeta
            && parameters.beta <= domain::kMaximumRandomWalkerBeta
            && is_valid_distance_power(parameters.distance_power);
    }

    [[nodiscard]] inline GlobalBetaEdgeWeightParameters
    global_beta_edge_weight_parameters_from(
        const domain::RandomWalkerParameters& parameters
    ) noexcept {
        assert(domain::is_valid(parameters));
        return {
            .beta = parameters.beta
            , .distance_power = parameters.distance_power
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
        assert(is_valid_distance_power(distance_power));

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

    [[nodiscard]] inline double combine_contrast_and_geometry(
        double contrast_weight
        , double distance
        , double distance_power
    ) noexcept {
        assert(std::isfinite(contrast_weight));
        assert(contrast_weight >= 0.0);
        assert(contrast_weight <= 1.0);

        const double weight =
            contrast_weight / distance_weight(distance, distance_power);
        assert(std::isfinite(weight));
        assert(weight >= 0.0);
        assert(weight <= 1.0);
        return weight;
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

        return combine_contrast_and_geometry(
            contrast_weight
            , input.distance
            , parameters.distance_power
        );
    }

    struct GlobalBetaEdgeWeight {
        GlobalBetaEdgeWeightParameters parameters;

        [[nodiscard]] double operator()(
            const EdgeWeightInput& input
        ) const noexcept {
            return compute_global_beta_edge_weight(input, parameters);
        }
    };

    static_assert(EdgeWeightFunction<GlobalBetaEdgeWeight>);
}
#pragma once

#include "model/domain/RandomWalkerParameters.hpp"

#include <cassert>
#include <cmath>
#include <concepts>
#include <cstdint>

namespace random_walker::graph {

    struct EdgeWeightParameters {
        double beta = domain::kDefaultRandomWalkerBeta;
        double distance_power = domain::kDefaultRandomWalkerDistancePower;
        bool operator==(const EdgeWeightParameters&) const = default;
    };

    [[nodiscard]] inline bool is_valid(
        const EdgeWeightParameters& parameters
    ) noexcept {
        return std::isfinite(parameters.beta)
            && parameters.beta >= domain::kMinimumRandomWalkerBeta
            && parameters.beta <= domain::kMaximumRandomWalkerBeta
            && std::isfinite(parameters.distance_power)
            && parameters.distance_power >= domain::kMinimumRandomWalkerDistancePower
            && parameters.distance_power <= domain::kMaximumRandomWalkerDistancePower;
    }

    [[nodiscard]] inline EdgeWeightParameters edge_weight_parameters_from(
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
        , std::uint8_t first_intensity
        , std::uint8_t second_intensity
        , double distance
    ) {
        { function(first_intensity, second_intensity, distance) }
            -> std::convertible_to<double>;
    };

    [[nodiscard]] inline double compute_edge_weight(
        std::uint8_t first_intensity
        , std::uint8_t second_intensity
        , double distance
        , const EdgeWeightParameters& parameters
    ) noexcept {
        assert(is_valid(parameters));
        assert(std::isfinite(distance));
        assert(distance >= 1.0);

        const double difference =
            static_cast<double>(first_intensity)
            - static_cast<double>(second_intensity);
        const double contrast_weight =
            std::exp(-parameters.beta * difference * difference);
        const double distance_weight =
            std::pow(distance, parameters.distance_power);
        assert(std::isfinite(contrast_weight));
        assert(std::isfinite(distance_weight));
        assert(distance_weight >= 1.0);

        const double weight = contrast_weight / distance_weight;
        assert(std::isfinite(weight));
        assert(weight >= 0.0);
        assert(weight <= 1.0);
        return weight;
    }

    struct ExponentialDistanceEdgeWeight {
        EdgeWeightParameters parameters;

        [[nodiscard]] double operator()(
            std::uint8_t first_intensity
            , std::uint8_t second_intensity
            , double distance
        ) const noexcept {
            return compute_edge_weight(
                first_intensity
                , second_intensity
                , distance
                , parameters
            );
        }
    };

    static_assert(EdgeWeightFunction<ExponentialDistanceEdgeWeight>);
}

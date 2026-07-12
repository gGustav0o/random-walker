#pragma once

#include <cmath>
#include <optional>

namespace random_walker::domain {

    inline constexpr double kMinimumRandomWalkerBeta = 1e-6;
    inline constexpr double kMaximumRandomWalkerBeta = 1e-1;
    inline constexpr double kDefaultRandomWalkerBeta = 0.001;
    inline constexpr double kMinimumRandomWalkerDistancePower = 0.0;
    inline constexpr double kMaximumRandomWalkerDistancePower = 2.0;
    inline constexpr double kDefaultRandomWalkerDistancePower = 0.0;

    enum class PixelConnectivity {
        Four
        , Eight
    };

    inline constexpr PixelConnectivity kDefaultPixelConnectivity =
        PixelConnectivity::Four;

    enum class RandomWalkerParameterError {
        InvalidBeta
        , InvalidDistancePower
        , InvalidConnectivity
    };

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

    struct RandomWalkerParameters {
        double beta = kDefaultRandomWalkerBeta;
        double distance_power = kDefaultRandomWalkerDistancePower;
        PixelConnectivity connectivity = kDefaultPixelConnectivity;
        bool operator==(const RandomWalkerParameters&) const = default;
    };

    [[nodiscard]] inline std::optional<RandomWalkerParameterError> validate(
        const RandomWalkerParameters& parameters
    ) noexcept {
        if (!std::isfinite(parameters.beta)
            || parameters.beta < kMinimumRandomWalkerBeta
            || parameters.beta > kMaximumRandomWalkerBeta
        ) {
            return RandomWalkerParameterError::InvalidBeta;
        }

        if (!std::isfinite(parameters.distance_power)
            || parameters.distance_power < kMinimumRandomWalkerDistancePower
            || parameters.distance_power > kMaximumRandomWalkerDistancePower
        ) {
            return RandomWalkerParameterError::InvalidDistancePower;
        }

        if (!is_valid(parameters.connectivity)) {
            return RandomWalkerParameterError::InvalidConnectivity;
        }

        return std::nullopt;
    }

    [[nodiscard]] inline bool is_valid(
        const RandomWalkerParameters& parameters
    ) noexcept {
        return !validate(parameters).has_value();
    }
}

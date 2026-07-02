#pragma once

#include <cmath>

namespace random_walker::domain {

    inline constexpr double kMinimumRandomWalkerBeta = 1e-6;
    inline constexpr double kMaximumRandomWalkerBeta = 1e-1;
    inline constexpr double kDefaultRandomWalkerBeta = 0.001;

    enum class PixelConnectivity {
        Four
        , Eight
    };

    inline constexpr PixelConnectivity kDefaultPixelConnectivity =
        PixelConnectivity::Four;

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
        PixelConnectivity connectivity = kDefaultPixelConnectivity;
        bool operator==(const RandomWalkerParameters&) const = default;
    };

    [[nodiscard]] inline bool is_valid(
        const RandomWalkerParameters& parameters
    ) noexcept {
        return std::isfinite(parameters.beta)
            && parameters.beta >= kMinimumRandomWalkerBeta
            && parameters.beta <= kMaximumRandomWalkerBeta
            && is_valid(parameters.connectivity);
    }
}
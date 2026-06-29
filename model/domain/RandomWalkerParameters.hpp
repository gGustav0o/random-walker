#pragma once

#include <cmath>

namespace random_walker::domain {

    inline constexpr double kMinimumRandomWalkerBeta = 1e-6;
    inline constexpr double kMaximumRandomWalkerBeta = 1e-1;
    inline constexpr double kDefaultRandomWalkerBeta = 0.001;

    struct RandomWalkerParameters {
        double beta = kDefaultRandomWalkerBeta;
        bool operator==(const RandomWalkerParameters&) const = default;
    };

    [[nodiscard]] inline bool is_valid(
        const RandomWalkerParameters& parameters
    ) noexcept {
        return std::isfinite(parameters.beta)
            && parameters.beta >= kMinimumRandomWalkerBeta
            && parameters.beta <= kMaximumRandomWalkerBeta;
    }
}

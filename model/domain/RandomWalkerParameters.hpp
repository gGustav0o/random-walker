#pragma once

namespace random_walker::domain
{
    inline constexpr double kDefaultRandomWalkerBeta = 0.001;

    struct RandomWalkerParameters
    {
        double beta = kDefaultRandomWalkerBeta;

        bool operator==(const RandomWalkerParameters&) const = default;
    };
}

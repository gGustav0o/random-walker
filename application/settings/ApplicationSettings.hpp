#pragma once

#include "model/domain/RandomWalkerParameters.hpp"

namespace random_walker::application {
    struct ApplicationSettings {
        domain::RandomWalkerParameters random_walker;

        bool operator==(const ApplicationSettings&) const = default;
    };
}

#pragma once

#include "model/domain/AutoMarkers.hpp"
#include "model/domain/RandomWalkerParameters.hpp"

namespace random_walker::application {

    struct ApplicationSettings {
        domain::RandomWalkerParameters random_walker;
        domain::AutoMarkerParameters auto_markers;
        bool operator==(const ApplicationSettings&) const = default;
    };
}

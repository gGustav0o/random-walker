#pragma once

#include <optional>

#include "application/error/UserError.hpp"
#include "model/domain/AutoMarkers.hpp"
#include "model/domain/RandomWalkerParameters.hpp"
#include "model/executor/SegmentationExecutor.hpp"

namespace random_walker::viewmodel {

    [[nodiscard]] int view_connectivity(
        domain::PixelConnectivity connectivity
    ) noexcept;

    [[nodiscard]] std::optional<domain::PixelConnectivity>
    domain_connectivity(int connectivity) noexcept;

    [[nodiscard]] int view_foreground_polarity(
        domain::ForegroundPolarity foreground_polarity
    ) noexcept;

    [[nodiscard]] std::optional<domain::ForegroundPolarity>
    domain_foreground_polarity(int foreground_polarity) noexcept;

    [[nodiscard]] application::ApplicationError application_error(
        executor::ExecutionError error
    ) noexcept;
}

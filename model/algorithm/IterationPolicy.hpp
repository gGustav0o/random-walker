#pragma once

#include <cstddef>

namespace random_walker::algorithm {
    inline constexpr std::size_t kCancellationPollingInterval = 4096;
    inline constexpr std::size_t kProgressReportInterval = 4096;

    [[nodiscard]] constexpr bool should_poll_cancellation(
        std::size_t iteration
    ) noexcept {
        return iteration % kCancellationPollingInterval == 0;
    }

    [[nodiscard]] constexpr bool should_report_progress(
        std::size_t iteration
    ) noexcept {
        return iteration % kProgressReportInterval == 0;
    }
}

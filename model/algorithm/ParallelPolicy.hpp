#pragma once

#include <cstddef>

#ifndef RANDOM_WALKER_ENABLE_OPENMP
#define RANDOM_WALKER_ENABLE_OPENMP 0
#endif

namespace random_walker::algorithm {
    inline constexpr std::size_t kParallelWorkItemThreshold = 65'536;

    inline constexpr bool kOpenMpEnabled =
        RANDOM_WALKER_ENABLE_OPENMP != 0;

    [[nodiscard]] constexpr bool should_run_parallel(
        std::size_t work_item_count
    ) noexcept {
        return kOpenMpEnabled
            && work_item_count >= kParallelWorkItemThreshold;
    }
}
#pragma once

#include <stop_token>

namespace random_walker::domain {
    struct Cancelled {
        bool operator==(const Cancelled&) const = default;
    };

    class CancellationToken final {
    public:
        CancellationToken() = default;

        explicit CancellationToken(std::stop_token token) noexcept
            : token_(token) {
        }

        [[nodiscard]] bool stop_requested() const noexcept {
            return token_.stop_requested();
        }

        [[nodiscard]] bool stop_possible() const noexcept {
            return token_.stop_possible();
        }

    private:
        std::stop_token token_;
    };
}

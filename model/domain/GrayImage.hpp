#pragma once

#include <cstdint>
#include <utility>

#include <Eigen/Core>

namespace random_walker::domain {
    using GrayImageMatrix = Eigen::Matrix<std::uint8_t, Eigen::Dynamic, Eigen::Dynamic>;

    class GrayImage final {
    public:
        GrayImage() = default;
        explicit GrayImage(GrayImageMatrix pixels)
            : pixels_(std::move(pixels)) {
        }

        [[nodiscard]] bool empty() const noexcept {
            return pixels_.size() == 0;
        }

        [[nodiscard]] int width() const noexcept {
            return static_cast<int>(pixels_.cols());
        }

        [[nodiscard]] int height() const noexcept {
            return static_cast<int>(pixels_.rows());
        }

        [[nodiscard]] std::uint8_t at(int row, int column) const noexcept {
            return pixels_(row, column);
        }

        [[nodiscard]] const GrayImageMatrix& pixels() const noexcept {
            return pixels_;
        }

    private:
        GrayImageMatrix pixels_;
    };
}

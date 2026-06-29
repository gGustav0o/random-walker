#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <utility>

#include <Eigen/Core>

namespace random_walker::domain {
    using GrayImageMatrix = Eigen::Matrix<std::uint8_t, Eigen::Dynamic, Eigen::Dynamic>;

    class GrayImage final {
    public:
        GrayImage() = default;
        explicit GrayImage(GrayImageMatrix pixels)
            : pixels_(std::move(pixels)) {
            assert(pixels_.rows() <= std::numeric_limits<int>::max());
            assert(pixels_.cols() <= std::numeric_limits<int>::max());
        }

        [[nodiscard]] bool empty() const noexcept {
            return pixels_.size() == 0;
        }

        [[nodiscard]] int width() const noexcept {
            assert(pixels_.cols() <= std::numeric_limits<int>::max());
            return static_cast<int>(pixels_.cols());
        }

        [[nodiscard]] int height() const noexcept {
            assert(pixels_.rows() <= std::numeric_limits<int>::max());
            return static_cast<int>(pixels_.rows());
        }

        [[nodiscard]] std::uint8_t at(int row, int column) const noexcept {
            assert(row >= 0);
            assert(column >= 0);
            assert(row < height());
            assert(column < width());
            return pixels_(row, column);
        }

        [[nodiscard]] const GrayImageMatrix& pixels() const noexcept {
            return pixels_;
        }

    private:
        GrayImageMatrix pixels_;
    };
}

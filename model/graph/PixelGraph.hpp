#pragma once

#include <Eigen/Core>
#include <Eigen/Sparse>

#include <vector>
#include <utility>

#include "../../MetaAnnotations.hpp"

namespace graph
{

    class PixelGraph
    {
    public:
        explicit PixelGraph(const Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic>& image);

        [[nodiscard]] Eigen::SparseMatrix<double> laplacian() const;
        [[nodiscard]] int index_at(int row, int col) const noexcept;
        [[nodiscard]] std::pair<int, int> coords_from_index(int index) const noexcept;

        [[nodiscard]] int width() const noexcept { return width_; }
        [[nodiscard]] int height() const noexcept { return height_; }
        [[nodiscard]] int pixel_count() const noexcept { return width_ * height_; }

    private:
        Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic> image_;
        int width_ = 0;
        int height_ = 0;
    };

} // namespace graph

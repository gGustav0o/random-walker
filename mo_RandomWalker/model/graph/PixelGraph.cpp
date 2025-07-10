#include "PixelGraph.hpp"

#include "../../global.hpp"

#include <Eigen/Sparse>
#include <cmath>
#include <optional>

namespace graph
{
    static constexpr double kBeta = 0.001;
    //static constexpr double kBeta = 0.05;
    //static constexpr double kBeta = 0.085;
    namespace weights
    {
        namespace
        {
            inline _impure_(double compute_weight(uint8_t g1, uint8_t g2, std::optional<double> intensity_variance = std::nullopt))
            {
                const double diff = static_cast<double>(g1) - static_cast<double>(g2);
                return std::exp(-kBeta * diff * diff);

                const double beta
                    = intensity_variance.has_value()
                    ? 1.0 / (2.0 * *intensity_variance + 1e-12)
                    : kBeta;

                return std::exp(-beta * diff * diff);
            }

            _impure_(double compute_weight_local_variance(
                uint8_t g1
                , uint8_t g2
                , double local_variance_1
                , double local_variance_2))
            {
                const double diff = static_cast<double>(g1) - static_cast<double>(g2);
                const double sigma2 = 0.5 * (local_variance_1 + local_variance_2);
                const double beta = 1.0 / (2.0 * sigma2 + eps::eps(sigma2));
                return std::exp(-beta * diff * diff);
            }

            _impure_(double compute_weight_gradient(
                double grad_magnitude_1
                , double grad_magnitude_2
                , double gamma = 40.0))
            {
                const double avg_grad = 0.5 * (grad_magnitude_1 + grad_magnitude_2);
                return std::exp(-gamma / (avg_grad + eps::eps(avg_grad)));
            }
        }
    }

    namespace
    {
        enum class Direction {
            Right
            , Down
            , Left
            , Up
        };

        [[nodiscard]] constexpr std::pair<int, int> offset(Direction dir) noexcept
        {
            switch (dir) {
            case Direction::Right: return { 0, 1 };
            case Direction::Down:  return { 1, 0 };
            case Direction::Left:  return { 0, -1 };
            case Direction::Up:    return { -1, 0 };
            }
            return { 0, 0 }; // unreachable
        }

        constexpr std::array<Direction, 4> kDirections = {
            Direction::Right
            , Direction::Down
            , Direction::Left
            , Direction::Up
        };

        inline _pure_(bool is_inside(int y, int x, int height, int width))
        {
            return y >= 0 && y < height && x >= 0 && x < width;
        }

        _impure_(double estimate_gamma_from_gradient(const Eigen::MatrixXd& grad))
        {
            const double mean_grad = grad.mean();
            return std::max(eps::eps(mean_grad), mean_grad * std::log(2.0));
        }


        _impure_(Eigen::MatrixXd compute_local_variance(
            const Eigen::Matrix<uint8_t, Eigen::Dynamic
            , Eigen::Dynamic>& image
            , int window_size = 5
        ))
        {
            const int height = static_cast<int>(image.rows());
            const int width = static_cast<int>(image.cols());
            const int radius = window_size / 2;

            Eigen::MatrixXd result = Eigen::MatrixXd::Zero(height, width);
            Eigen::MatrixXd float_image = image.cast<double>();

            for (size_t y = 0; y < height; ++y)
            {
                for (size_t x = 0; x < width; ++x)
                {
                    int count = 0;
                    double sum = 0.0;
                    double sum_sq = 0.0;

                    for (int dy = -radius; dy <= radius; ++dy)
                    {
                        for (int dx = -radius; dx <= radius; ++dx)
                        {
                            const int ny = y + dy;
                            const int nx = x + dx;

                            if (ny >= 0 && ny < height && nx >= 0 && nx < width)
                            {
                                double val = float_image(ny, nx);
                                sum += val;
                                sum_sq += val * val;
                                ++count;
                            }
                        }
                    }

                    const double mean = sum / count;
                    result(y, x) = (sum_sq / count) - (mean * mean);
                }
            }

            return result;
        }

        _impure_(Eigen::MatrixXd compute_gradient_magnitude(
            const Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic>& image
        ))
        {
            const int height = static_cast<int>(image.rows());
            const int width = static_cast<int>(image.cols());

            Eigen::MatrixXd grad = Eigen::MatrixXd::Zero(height, width);
            Eigen::MatrixXd float_image = image.cast<double>();

            const std::array<std::pair<int, int>, 4> offsets = {
                std::pair{-1, 0}, std::pair{1, 0}, std::pair{0, -1}, std::pair{0, 1}
            };

            for (size_t y = 1; y < height - 1; ++y)
            {
                for (size_t x = 1; x < width - 1; ++x)
                {
                    double gx = float_image(y, x + 1) - float_image(y, x - 1);
                    double gy = float_image(y + 1, x) - float_image(y - 1, x);
                    grad(y, x) = std::hypot(gx, gy);
                }
            }

            return grad;
        }


        _impure_(double compute_intensity_variance(const Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic>& image))
        {
            Eigen::MatrixXd image_d = image.cast<double>();

            double mean = image_d.mean();
            double variance = (image_d.array() - mean).square().mean();

            return variance;
        }

        _impure_(Eigen::SparseMatrix<double> build_laplacian(const Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic>& image))
        {
            const int height = static_cast<int>(image.rows());
            const int width = static_cast<int>(image.cols());
            const int n = width * height;
			const auto intensity_variance = std::make_optional(compute_intensity_variance(image));

            auto index_at = [width](int row, int col) constexpr noexcept -> int {
                return row * width + col;
                };

            std::vector<Eigen::Triplet<double>> weight_triplets;
            weight_triplets.reserve(n * 4);

            Eigen::VectorXd degrees = Eigen::VectorXd::Zero(n);

            for (size_t y = 0; y < height; ++y)
            {
                for (size_t x = 0; x < width; ++x)
                {
                    const int i = index_at(y, x);
                    const uint8_t gi = image(y, x);

                    for (Direction dir : kDirections)
                    {
                        const auto [dy, dx] = offset(dir);
                        const int ny = y + dy;
                        const int nx = x + dx;
                        if (!is_inside(ny, nx, height, width))
                            continue;

                        const int j = index_at(ny, nx);
                        const uint8_t gj = image(ny, nx);

                        const double w = weights::compute_weight(gi, gj, intensity_variance);
                        weight_triplets.emplace_back(i, j, -w);
                        degrees[i] += w;
                    }
                }
            }

            for (size_t i = 0; i < n; ++i)
            {
                weight_triplets.emplace_back(i, i, degrees[i]);
            }

            Eigen::SparseMatrix<double> L(n, n);
            L.setFromTriplets(weight_triplets.begin(), weight_triplets.end());
            return L;
        }
    }

    PixelGraph::PixelGraph(const Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic>& image)
        : image_(image)
        , width_(static_cast<int>(image.cols()))
        , height_(static_cast<int>(image.rows()))
    {
    }

    int PixelGraph::index_at(int row, int col) const noexcept
    {
        return row * width_ + col;
    }

    std::pair<int, int> PixelGraph::coords_from_index(int index) const noexcept
    {
        return { index / width_, index % width_ };
    }

    Eigen::SparseMatrix<double> PixelGraph::laplacian() const
    {
		return build_laplacian(image_);
    }

} // namespace graph

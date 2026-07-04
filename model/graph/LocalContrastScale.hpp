#pragma once

#include "model/domain/GrayImage.hpp"
#include "model/domain/RandomWalkerParameters.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cmath>
#include <utility>
#include <vector>

#include <Eigen/Core>

namespace random_walker::graph {

    struct GridPoint {
        int row = 0;
        int column = 0;
        bool operator==(const GridPoint&) const = default;
    };

    class LocalContrastScaleMap final {
    public:
        LocalContrastScaleMap() = default;
        explicit LocalContrastScaleMap(Eigen::MatrixXd local_variances)
            : local_variances_(std::move(local_variances)) {
            assert(local_variances_.rows() >= 0);
            assert(local_variances_.cols() >= 0);
            for (Eigen::Index row = 0; row < local_variances_.rows(); ++row) {
                for (
                    Eigen::Index column = 0;
                    column < local_variances_.cols();
                    ++column
                ) {
                    const double variance = local_variances_(row, column);
                    assert(std::isfinite(variance));
                    assert(variance >= domain::kMinimumLocalContrastVariance);
                }
            }
        }

        [[nodiscard]] bool empty() const noexcept {
            return local_variances_.size() == 0;
        }

        [[nodiscard]] int width() const noexcept {
            return static_cast<int>(local_variances_.cols());
        }

        [[nodiscard]] int height() const noexcept {
            return static_cast<int>(local_variances_.rows());
        }

        [[nodiscard]] double variance_at(GridPoint point) const noexcept {
            assert(point.row >= 0);
            assert(point.column >= 0);
            assert(point.row < height());
            assert(point.column < width());
            const double variance = local_variances_(point.row, point.column);
            assert(std::isfinite(variance));
            assert(variance >= domain::kMinimumLocalContrastVariance);
            return variance;
        }

        [[nodiscard]] const Eigen::MatrixXd& variances() const noexcept {
            return local_variances_;
        }

    private:
        Eigen::MatrixXd local_variances_;
    };

    [[nodiscard]] inline double raw_local_window_variance(
        const domain::GrayImage& image
        , int center_row
        , int center_column
        , int radius
    ) noexcept {
        assert(!image.empty());
        assert(radius >= domain::kMinimumLocalContrastRadius);
        assert(radius <= domain::kMaximumLocalContrastRadius);
        assert(center_row >= 0);
        assert(center_column >= 0);
        assert(center_row < image.height());
        assert(center_column < image.width());

        const int first_row = std::max(0, center_row - radius);
        const int last_row = std::min(image.height() - 1, center_row + radius);
        const int first_column = std::max(0, center_column - radius);
        const int last_column = std::min(
            image.width() - 1
            , center_column + radius
        );

        double sum = 0.0;
        double squared_sum = 0.0;
        int count = 0;
        for (int row = first_row; row <= last_row; ++row) {
            for (int column = first_column; column <= last_column; ++column) {
                const double intensity =
                    static_cast<double>(image.at(row, column));
                sum += intensity;
                squared_sum += intensity * intensity;
                ++count;
            }
        }

        assert(count > 0);
        const double mean = sum / static_cast<double>(count);
        const double variance =
            squared_sum / static_cast<double>(count) - mean * mean;
        assert(std::isfinite(variance));
        return std::max(0.0, variance);
    }

    [[nodiscard]] inline double local_window_variance(
        const domain::GrayImage& image
        , int center_row
        , int center_column
        , const domain::EffectiveLocalContrastScale& scale
    ) noexcept {
        assert(domain::is_valid(scale));
        const double variance = raw_local_window_variance(
            image
            , center_row
            , center_column
            , scale.radius
        );
        const double clamped_variance = std::max(
            scale.minimum_variance
            , variance
        );
        assert(std::isfinite(clamped_variance));
        assert(clamped_variance >= domain::kMinimumLocalContrastVariance);
        return clamped_variance;
    }

    [[nodiscard]] inline double local_variance_quantile(
        std::vector<double> variances
        , double quantile
    ) {
        assert(!variances.empty());
        assert(std::isfinite(quantile));
        assert(quantile >= domain::kMinimumLocalContrastAutoQuantile);
        assert(quantile <= domain::kMaximumLocalContrastAutoQuantile);

        std::sort(variances.begin(), variances.end());
        const double position =
            quantile * static_cast<double>(variances.size() - std::size_t {1});
        const auto lower_index = static_cast<std::size_t>(std::floor(position));
        const auto upper_index = static_cast<std::size_t>(std::ceil(position));
        assert(lower_index < variances.size());
        assert(upper_index < variances.size());
        const double weight = position - static_cast<double>(lower_index);
        return variances[lower_index] * (1.0 - weight)
            + variances[upper_index] * weight;
    }

    [[nodiscard]] inline double estimate_minimum_variance(
        const domain::GrayImage& image
        , const domain::LocalContrastScaleParameters& parameters
    ) {
        assert(!image.empty());
        assert(domain::is_valid(parameters));

        std::vector<double> variances;
        variances.reserve(
            static_cast<std::size_t>(image.width())
            * static_cast<std::size_t>(image.height())
        );
        for (int row = 0; row < image.height(); ++row) {
            for (int column = 0; column < image.width(); ++column) {
                variances.push_back(raw_local_window_variance(
                    image
                    , row
                    , column
                    , parameters.radius
                ));
            }
        }

        return std::clamp(
            local_variance_quantile(
                std::move(variances)
                , parameters.auto_minimum_variance_quantile
            )
            , domain::kMinimumLocalContrastVariance
            , domain::kMaximumLocalContrastVariance
        );
    }

    [[nodiscard]] inline domain::EffectiveLocalContrastScale
    effective_local_contrast_scale(
        const domain::GrayImage& image
        , const domain::LocalContrastScaleParameters& parameters
    ) {
        assert(domain::is_valid(parameters));
        switch (parameters.minimum_variance_mode) {
        case domain::MinimumVarianceMode::Manual:
            return domain::manual_effective_scale(parameters);
        case domain::MinimumVarianceMode::Auto:
            return {
                .radius = parameters.radius
                , .minimum_variance = estimate_minimum_variance(
                    image
                    , parameters
                )
            };
        }

        assert(false);
        return domain::manual_effective_scale(parameters);
    }

    [[nodiscard]] inline LocalContrastScaleMap build_local_contrast_scale_map(
        const domain::GrayImage& image
        , const domain::EffectiveLocalContrastScale& scale
    ) {
        assert(!image.empty());
        assert(domain::is_valid(scale));

        Eigen::MatrixXd variances(image.height(), image.width());
        for (int row = 0; row < image.height(); ++row) {
            for (int column = 0; column < image.width(); ++column) {
                variances(row, column) = local_window_variance(
                    image
                    , row
                    , column
                    , scale
                );
            }
        }

        return LocalContrastScaleMap(std::move(variances));
    }

    [[nodiscard]] inline LocalContrastScaleMap build_local_contrast_scale_map(
        const domain::GrayImage& image
        , const domain::LocalContrastScaleParameters& parameters
    ) {
        assert(domain::is_valid(parameters));
        return build_local_contrast_scale_map(
            image
            , effective_local_contrast_scale(image, parameters)
        );
    }
}

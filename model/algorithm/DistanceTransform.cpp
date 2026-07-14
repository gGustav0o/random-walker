#include "DistanceTransform.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>

namespace random_walker::algorithm {

    std::vector<double> squared_distance_transform_1d(
        std::span<const double> samples
    ) {
        const int count = static_cast<int>(samples.size());
        std::vector<double> distances(
            samples.size()
            , std::numeric_limits<double>::infinity()
        );
        if (count == 0) {
            return distances;
        }

        const auto first_finite = std::find_if(
            samples.begin()
            , samples.end()
            , [](double value) {
                return std::isfinite(value);
            }
        );
        if (first_finite == samples.end()) {
            return distances;
        }

        std::vector<int> parabola_indices(samples.size(), 0);
        std::vector<double> breakpoints(samples.size() + 1U, 0.0);

        int envelope_index = 0;
        parabola_indices[0] = static_cast<int>(first_finite - samples.begin());
        breakpoints[0] = -std::numeric_limits<double>::infinity();
        breakpoints[1] = std::numeric_limits<double>::infinity();

        for (int sample_index = parabola_indices[0] + 1;
             sample_index < count;
             ++sample_index
        ) {
            if (!std::isfinite(samples[static_cast<std::size_t>(sample_index)])) {
                continue;
            }

            const auto intersection = [&samples](int left, int right) {
                const double left_position = static_cast<double>(left);
                const double right_position = static_cast<double>(right);
                return (
                    samples[static_cast<std::size_t>(right)]
                    + right_position * right_position
                    - samples[static_cast<std::size_t>(left)]
                    - left_position * left_position
                ) / (2.0 * (right_position - left_position));
            };

            double crossing = intersection(
                parabola_indices[static_cast<std::size_t>(envelope_index)]
                , sample_index
            );
            while (
                crossing <= breakpoints[static_cast<std::size_t>(envelope_index)]
            ) {
                --envelope_index;
                crossing = intersection(
                    parabola_indices[static_cast<std::size_t>(envelope_index)]
                    , sample_index
                );
            }

            ++envelope_index;
            parabola_indices[static_cast<std::size_t>(envelope_index)] =
                sample_index;
            breakpoints[static_cast<std::size_t>(envelope_index)] = crossing;
            breakpoints[static_cast<std::size_t>(envelope_index + 1)] =
                std::numeric_limits<double>::infinity();
        }

        envelope_index = 0;
        for (int sample_index = 0; sample_index < count; ++sample_index) {
            while (
                breakpoints[static_cast<std::size_t>(envelope_index + 1)]
                < static_cast<double>(sample_index)
            ) {
                ++envelope_index;
            }

            const int nearest_index =
                parabola_indices[static_cast<std::size_t>(envelope_index)];
            const double offset =
                static_cast<double>(sample_index - nearest_index);
            distances[static_cast<std::size_t>(sample_index)] =
                offset * offset + samples[static_cast<std::size_t>(nearest_index)];
        }

        return distances;
    }

    std::vector<double> squared_euclidean_distance_transform(
        int width
        , int height
        , std::span<const std::uint8_t> source_mask
    ) {
        assert(width >= 0);
        assert(height >= 0);
        assert(
            static_cast<std::size_t>(width) * static_cast<std::size_t>(height)
            == source_mask.size()
        );

        const std::size_t pixel_count =
            static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
        std::vector<double> column_pass(
            pixel_count
            , std::numeric_limits<double>::infinity()
        );
        std::vector<double> distances(
            pixel_count
            , std::numeric_limits<double>::infinity()
        );
        if (width == 0 || height == 0) {
            return distances;
        }

        std::vector<double> line(
            static_cast<std::size_t>(std::max(width, height))
            , 0.0
        );
        for (int column = 0; column < width; ++column) {
            for (int row = 0; row < height; ++row) {
                const std::size_t index =
                    static_cast<std::size_t>(row)
                        * static_cast<std::size_t>(width)
                    + static_cast<std::size_t>(column);
                line[static_cast<std::size_t>(row)] =
                    source_mask[index] != 0U
                    ? 0.0
                    : std::numeric_limits<double>::infinity();
            }

            const std::vector<double> transformed =
                squared_distance_transform_1d(
                    std::span<const double>(
                        line.data()
                        , static_cast<std::size_t>(height)
                    )
                );
            for (int row = 0; row < height; ++row) {
                column_pass[
                    static_cast<std::size_t>(row)
                        * static_cast<std::size_t>(width)
                    + static_cast<std::size_t>(column)
                ] = transformed[static_cast<std::size_t>(row)];
            }
        }

        for (int row = 0; row < height; ++row) {
            for (int column = 0; column < width; ++column) {
                line[static_cast<std::size_t>(column)] =
                    column_pass[
                        static_cast<std::size_t>(row)
                            * static_cast<std::size_t>(width)
                        + static_cast<std::size_t>(column)
                    ];
            }

            const std::vector<double> transformed =
                squared_distance_transform_1d(
                    std::span<const double>(
                        line.data()
                        , static_cast<std::size_t>(width)
                    )
                );
            for (int column = 0; column < width; ++column) {
                distances[
                    static_cast<std::size_t>(row)
                        * static_cast<std::size_t>(width)
                    + static_cast<std::size_t>(column)
                ] = transformed[static_cast<std::size_t>(column)];
            }
        }

        return distances;
    }
}

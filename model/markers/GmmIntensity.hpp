#pragma once

#include "model/domain/AutoMarkers.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <span>
#include <variant>
#include <vector>

namespace random_walker::markers {

    inline constexpr double kPi = 3.141592653589793238462643383279502884;

    enum class GmmError {
        EmptySamples
        , InvalidParameters
        , DegenerateSamples
        , NonFiniteLikelihood
    };

    struct GaussianComponent1d {
        double mean = 0.0;
        double variance = domain::kDefaultGmmMinimumVariance;
        double weight = 0.5;
        bool operator==(const GaussianComponent1d&) const = default;
    };

    struct GmmModel1d {
        std::array<GaussianComponent1d, domain::kGmmIntensityComponentCount>
            components;
        double log_likelihood = -std::numeric_limits<double>::infinity();
        int iterations = 0;
        bool converged = false;
    };

    using GmmFitOutcome = std::variant<GmmModel1d, GmmError>;

    [[nodiscard]] inline bool is_valid(
        const GaussianComponent1d& component
    ) noexcept {
        return std::isfinite(component.mean)
            && std::isfinite(component.variance)
            && component.variance >= domain::kMinimumGmmVariance
            && std::isfinite(component.weight)
            && component.weight >= 0.0
            && component.weight <= 1.0;
    }

    [[nodiscard]] inline bool has_valid_components(
        const GmmModel1d& model
    ) noexcept {
        return std::all_of(
            model.components.begin()
            , model.components.end()
            , [](const GaussianComponent1d& component) {
                return is_valid(component);
            }
        );
    }

    [[nodiscard]] inline bool is_valid(
        const GmmModel1d& model
    ) noexcept {
        const double weight_sum = std::accumulate(
            model.components.begin()
            , model.components.end()
            , 0.0
            , [](double sum, const GaussianComponent1d& component) {
                return sum + component.weight;
            }
        );
        return has_valid_components(model)
            && std::isfinite(model.log_likelihood)
            && model.iterations >= 0
            && std::abs(weight_sum - 1.0) < 1e-9;
    }

    [[nodiscard]] inline double gaussian_density(
        double value
        , const GaussianComponent1d& component
    ) noexcept {
        assert(std::isfinite(value));
        assert(is_valid(component));

        const double difference = value - component.mean;
        const double normalizer = std::sqrt(2.0 * kPi * component.variance);
        const double exponent = -difference * difference / (2.0 * component.variance);
        const double density = std::exp(exponent) / normalizer;
        assert(std::isfinite(density));
        assert(density >= 0.0);
        return density;
    }

    [[nodiscard]] inline std::array<double, domain::kGmmIntensityComponentCount>
    posterior_probabilities(
        const GmmModel1d& model
        , double value
    ) noexcept {
        assert(has_valid_components(model));
        assert(std::isfinite(value));

        std::array<double, domain::kGmmIntensityComponentCount> weighted {};
        for (std::size_t component_index = 0;
             component_index < weighted.size();
             ++component_index
        ) {
            const GaussianComponent1d& component = model.components[component_index];
            weighted[component_index] = component.weight
                * gaussian_density(value, component);
        }

        const double total = std::accumulate(
            weighted.begin()
            , weighted.end()
            , 0.0
        );
        if (!(total > 0.0) || !std::isfinite(total)) {
            return {0.5, 0.5};
        }

        for (double& probability : weighted) {
            probability /= total;
            assert(std::isfinite(probability));
            assert(probability >= 0.0);
            assert(probability <= 1.0);
        }
        return weighted;
    }

    [[nodiscard]] inline std::size_t foreground_component_index(
        const GmmModel1d& model
        , domain::ForegroundPolarity polarity
    ) noexcept {
        assert(is_valid(model));
        assert(domain::is_valid(polarity));

        const auto compare_by_mean = [](const GaussianComponent1d& first,
                                        const GaussianComponent1d& second) {
            return first.mean < second.mean;
        };
        const auto begin = model.components.begin();
        const auto end = model.components.end();
        const auto iterator = polarity == domain::ForegroundPolarity::DarkObject
            ? std::min_element(begin, end, compare_by_mean)
            : std::max_element(begin, end, compare_by_mean);
        return static_cast<std::size_t>(std::distance(begin, iterator));
    }

    [[nodiscard]] inline double foreground_probability(
        const GmmModel1d& model
        , double value
        , domain::ForegroundPolarity polarity
    ) noexcept {
        const std::array<double, domain::kGmmIntensityComponentCount> posterior =
            posterior_probabilities(model, value);
        return posterior[foreground_component_index(model, polarity)];
    }

    [[nodiscard]] inline double sample_mean(
        std::span<const double> samples
    ) noexcept {
        assert(!samples.empty());
        const double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
        return sum / static_cast<double>(samples.size());
    }

    [[nodiscard]] inline double sample_variance(
        std::span<const double> samples
        , double mean
        , double minimum_variance
    ) noexcept {
        assert(!samples.empty());
        assert(std::isfinite(mean));
        assert(std::isfinite(minimum_variance));
        assert(minimum_variance >= domain::kMinimumGmmVariance);

        const double squared_sum = std::accumulate(
            samples.begin()
            , samples.end()
            , 0.0
            , [mean](double sum, double value) {
                const double difference = value - mean;
                return sum + difference * difference;
            }
        );
        return std::max(
            minimum_variance
            , squared_sum / static_cast<double>(samples.size())
        );
    }

    [[nodiscard]] inline GmmModel1d initialize_gmm(
        std::span<const double> samples
        , const domain::GmmParameters& parameters
    ) {
        assert(!samples.empty());
        assert(domain::is_valid(parameters));

        const auto [minimum, maximum] = std::minmax_element(
            samples.begin()
            , samples.end()
        );
        const double mean = sample_mean(samples);
        const double variance = sample_variance(
            samples
            , mean
            , parameters.minimum_variance
        );

        return {
            .components = {
                GaussianComponent1d {
                    .mean = *minimum
                    , .variance = variance
                    , .weight = 0.5
                }
                , GaussianComponent1d {
                    .mean = *maximum
                    , .variance = variance
                    , .weight = 0.5
                }
            }
        };
    }

    [[nodiscard]] inline double log_likelihood(
        std::span<const double> samples
        , const GmmModel1d& model
    ) noexcept {
        assert(!samples.empty());
        assert(has_valid_components(model));

        double result = 0.0;
        for (const double sample : samples) {
            double density = 0.0;
            for (const GaussianComponent1d& component : model.components) {
                density += component.weight * gaussian_density(sample, component);
            }
            if (!(density > 0.0) || !std::isfinite(density)) {
                return -std::numeric_limits<double>::infinity();
            }
            result += std::log(density);
        }
        return result;
    }

    [[nodiscard]] inline GmmFitOutcome fit_gmm(
        std::span<const double> samples
        , const domain::GmmParameters& parameters
    ) {
        if (!domain::is_valid(parameters)) {
            return GmmError::InvalidParameters;
        }
        if (samples.empty()) {
            return GmmError::EmptySamples;
        }
        if (!std::all_of(samples.begin(), samples.end(), [](double value) {
                return std::isfinite(value);
            })
        ) {
            return GmmError::DegenerateSamples;
        }

        GmmModel1d model = initialize_gmm(samples, parameters);
        double previous_likelihood = -std::numeric_limits<double>::infinity();

        for (int iteration = 1; iteration <= parameters.max_iterations; ++iteration) {
            std::array<double, domain::kGmmIntensityComponentCount>
                responsibility_sum {};
            std::array<double, domain::kGmmIntensityComponentCount>
                weighted_sum {};
            std::array<double, domain::kGmmIntensityComponentCount>
                weighted_squared_deviation_sum {};
            std::vector<std::array<double, domain::kGmmIntensityComponentCount>>
                responsibilities;
            responsibilities.reserve(samples.size());

            for (const double sample : samples) {
                const auto posterior = posterior_probabilities(model, sample);
                responsibilities.push_back(posterior);
                for (std::size_t component_index = 0;
                     component_index < posterior.size();
                     ++component_index
                ) {
                    responsibility_sum[component_index] += posterior[component_index];
                    weighted_sum[component_index] += posterior[component_index] * sample;
                }
            }

            for (std::size_t component_index = 0;
                 component_index < model.components.size();
                 ++component_index
            ) {
                const double effective_count = responsibility_sum[component_index];
                if (!(effective_count > 0.0) || !std::isfinite(effective_count)) {
                    return GmmError::DegenerateSamples;
                }

                model.components[component_index].mean =
                    weighted_sum[component_index] / effective_count;
                model.components[component_index].weight =
                    effective_count / static_cast<double>(samples.size());
            }

            for (std::size_t sample_index = 0;
                 sample_index < samples.size();
                 ++sample_index
            ) {
                const double sample = samples[sample_index];
                for (std::size_t component_index = 0;
                     component_index < model.components.size();
                     ++component_index
                ) {
                    const double difference =
                        sample - model.components[component_index].mean;
                    weighted_squared_deviation_sum[component_index] +=
                        responsibilities[sample_index][component_index]
                        * difference
                        * difference;
                }
            }

            for (std::size_t component_index = 0;
                 component_index < model.components.size();
                 ++component_index
            ) {
                const double variance = weighted_squared_deviation_sum[component_index]
                    / responsibility_sum[component_index];
                model.components[component_index].variance = std::max(
                    parameters.minimum_variance
                    , variance
                );
            }

            model.iterations = iteration;
            model.log_likelihood = log_likelihood(samples, model);
            if (!std::isfinite(model.log_likelihood)) {
                return GmmError::NonFiniteLikelihood;
            }

            if (std::isfinite(previous_likelihood)
                && std::abs(model.log_likelihood - previous_likelihood)
                    <= parameters.convergence_tolerance
            ) {
                model.converged = true;
                break;
            }
            previous_likelihood = model.log_likelihood;
        }

        std::sort(
            model.components.begin()
            , model.components.end()
            , [](const GaussianComponent1d& first,
                 const GaussianComponent1d& second) {
                return first.mean < second.mean;
            }
        );
        assert(is_valid(model));
        return model;
    }
}

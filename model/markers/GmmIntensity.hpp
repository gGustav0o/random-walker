#pragma once

#include "model/domain/AutoMarkers.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <span>
#include <variant>
#include <vector>

namespace random_walker::markers {

    inline constexpr std::size_t kGrayscaleIntensityBinCount = 256;

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

    struct WeightedIntensitySample {
        double value = 0.0;
        std::size_t count = 0;
        bool operator==(const WeightedIntensitySample&) const = default;
    };

    struct IntensityHistogram {
        std::array<std::size_t, kGrayscaleIntensityBinCount> counts {};
        std::size_t sample_count = 0;
        bool operator==(const IntensityHistogram&) const = default;
    };

    using GmmFitOutcome = std::variant<GmmModel1d, GmmError>;

    [[nodiscard]] bool is_valid(
        const GaussianComponent1d& component
    ) noexcept;

    [[nodiscard]] bool is_valid(
        const GmmModel1d& model
    ) noexcept;

    [[nodiscard]] bool is_valid(
        const WeightedIntensitySample& sample
    ) noexcept;

    [[nodiscard]] std::array<double, domain::kGmmIntensityComponentCount>
    posterior_probabilities(
        const GmmModel1d& model
        , double value
    ) noexcept;

    [[nodiscard]] double foreground_probability(
        const GmmModel1d& model
        , double value
        , domain::ForegroundPolarity polarity
    ) noexcept;

    [[nodiscard]] GmmFitOutcome fit_gmm(
        std::span<const WeightedIntensitySample> samples
        , const domain::GmmParameters& parameters
    );

    [[nodiscard]] GmmFitOutcome fit_gmm(
        const IntensityHistogram& histogram
        , const domain::GmmParameters& parameters
    );

    [[nodiscard]] GmmFitOutcome fit_gmm(
        std::span<const double> samples
        , const domain::GmmParameters& parameters
    );
}

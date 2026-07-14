#include <cmath>
#include <limits>
#include <variant>
#include <vector>

#include <QtTest>

#include "model/domain/AutoMarkers.hpp"
#include "model/markers/GmmIntensity.hpp"

namespace domain = random_walker::domain;
namespace markers = random_walker::markers;

class GmmIntensityTests final : public QObject {
    Q_OBJECT

private slots:
    void rejects_empty_samples();
    void rejects_invalid_parameters();
    void separates_two_intensity_modes_deterministically();
    void histogram_fit_matches_repeated_samples_fit();
    void posterior_is_high_near_component_centers();
    void foreground_probability_respects_polarity();
    void clamps_component_variance_to_minimum();
};

void GmmIntensityTests::rejects_empty_samples() {
    const std::vector<double> samples;

    const markers::GmmFitOutcome outcome = markers::fit_gmm(
        samples
        , domain::GmmParameters {}
    );

    QVERIFY(std::holds_alternative<markers::GmmError>(outcome));
    QCOMPARE(
        static_cast<int>(std::get<markers::GmmError>(outcome))
        , static_cast<int>(markers::GmmError::EmptySamples)
    );
}

void GmmIntensityTests::rejects_invalid_parameters() {
    const std::vector<double> samples {0.0, 1.0};

    const markers::GmmFitOutcome outcome = markers::fit_gmm(
        samples
        , domain::GmmParameters {.component_count = 3}
    );

    QVERIFY(std::holds_alternative<markers::GmmError>(outcome));
    QCOMPARE(
        static_cast<int>(std::get<markers::GmmError>(outcome))
        , static_cast<int>(markers::GmmError::InvalidParameters)
    );
}

void GmmIntensityTests::separates_two_intensity_modes_deterministically() {
    const std::vector<double> samples {
        0.0, 1.0, 0.0, 1.0,
        200.0, 201.0, 200.0, 201.0
    };

    const markers::GmmFitOutcome outcome = markers::fit_gmm(
        samples
        , domain::GmmParameters {
            .max_iterations = 100
            , .convergence_tolerance = 1e-10
            , .minimum_variance = 1.0
        }
    );

    QVERIFY(std::holds_alternative<markers::GmmModel1d>(outcome));
    const markers::GmmModel1d& model = std::get<markers::GmmModel1d>(outcome);
    QVERIFY(markers::is_valid(model));
    QVERIFY(model.converged);
    QVERIFY(model.components[0].mean < model.components[1].mean);
    QVERIFY(std::abs(model.components[0].mean - 0.5) < 1e-6);
    QVERIFY(std::abs(model.components[1].mean - 200.5) < 1e-6);
    QVERIFY(std::abs(model.components[0].weight - 0.5) < 1e-6);
    QVERIFY(std::abs(model.components[1].weight - 0.5) < 1e-6);
}

void GmmIntensityTests::histogram_fit_matches_repeated_samples_fit() {
    const std::vector<double> samples {
        0.0, 1.0, 0.0, 1.0,
        200.0, 201.0, 200.0, 201.0
    };
    markers::IntensityHistogram histogram;
    histogram.counts[0] = 2;
    histogram.counts[1] = 2;
    histogram.counts[200] = 2;
    histogram.counts[201] = 2;
    histogram.sample_count = samples.size();
    const domain::GmmParameters parameters {
        .max_iterations = 100
        , .convergence_tolerance = 1e-10
        , .minimum_variance = 1.0
    };

    const markers::GmmFitOutcome sample_outcome =
        markers::fit_gmm(samples, parameters);
    const markers::GmmFitOutcome histogram_outcome =
        markers::fit_gmm(histogram, parameters);

    QVERIFY(std::holds_alternative<markers::GmmModel1d>(sample_outcome));
    QVERIFY(std::holds_alternative<markers::GmmModel1d>(histogram_outcome));
    const markers::GmmModel1d& sample_model =
        std::get<markers::GmmModel1d>(sample_outcome);
    const markers::GmmModel1d& histogram_model =
        std::get<markers::GmmModel1d>(histogram_outcome);

    QCOMPARE(histogram_model.iterations, sample_model.iterations);
    QCOMPARE(histogram_model.converged, sample_model.converged);
    QVERIFY(std::abs(
        histogram_model.log_likelihood - sample_model.log_likelihood
    ) < 1e-9);
    for (std::size_t index = 0;
         index < histogram_model.components.size();
         ++index
    ) {
        QVERIFY(std::abs(
            histogram_model.components[index].mean
                - sample_model.components[index].mean
        ) < 1e-9);
        QVERIFY(std::abs(
            histogram_model.components[index].variance
                - sample_model.components[index].variance
        ) < 1e-9);
        QVERIFY(std::abs(
            histogram_model.components[index].weight
                - sample_model.components[index].weight
        ) < 1e-9);
    }
}

void GmmIntensityTests::posterior_is_high_near_component_centers() {
    const std::vector<double> samples {
        0.0, 1.0, 0.0, 1.0,
        200.0, 201.0, 200.0, 201.0
    };
    const auto outcome = markers::fit_gmm(samples, domain::GmmParameters {});
    QVERIFY(std::holds_alternative<markers::GmmModel1d>(outcome));
    const markers::GmmModel1d& model = std::get<markers::GmmModel1d>(outcome);

    const auto dark_posterior = markers::posterior_probabilities(model, 0.0);
    const auto bright_posterior = markers::posterior_probabilities(model, 201.0);

    QVERIFY(dark_posterior[0] > 0.99);
    QVERIFY(dark_posterior[1] < 0.01);
    QVERIFY(bright_posterior[0] < 0.01);
    QVERIFY(bright_posterior[1] > 0.99);
}

void GmmIntensityTests::foreground_probability_respects_polarity() {
    const std::vector<double> samples {
        0.0, 1.0, 0.0, 1.0,
        200.0, 201.0, 200.0, 201.0
    };
    const auto outcome = markers::fit_gmm(samples, domain::GmmParameters {});
    QVERIFY(std::holds_alternative<markers::GmmModel1d>(outcome));
    const markers::GmmModel1d& model = std::get<markers::GmmModel1d>(outcome);

    QVERIFY(markers::foreground_probability(
        model
        , 0.0
        , domain::ForegroundPolarity::DarkObject
    ) > 0.99);
    QVERIFY(markers::foreground_probability(
        model
        , 201.0
        , domain::ForegroundPolarity::BrightObject
    ) > 0.99);
}

void GmmIntensityTests::clamps_component_variance_to_minimum() {
    const std::vector<double> samples {
        10.0, 10.0, 10.0,
        20.0, 20.0, 20.0
    };
    constexpr double minimum_variance = 2.0;

    const auto outcome = markers::fit_gmm(
        samples
        , domain::GmmParameters {.minimum_variance = minimum_variance}
    );

    QVERIFY(std::holds_alternative<markers::GmmModel1d>(outcome));
    const markers::GmmModel1d& model = std::get<markers::GmmModel1d>(outcome);
    QVERIFY(markers::is_valid(model));
    for (const markers::GaussianComponent1d& component : model.components) {
        QCOMPARE(component.variance, minimum_variance);
    }
}

QTEST_GUILESS_MAIN(GmmIntensityTests)
#include "GmmIntensityTests.moc"

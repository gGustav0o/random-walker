#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include <QtTest>

#include "model/domain/AutoMarkers.hpp"
#include "model/domain/GrayImage.hpp"
#include "model/domain/RandomWalkerParameters.hpp"
#include "model/domain/Seed.hpp"
#include "model/domain/Segmentation.hpp"

namespace domain = random_walker::domain;

class DomainTests final : public QObject {
    Q_OBJECT

private slots:
    void default_random_walker_parameters_are_valid();
    void random_walker_beta_accepts_closed_valid_range();
    void random_walker_beta_rejects_values_outside_valid_range();
    void random_walker_distance_power_accepts_closed_valid_range();
    void random_walker_distance_power_rejects_values_outside_valid_range();
    void random_walker_edge_weight_model_accepts_known_values();
    void random_walker_edge_weight_model_rejects_unknown_values();
    void local_contrast_scale_parameters_accept_valid_bounds();
    void local_contrast_scale_parameters_reject_invalid_values();
    void effective_local_contrast_scale_accepts_valid_bounds();
    void effective_local_contrast_scale_rejects_invalid_values();
    void auto_marker_parameters_accept_defaults_and_bounds();
    void auto_marker_parameters_reject_invalid_values();
    void marker_label_mask_stores_labels_and_counts_seeds();
    void marker_label_mask_converts_to_automatic_seed_regions();
    void gray_image_reports_empty_default_state();
    void gray_image_reports_dimensions_and_pixel_values();
    void seed_pixel_count_counts_only_requested_label();
    void segmentation_request_exposes_immutable_input_contract();
};

void DomainTests::default_random_walker_parameters_are_valid() {
    const domain::RandomWalkerParameters parameters;

    QVERIFY(domain::is_valid(parameters));
    QCOMPARE(parameters.beta, domain::kDefaultRandomWalkerBeta);
    QCOMPARE(
        static_cast<int>(parameters.connectivity)
        , static_cast<int>(domain::kDefaultPixelConnectivity)
    );
    QCOMPARE(
        parameters.distance_power
        , domain::kDefaultRandomWalkerDistancePower
    );
    QCOMPARE(
        static_cast<int>(parameters.edge_weight_model)
        , static_cast<int>(domain::kDefaultEdgeWeightModel)
    );
    QCOMPARE(
        parameters.local_contrast_scale.radius
        , domain::kDefaultLocalContrastRadius
    );
    QCOMPARE(
        parameters.local_contrast_scale.minimum_variance
        , domain::kDefaultLocalContrastMinimumVariance
    );
    QCOMPARE(
        static_cast<int>(parameters.local_contrast_scale.minimum_variance_mode)
        , static_cast<int>(domain::kDefaultMinimumVarianceMode)
    );
    QCOMPARE(
        parameters.local_contrast_scale.auto_minimum_variance_quantile
        , domain::kDefaultLocalContrastAutoQuantile
    );
}


void DomainTests::random_walker_beta_accepts_closed_valid_range() {
    QVERIFY(domain::is_valid(domain::RandomWalkerParameters {
        .beta = domain::kMinimumRandomWalkerBeta
    }));
    QVERIFY(domain::is_valid(domain::RandomWalkerParameters {
        .beta = domain::kMaximumRandomWalkerBeta
    }));
}

void DomainTests::random_walker_beta_rejects_values_outside_valid_range() {
    QVERIFY(!domain::is_valid(domain::RandomWalkerParameters {
        .beta = std::nextafter(
            domain::kMinimumRandomWalkerBeta
            , 0.0
        )
    }));
    QVERIFY(!domain::is_valid(domain::RandomWalkerParameters {
        .beta = std::nextafter(
            domain::kMaximumRandomWalkerBeta
            , 1.0
        )
    }));
    QVERIFY(!domain::is_valid(domain::RandomWalkerParameters {
        .beta = std::numeric_limits<double>::quiet_NaN()
    }));
    QVERIFY(!domain::is_valid(domain::RandomWalkerParameters {
        .beta = std::numeric_limits<double>::infinity()
    }));
}

void DomainTests::random_walker_distance_power_accepts_closed_valid_range() {
    QVERIFY(domain::is_valid(domain::RandomWalkerParameters {
        .distance_power = domain::kMinimumRandomWalkerDistancePower
    }));
    QVERIFY(domain::is_valid(domain::RandomWalkerParameters {
        .distance_power = domain::kMaximumRandomWalkerDistancePower
    }));
}

void DomainTests::random_walker_distance_power_rejects_values_outside_valid_range() {
    QVERIFY(!domain::is_valid(domain::RandomWalkerParameters {
        .distance_power = std::nextafter(
            domain::kMinimumRandomWalkerDistancePower
            , -1.0
        )
    }));
    QVERIFY(!domain::is_valid(domain::RandomWalkerParameters {
        .distance_power = std::nextafter(
            domain::kMaximumRandomWalkerDistancePower
            , 3.0
        )
    }));
    QVERIFY(!domain::is_valid(domain::RandomWalkerParameters {
        .distance_power = std::numeric_limits<double>::quiet_NaN()
    }));
    QVERIFY(!domain::is_valid(domain::RandomWalkerParameters {
        .distance_power = std::numeric_limits<double>::infinity()
    }));
}

void DomainTests::random_walker_edge_weight_model_accepts_known_values() {
    QVERIFY(domain::is_valid(domain::RandomWalkerParameters {
        .edge_weight_model = domain::EdgeWeightModel::GlobalBeta
    }));
    QVERIFY(domain::is_valid(domain::RandomWalkerParameters {
        .edge_weight_model = domain::EdgeWeightModel::LocalVarianceNormalized
    }));
}

void DomainTests::random_walker_edge_weight_model_rejects_unknown_values() {
    QVERIFY(!domain::is_valid(domain::RandomWalkerParameters {
        .edge_weight_model = static_cast<domain::EdgeWeightModel>(-1)
    }));
}

void DomainTests::local_contrast_scale_parameters_accept_valid_bounds() {
    QVERIFY(domain::is_valid(domain::LocalContrastScaleParameters {
        .radius = domain::kMinimumLocalContrastRadius
        , .minimum_variance = domain::kMinimumLocalContrastVariance
        , .minimum_variance_mode = domain::MinimumVarianceMode::Manual
        , .auto_minimum_variance_quantile =
            domain::kMinimumLocalContrastAutoQuantile
    }));
    QVERIFY(domain::is_valid(domain::LocalContrastScaleParameters {
        .radius = domain::kMaximumLocalContrastRadius
        , .minimum_variance = domain::kMaximumLocalContrastVariance
        , .minimum_variance_mode = domain::MinimumVarianceMode::Auto
        , .auto_minimum_variance_quantile =
            domain::kMaximumLocalContrastAutoQuantile
    }));
}

void DomainTests::local_contrast_scale_parameters_reject_invalid_values() {
    QVERIFY(!domain::is_valid(domain::LocalContrastScaleParameters {
        .radius = domain::kMinimumLocalContrastRadius - 1
    }));
    QVERIFY(!domain::is_valid(domain::LocalContrastScaleParameters {
        .radius = domain::kMaximumLocalContrastRadius + 1
    }));
    QVERIFY(!domain::is_valid(domain::LocalContrastScaleParameters {
        .minimum_variance = std::nextafter(
            domain::kMinimumLocalContrastVariance
            , 0.0
        )
    }));
    QVERIFY(!domain::is_valid(domain::LocalContrastScaleParameters {
        .minimum_variance = std::nextafter(
            domain::kMaximumLocalContrastVariance
            , domain::kMaximumLocalContrastVariance + 1.0
        )
    }));
    QVERIFY(!domain::is_valid(domain::LocalContrastScaleParameters {
        .minimum_variance = std::numeric_limits<double>::quiet_NaN()
    }));
    QVERIFY(!domain::is_valid(domain::LocalContrastScaleParameters {
        .minimum_variance = std::numeric_limits<double>::infinity()
    }));
    QVERIFY(!domain::is_valid(domain::LocalContrastScaleParameters {
        .minimum_variance_mode = static_cast<domain::MinimumVarianceMode>(-1)
    }));
    QVERIFY(!domain::is_valid(domain::LocalContrastScaleParameters {
        .auto_minimum_variance_quantile = std::nextafter(
            domain::kMinimumLocalContrastAutoQuantile
            , 0.0
        )
    }));
    QVERIFY(!domain::is_valid(domain::LocalContrastScaleParameters {
        .auto_minimum_variance_quantile = std::nextafter(
            domain::kMaximumLocalContrastAutoQuantile
            , domain::kMaximumLocalContrastAutoQuantile + 1.0
        )
    }));
    QVERIFY(!domain::is_valid(domain::LocalContrastScaleParameters {
        .auto_minimum_variance_quantile =
            std::numeric_limits<double>::quiet_NaN()
    }));
}

void DomainTests::effective_local_contrast_scale_accepts_valid_bounds() {
    QVERIFY(domain::is_valid(domain::EffectiveLocalContrastScale {
        .radius = domain::kMinimumLocalContrastRadius
        , .minimum_variance = domain::kMinimumLocalContrastVariance
    }));
    QVERIFY(domain::is_valid(domain::EffectiveLocalContrastScale {
        .radius = domain::kMaximumLocalContrastRadius
        , .minimum_variance = domain::kMaximumLocalContrastVariance
    }));

    const domain::LocalContrastScaleParameters parameters {
        .radius = 2
        , .minimum_variance = 3.0
        , .minimum_variance_mode = domain::MinimumVarianceMode::Manual
    };
    const domain::EffectiveLocalContrastScale scale =
        domain::manual_effective_scale(parameters);
    QCOMPARE(scale.radius, parameters.radius);
    QCOMPARE(scale.minimum_variance, parameters.minimum_variance);
}

void DomainTests::effective_local_contrast_scale_rejects_invalid_values() {
    QVERIFY(!domain::is_valid(domain::EffectiveLocalContrastScale {
        .radius = domain::kMinimumLocalContrastRadius - 1
    }));
    QVERIFY(!domain::is_valid(domain::EffectiveLocalContrastScale {
        .radius = domain::kMaximumLocalContrastRadius + 1
    }));
    QVERIFY(!domain::is_valid(domain::EffectiveLocalContrastScale {
        .minimum_variance = std::nextafter(
            domain::kMinimumLocalContrastVariance
            , 0.0
        )
    }));
    QVERIFY(!domain::is_valid(domain::EffectiveLocalContrastScale {
        .minimum_variance = std::numeric_limits<double>::quiet_NaN()
    }));
}

void DomainTests::auto_marker_parameters_accept_defaults_and_bounds() {
    const domain::AutoMarkerParameters defaults;
    QVERIFY(domain::is_valid(defaults));

    QVERIFY(domain::is_valid(domain::AutoMarkerParameters {
        .gmm = {
            .component_count = domain::kGmmIntensityComponentCount
            , .max_iterations = domain::kMinimumGmmIterations
            , .convergence_tolerance = domain::kMinimumGmmConvergenceTolerance
            , .minimum_variance = domain::kMinimumGmmVariance
        }
        , .confidence_threshold = domain::kMinimumAutoMarkerConfidenceThreshold
        , .minimum_component_area = domain::kMinimumAutoMarkerComponentArea
        , .erosion_radius = domain::kMinimumAutoMarkerErosionRadius
        , .foreground_polarity = domain::ForegroundPolarity::DarkObject
    }));
    QVERIFY(domain::is_valid(domain::AutoMarkerParameters {
        .gmm = {
            .component_count = domain::kGmmIntensityComponentCount
            , .max_iterations = domain::kMaximumGmmIterations
            , .convergence_tolerance = domain::kMaximumGmmConvergenceTolerance
            , .minimum_variance = domain::kMaximumGmmVariance
        }
        , .confidence_threshold = domain::kMaximumAutoMarkerConfidenceThreshold
        , .minimum_component_area = domain::kMaximumAutoMarkerComponentArea
        , .erosion_radius = domain::kMaximumAutoMarkerErosionRadius
        , .foreground_polarity = domain::ForegroundPolarity::BrightObject
    }));
}

void DomainTests::auto_marker_parameters_reject_invalid_values() {
    QVERIFY(!domain::is_valid(domain::GmmParameters {
        .component_count = domain::kGmmIntensityComponentCount + 1
    }));
    QVERIFY(!domain::is_valid(domain::GmmParameters {
        .max_iterations = domain::kMinimumGmmIterations - 1
    }));
    QVERIFY(!domain::is_valid(domain::GmmParameters {
        .convergence_tolerance = std::numeric_limits<double>::quiet_NaN()
    }));
    QVERIFY(!domain::is_valid(domain::GmmParameters {
        .minimum_variance = std::nextafter(domain::kMinimumGmmVariance, 0.0)
    }));
    QVERIFY(!domain::is_valid(domain::AutoMarkerParameters {
        .confidence_threshold = std::nextafter(
            domain::kMinimumAutoMarkerConfidenceThreshold
            , 0.0
        )
    }));
    QVERIFY(!domain::is_valid(domain::AutoMarkerParameters {
        .minimum_component_area = domain::kMinimumAutoMarkerComponentArea - 1
    }));
    QVERIFY(!domain::is_valid(domain::AutoMarkerParameters {
        .erosion_radius = domain::kMaximumAutoMarkerErosionRadius + 1
    }));
    QVERIFY(!domain::is_valid(domain::AutoMarkerParameters {
        .foreground_polarity = static_cast<domain::ForegroundPolarity>(-1)
    }));
}

void DomainTests::marker_label_mask_stores_labels_and_counts_seeds() {
    domain::MarkerLabelMask mask(3, 2);

    QVERIFY(!mask.empty());
    QCOMPARE(mask.width(), 3);
    QCOMPARE(mask.height(), 2);
    QCOMPARE(mask.size(), std::size_t {6});
    QCOMPARE(mask.seed_count(), std::size_t {0});

    mask.set(0, 1, domain::MarkerLabel::Background);
    mask.set(1, 2, domain::MarkerLabel::Object);

    QCOMPARE(
        static_cast<int>(mask.at(0, 1))
        , static_cast<int>(domain::MarkerLabel::Background)
    );
    QCOMPARE(
        static_cast<int>(mask.at(1, 2))
        , static_cast<int>(domain::MarkerLabel::Object)
    );
    QCOMPARE(mask.count(domain::MarkerLabel::Background), std::size_t {1});
    QCOMPARE(mask.count(domain::MarkerLabel::Object), std::size_t {1});
    QCOMPARE(mask.seed_count(), std::size_t {2});
}

void DomainTests::marker_label_mask_converts_to_automatic_seed_regions() {
    domain::MarkerLabelMask mask(3, 2);
    mask.set(0, 1, domain::MarkerLabel::Background);
    mask.set(1, 2, domain::MarkerLabel::Object);

    const std::vector<domain::SeedRegion> regions =
        domain::seed_regions_from_marker_mask(mask);

    QCOMPARE(regions.size(), std::size_t {2});
    QCOMPARE(regions[0].area.x, 1);
    QCOMPARE(regions[0].area.y, 0);
    QCOMPARE(regions[0].area.width, 1);
    QCOMPARE(regions[0].area.height, 1);
    QCOMPARE(
        static_cast<int>(regions[0].label)
        , static_cast<int>(domain::SeedLabel::Background)
    );
    QCOMPARE(
        static_cast<int>(regions[0].source)
        , static_cast<int>(domain::SeedSource::Automatic)
    );
    QCOMPARE(regions[1].area.x, 2);
    QCOMPARE(regions[1].area.y, 1);
    QCOMPARE(
        static_cast<int>(regions[1].label)
        , static_cast<int>(domain::SeedLabel::Object)
    );
    QCOMPARE(
        static_cast<int>(regions[1].source)
        , static_cast<int>(domain::SeedSource::Automatic)
    );
}

void DomainTests::gray_image_reports_empty_default_state() {
    const domain::GrayImage image;

    QVERIFY(image.empty());
    QCOMPARE(image.width(), 0);
    QCOMPARE(image.height(), 0);
    QCOMPARE(static_cast<int>(image.pixels().size()), 0);
}

void DomainTests::gray_image_reports_dimensions_and_pixel_values() {
    domain::GrayImageMatrix pixels(2, 3);
    pixels << std::uint8_t {1}, std::uint8_t {2}, std::uint8_t {3},
        std::uint8_t {4}, std::uint8_t {5}, std::uint8_t {6};

    const domain::GrayImage image(std::move(pixels));

    QVERIFY(!image.empty());
    QCOMPARE(image.width(), 3);
    QCOMPARE(image.height(), 2);
    QCOMPARE(static_cast<int>(image.at(0, 0)), 1);
    QCOMPARE(static_cast<int>(image.at(1, 2)), 6);
}

void DomainTests::seed_pixel_count_counts_only_requested_label() {
    const std::vector<domain::SeedRegion> regions {
        domain::SeedRegion {
            .area = {.x = 0, .y = 0, .width = 2, .height = 3},
            .label = domain::SeedLabel::Background
        },
        domain::SeedRegion {
            .area = {.x = 1, .y = 1, .width = 4, .height = 1},
            .label = domain::SeedLabel::Object
        },
        domain::SeedRegion {
            .area = {.x = 2, .y = 2, .width = 1, .height = 2},
            .label = domain::SeedLabel::Background
        },
        domain::SeedRegion {
            .area = {.x = 3, .y = 3, .width = 0, .height = 5},
            .label = domain::SeedLabel::Object
        }
    };

    QCOMPARE(
        domain::seed_pixel_count(regions, domain::SeedLabel::Background)
        , 8
    );
    QCOMPARE(
        domain::seed_pixel_count(regions, domain::SeedLabel::Object)
        , 4
    );
    QVERIFY(domain::has_seed_label(regions, domain::SeedLabel::Background));
    QVERIFY(domain::has_seed_label(regions, domain::SeedLabel::Object));
    QCOMPARE(domain::valid_seed_pixel_count(regions), std::size_t {12});
}

void DomainTests::segmentation_request_exposes_immutable_input_contract() {
    domain::GrayImageMatrix pixels(1, 2);
    pixels << std::uint8_t {10}, std::uint8_t {20};

    std::vector<domain::SeedRegion> seeds {
        domain::SeedRegion {
            .area = {.x = 0, .y = 0, .width = 1, .height = 1},
            .label = domain::SeedLabel::Background
        },
        domain::SeedRegion {
            .area = {.x = 1, .y = 0, .width = 1, .height = 1},
            .label = domain::SeedLabel::Object
        }
    };

    const domain::RandomWalkerParameters parameters {.beta = 0.01};
    const domain::SegmentationRequest request(
        42
        , domain::GrayImage(std::move(pixels))
        , std::move(seeds)
        , parameters
    );

    QCOMPARE(request.request_id(), 42);
    QCOMPARE(request.image().width(), 2);
    QCOMPARE(request.image().height(), 1);
    QCOMPARE(static_cast<int>(request.image().at(0, 1)), 20);
    QCOMPARE(request.seed_regions().size(), std::size_t {2});
    QCOMPARE(
        static_cast<int>(request.seed_regions()[0].label)
        , static_cast<int>(domain::SeedLabel::Background)
    );
    QCOMPARE(
        static_cast<int>(request.seed_regions()[1].label)
        , static_cast<int>(domain::SeedLabel::Object)
    );
    QVERIFY(request.parameters() == parameters);
}

QTEST_GUILESS_MAIN(DomainTests)
#include "DomainTests.moc"

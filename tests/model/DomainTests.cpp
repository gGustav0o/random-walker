#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include <QtTest>

#include "model/domain/AutoMarkers.hpp"
#include "model/domain/GrayImage.hpp"
#include "model/domain/ImageGeometry.hpp"
#include "model/domain/RandomWalkerParameters.hpp"
#include "model/domain/Seed.hpp"
#include "model/domain/Segmentation.hpp"
#include "model/domain/SegmentationConstraintsValidation.hpp"
#include "model/domain/SegmentationLimits.hpp"

namespace domain = random_walker::domain;

class DomainTests final : public QObject {
    Q_OBJECT

private slots:
    void default_random_walker_parameters_are_valid();
    void random_walker_beta_accepts_closed_valid_range();
    void random_walker_beta_rejects_values_outside_valid_range();
    void random_walker_distance_power_accepts_closed_valid_range();
    void random_walker_distance_power_rejects_values_outside_valid_range();
    void random_walker_parameters_report_first_invalid_field();
    void pixel_connectivity_rejects_unknown_values();
    void auto_marker_parameters_accept_defaults_and_bounds();
    void auto_marker_parameters_reject_invalid_values();
    void marker_label_mask_stores_labels_and_counts_seeds();
    void image_geometry_accepts_representable_pixel_count();
    void image_geometry_rejects_unrepresentable_pixel_count();
    void segmentation_limits_accept_supported_pixel_count();
    void segmentation_limits_reject_oversized_pixel_count();
    void image_geometry_flattens_pixel_coordinates();
    void gray_image_reports_empty_default_state();
    void gray_image_reports_dimensions_and_pixel_values();
    void seed_pixel_count_counts_only_requested_label();
    void segmentation_request_exposes_immutable_input_contract();
    void constraints_validation_accepts_manual_and_automatic_union();
    void constraints_validation_rejects_unexpected_automatic_marker_geometry();
    void constraints_validation_prefers_manual_regions_over_automatic_markers();
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
            , std::numeric_limits<double>::infinity()
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

void DomainTests::random_walker_parameters_report_first_invalid_field() {
    const auto expect_parameter_error = [](
        const domain::RandomWalkerParameters& parameters
        , domain::RandomWalkerParameterError expected_error
    ) {
        const auto error = domain::validate(parameters);
        QVERIFY(error.has_value());
        QCOMPARE(
            static_cast<int>(*error)
            , static_cast<int>(expected_error)
        );
    };

    expect_parameter_error(
        domain::RandomWalkerParameters {
            .beta = std::nextafter(domain::kMinimumRandomWalkerBeta, 0.0)
        }
        , domain::RandomWalkerParameterError::InvalidBeta
    );
    expect_parameter_error(
        domain::RandomWalkerParameters {
            .distance_power = std::nextafter(
                domain::kMinimumRandomWalkerDistancePower
                , -1.0
            )
        }
        , domain::RandomWalkerParameterError::InvalidDistancePower
    );
    expect_parameter_error(
        domain::RandomWalkerParameters {
            .connectivity = static_cast<domain::PixelConnectivity>(-1)
        }
        , domain::RandomWalkerParameterError::InvalidConnectivity
    );
    QVERIFY(!domain::validate(domain::RandomWalkerParameters {}).has_value());
}

void DomainTests::pixel_connectivity_rejects_unknown_values() {
    QVERIFY(domain::is_valid(domain::PixelConnectivity::Four));
    QVERIFY(domain::is_valid(domain::PixelConnectivity::Eight));
    QVERIFY(!domain::is_valid(static_cast<domain::PixelConnectivity>(-1)));
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
        , .minimum_boundary_distance = domain::kMinimumAutoMarkerBoundaryDistance
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
        , .minimum_boundary_distance = domain::kMaximumAutoMarkerBoundaryDistance
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
        .minimum_boundary_distance = domain::kMinimumAutoMarkerBoundaryDistance - 1
    }));
    QVERIFY(!domain::is_valid(domain::AutoMarkerParameters {
        .minimum_boundary_distance = domain::kMaximumAutoMarkerBoundaryDistance + 1
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

void DomainTests::image_geometry_accepts_representable_pixel_count() {
    QVERIFY(domain::has_representable_pixel_count(0, 0));
    QVERIFY(domain::has_representable_pixel_count(1, 0));
    QVERIFY(domain::has_representable_pixel_count(
        domain::kMaximumSupportedImagePixels
        , 1
    ));
    QVERIFY(domain::is_supported_non_empty_image_geometry(
        domain::kMaximumSupportedImagePixels
        , 1
    ));
    QCOMPARE(
        domain::pixel_count_as_int(domain::kMaximumSupportedImagePixels, 1)
        , domain::kMaximumSupportedImagePixels
    );
}

void DomainTests::image_geometry_rejects_unrepresentable_pixel_count() {
    QVERIFY(!domain::has_representable_pixel_count(-1, 1));
    QVERIFY(!domain::has_representable_pixel_count(1, -1));
    QVERIFY(!domain::is_supported_non_empty_image_geometry(0, 1));
    QVERIFY(!domain::is_supported_non_empty_image_geometry(
        domain::kMaximumSupportedImagePixels
        , 2
    ));
}

void DomainTests::segmentation_limits_accept_supported_pixel_count() {
    QVERIFY(domain::is_supported_segmentation_image_geometry(1024, 1024));
    QVERIFY(domain::is_supported_segmentation_image_geometry(
        domain::kMaximumRandomWalkerSolvablePixels
        , 1
    ));
}

void DomainTests::segmentation_limits_reject_oversized_pixel_count() {
    QVERIFY(!domain::is_supported_segmentation_image_geometry(0, 1));
    QVERIFY(!domain::is_supported_segmentation_image_geometry(1025, 1024));
    QVERIFY(!domain::is_supported_segmentation_image_geometry(
        domain::kMaximumRandomWalkerSolvablePixels
        , 2
    ));
}

void DomainTests::image_geometry_flattens_pixel_coordinates() {
    QCOMPARE(domain::linear_pixel_index(0, 0, 7), 0);
    QCOMPARE(domain::linear_pixel_index(2, 3, 7), 17);
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
    QCOMPARE(domain::valid_seed_pixel_count(regions), std::size_t {12});
}

void DomainTests::segmentation_request_exposes_immutable_input_contract() {
    domain::GrayImageMatrix pixels(1, 2);
    pixels << std::uint8_t {10}, std::uint8_t {20};

    domain::SegmentationConstraints constraints {
        .manual_seed_regions = {
            domain::SeedRegion {
                .area = {.x = 0, .y = 0, .width = 1, .height = 1},
                .label = domain::SeedLabel::Background
            },
            domain::SeedRegion {
                .area = {.x = 1, .y = 0, .width = 1, .height = 1},
                .label = domain::SeedLabel::Object
            }
        }
    };
    constraints.automatic_markers = domain::MarkerLabelMask(2, 1);
    constraints.automatic_markers.set(
        0
        , 1
        , domain::MarkerLabel::Object
    );

    const domain::RandomWalkerParameters parameters {.beta = 0.01};
    const domain::SegmentationRequest request(
        42
        , domain::GrayImage(std::move(pixels))
        , std::move(constraints)
        , parameters
    );

    QCOMPARE(request.request_id(), 42);
    QCOMPARE(request.image().width(), 2);
    QCOMPARE(request.image().height(), 1);
    QCOMPARE(static_cast<int>(request.image().at(0, 1)), 20);
    QCOMPARE(request.manual_seed_regions().size(), std::size_t {2});
    QCOMPARE(
        static_cast<int>(request.manual_seed_regions()[0].label)
        , static_cast<int>(domain::SeedLabel::Background)
    );
    QCOMPARE(
        static_cast<int>(request.manual_seed_regions()[1].label)
        , static_cast<int>(domain::SeedLabel::Object)
    );
    QCOMPARE(request.automatic_markers().width(), 2);
    QCOMPARE(request.automatic_markers().height(), 1);
    QCOMPARE(
        static_cast<int>(request.automatic_markers().at(0, 1))
        , static_cast<int>(domain::MarkerLabel::Object)
    );
    QVERIFY(request.parameters() == parameters);
}

void DomainTests::constraints_validation_accepts_manual_and_automatic_union() {
    domain::GrayImageMatrix pixels(1, 2);
    pixels << std::uint8_t {10}, std::uint8_t {20};

    domain::SegmentationConstraints constraints {
        .manual_seed_regions = {
            domain::SeedRegion {
                .area = {.x = 0, .y = 0, .width = 1, .height = 1},
                .label = domain::SeedLabel::Background
            }
        },
        .automatic_markers = domain::MarkerLabelMask(2, 1)
    };
    constraints.automatic_markers.set(
        0
        , 1
        , domain::MarkerLabel::Object
    );

    QVERIFY(!domain::validate(
        constraints
        , domain::GrayImage(std::move(pixels))
    ).has_value());
}

void DomainTests::
constraints_validation_rejects_unexpected_automatic_marker_geometry() {
    domain::GrayImageMatrix pixels(1, 2);
    pixels << std::uint8_t {10}, std::uint8_t {20};

    const domain::SegmentationConstraints constraints {
        .manual_seed_regions = {
            domain::SeedRegion {
                .area = {.x = 0, .y = 0, .width = 1, .height = 1},
                .label = domain::SeedLabel::Background
            },
            domain::SeedRegion {
                .area = {.x = 1, .y = 0, .width = 1, .height = 1},
                .label = domain::SeedLabel::Object
            }
        },
        .automatic_markers = domain::MarkerLabelMask(3, 1)
    };

    const auto error = domain::validate(
        constraints
        , domain::GrayImage(std::move(pixels))
    );

    QVERIFY(error.has_value());
    QCOMPARE(
        static_cast<int>(*error)
        , static_cast<int>(domain::SegmentationError::SeedOutOfBounds)
    );
}

void DomainTests::
constraints_validation_prefers_manual_regions_over_automatic_markers() {
    domain::GrayImageMatrix pixels(1, 2);
    pixels << std::uint8_t {10}, std::uint8_t {20};

    domain::SegmentationConstraints constraints {
        .manual_seed_regions = {
            domain::SeedRegion {
                .area = {.x = 0, .y = 0, .width = 1, .height = 1},
                .label = domain::SeedLabel::Background
            }
        },
        .automatic_markers = domain::MarkerLabelMask(2, 1)
    };
    constraints.automatic_markers.set(
        0
        , 0
        , domain::MarkerLabel::Object
    );
    constraints.automatic_markers.set(
        0
        , 1
        , domain::MarkerLabel::Object
    );

    QVERIFY(!domain::validate(
        constraints
        , domain::GrayImage(std::move(pixels))
    ).has_value());
}

QTEST_GUILESS_MAIN(DomainTests)
#include "DomainTests.moc"

#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include <QtTest>

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
    void gray_image_reports_empty_default_state();
    void gray_image_reports_dimensions_and_pixel_values();
    void seed_pixel_count_counts_only_requested_label();
    void segmentation_request_exposes_immutable_input_contract();
};

void DomainTests::default_random_walker_parameters_are_valid() {
    const domain::RandomWalkerParameters parameters;

    QVERIFY(domain::is_valid(parameters));
    QCOMPARE(parameters.beta, domain::kDefaultRandomWalkerBeta);
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

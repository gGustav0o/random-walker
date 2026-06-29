#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <stop_token>
#include <utility>
#include <variant>
#include <vector>

#include <QtTest>

#include "model/domain/Cancellation.hpp"
#include "model/domain/GrayImage.hpp"
#include "model/domain/RandomWalkerParameters.hpp"
#include "model/domain/Seed.hpp"
#include "model/domain/Segmentation.hpp"
#include "model/service/SegmentationService.hpp"

namespace domain = random_walker::domain;
namespace service = random_walker::service;

namespace {
    [[nodiscard]] domain::GrayImage make_image(
        int height
        , int width
        , std::initializer_list<std::uint8_t> values
    ) {
        domain::GrayImageMatrix pixels(height, width);
        auto value = values.begin();
        for (int row = 0; row < height; ++row) {
            for (int column = 0; column < width; ++column) {
                pixels(row, column) = *value;
                ++value;
            }
        }
        return domain::GrayImage(std::move(pixels));
    }

    [[nodiscard]] domain::GrayImage empty_image() {
        return domain::GrayImage {};
    }

    [[nodiscard]] domain::SeedRegion seed_region(
        int x
        , int y
        , int width
        , int height
        , domain::SeedLabel label
    ) {
        return domain::SeedRegion {
            .area = {
                .x = x
                , .y = y
                , .width = width
                , .height = height
            }
            , .label = label
        };
    }

    [[nodiscard]] domain::SegmentationRequest request(
        domain::GrayImage image
        , std::vector<domain::SeedRegion> seeds
        , domain::RandomWalkerParameters parameters = {}
    ) {
        return domain::SegmentationRequest(
            7
            , std::move(image)
            , std::move(seeds)
            , parameters
        );
    }

    [[nodiscard]] domain::CancellationToken cancelled_token() {
        std::stop_source source;
        source.request_stop();
        return domain::CancellationToken(source.get_token());
    }

    void expect_validation_error(
        const domain::SegmentationRequest& segmentation_request
        , domain::SegmentationError expected_error
    ) {
        const auto error = service::SegmentationService::validate(
            segmentation_request
        );

        QVERIFY(error.has_value());
        QCOMPARE(static_cast<int>(*error), static_cast<int>(expected_error));
    }
}

class SegmentationServiceTests final : public QObject {
    Q_OBJECT

private slots:
    void rejects_empty_image();
    void rejects_invalid_beta();
    void rejects_missing_background_seeds();
    void rejects_missing_object_seeds();
    void rejects_seed_out_of_bounds();
    void rejects_conflicting_seed_labels();
    void returns_cancelled_when_cancelled_before_start();
    void segments_fully_constrained_image_without_unknown_pixels();
    void segments_three_pixel_line_with_expected_interpolation();
};

void SegmentationServiceTests::rejects_empty_image() {
    const auto segmentation_request = request(
        empty_image()
        , {
            seed_region(0, 0, 1, 1, domain::SeedLabel::Background),
            seed_region(1, 0, 1, 1, domain::SeedLabel::Object)
        }
    );

    expect_validation_error(
        segmentation_request
        , domain::SegmentationError::EmptyImage
    );
}

void SegmentationServiceTests::rejects_invalid_beta() {
    const auto segmentation_request = request(
        make_image(1, 2, {10, 10})
        , {
            seed_region(0, 0, 1, 1, domain::SeedLabel::Background),
            seed_region(1, 0, 1, 1, domain::SeedLabel::Object)
        }
        , domain::RandomWalkerParameters {.beta = 0.0}
    );

    expect_validation_error(
        segmentation_request
        , domain::SegmentationError::InvalidBeta
    );
}

void SegmentationServiceTests::rejects_missing_background_seeds() {
    const auto segmentation_request = request(
        make_image(1, 2, {10, 10})
        , {
            seed_region(1, 0, 1, 1, domain::SeedLabel::Object)
        }
    );

    expect_validation_error(
        segmentation_request
        , domain::SegmentationError::MissingBackgroundSeeds
    );
}

void SegmentationServiceTests::rejects_missing_object_seeds() {
    const auto segmentation_request = request(
        make_image(1, 2, {10, 10})
        , {
            seed_region(0, 0, 1, 1, domain::SeedLabel::Background)
        }
    );

    expect_validation_error(
        segmentation_request
        , domain::SegmentationError::MissingObjectSeeds
    );
}

void SegmentationServiceTests::rejects_seed_out_of_bounds() {
    const auto segmentation_request = request(
        make_image(1, 2, {10, 10})
        , {
            seed_region(0, 0, 1, 1, domain::SeedLabel::Background),
            seed_region(2, 0, 1, 1, domain::SeedLabel::Object)
        }
    );

    expect_validation_error(
        segmentation_request
        , domain::SegmentationError::SeedOutOfBounds
    );
}

void SegmentationServiceTests::rejects_conflicting_seed_labels() {
    const auto segmentation_request = request(
        make_image(1, 2, {10, 10})
        , {
            seed_region(0, 0, 1, 1, domain::SeedLabel::Background),
            seed_region(0, 0, 1, 1, domain::SeedLabel::Object)
        }
    );

    expect_validation_error(
        segmentation_request
        , domain::SegmentationError::ConflictingSeedLabels
    );
}

void SegmentationServiceTests::returns_cancelled_when_cancelled_before_start() {
    const auto segmentation_request = request(
        make_image(1, 2, {10, 10})
        , {
            seed_region(0, 0, 1, 1, domain::SeedLabel::Background),
            seed_region(1, 0, 1, 1, domain::SeedLabel::Object)
        }
    );

    const service::SegmentationService segmentation_service;
    const domain::SegmentationOutcome outcome = segmentation_service.segment(
        segmentation_request
        , cancelled_token()
        , domain::ProgressReporter {}
    );

    QVERIFY(std::holds_alternative<domain::Cancelled>(outcome));
}

void SegmentationServiceTests::segments_fully_constrained_image_without_unknown_pixels() {
    const auto segmentation_request = request(
        make_image(2, 2, {10, 10, 10, 10})
        , {
            seed_region(0, 0, 1, 2, domain::SeedLabel::Background),
            seed_region(1, 0, 1, 2, domain::SeedLabel::Object)
        }
    );

    const service::SegmentationService segmentation_service;
    const domain::SegmentationOutcome outcome = segmentation_service.segment(
        segmentation_request
        , domain::CancellationToken {}
        , domain::ProgressReporter {}
    );

    QVERIFY(std::holds_alternative<domain::SegmentationResult>(outcome));
    const auto& result = std::get<domain::SegmentationResult>(outcome);
    QCOMPARE(static_cast<int>(result.probabilities.rows()), 2);
    QCOMPARE(static_cast<int>(result.probabilities.cols()), 2);
    QCOMPARE(result.probabilities(0, 0), 0.0);
    QCOMPARE(result.probabilities(1, 0), 0.0);
    QCOMPARE(result.probabilities(0, 1), 1.0);
    QCOMPARE(result.probabilities(1, 1), 1.0);
    QCOMPARE(static_cast<int>(result.mask(0, 0)), 0);
    QCOMPARE(static_cast<int>(result.mask(1, 0)), 0);
    QCOMPARE(static_cast<int>(result.mask(1, 1)), 1);
}

void SegmentationServiceTests::segments_three_pixel_line_with_expected_interpolation() {
    const auto segmentation_request = request(
        make_image(1, 3, {10, 10, 10})
        , {
            seed_region(0, 0, 1, 1, domain::SeedLabel::Background),
            seed_region(2, 0, 1, 1, domain::SeedLabel::Object)
        }
    );

    const service::SegmentationService segmentation_service;
    const domain::SegmentationOutcome outcome = segmentation_service.segment(
        segmentation_request
        , domain::CancellationToken {}
        , domain::ProgressReporter {}
    );

    QVERIFY(std::holds_alternative<domain::SegmentationResult>(outcome));
    const auto& result = std::get<domain::SegmentationResult>(outcome);
    QCOMPARE(static_cast<int>(result.probabilities.rows()), 1);
    QCOMPARE(static_cast<int>(result.probabilities.cols()), 3);
    QCOMPARE(result.probabilities(0, 0), 0.0);
    QVERIFY(std::abs(result.probabilities(0, 1) - 0.5) < 1e-12);
    QCOMPARE(result.probabilities(0, 2), 1.0);
    QCOMPARE(static_cast<int>(result.mask(0, 0)), 0);
    QCOMPARE(static_cast<int>(result.mask(0, 2)), 1);
}

QTEST_GUILESS_MAIN(SegmentationServiceTests)
#include "SegmentationServiceTests.moc"

#include <cstddef>
#include <utility>
#include <vector>

#include <QtTest>

#include "application/segmentation/SegmentationUseCase.hpp"

namespace application = random_walker::application;
namespace domain = random_walker::domain;

namespace {
    [[nodiscard]] domain::GrayImage make_image() {
        domain::GrayImageMatrix pixels(1, 1);
        pixels(0, 0) = 42;
        return domain::GrayImage(std::move(pixels));
    }
}

class SegmentationUseCaseTests final : public QObject {
    Q_OBJECT

private slots:
    void builds_request_from_application_settings();
};

void SegmentationUseCaseTests::builds_request_from_application_settings() {
    application::ApplicationSettings settings;
    settings.random_walker.connectivity = domain::PixelConnectivity::Eight;
    domain::SegmentationConstraints constraints {
        .manual_seed_regions = {
            {
                .area = {.x = 0, .y = 0, .width = 1, .height = 1},
                .label = domain::SeedLabel::Background
            }
        }
    };

    const domain::SegmentationRequest request =
        application::make_segmentation_request(
            static_cast<domain::SegmentationRequestId>(7)
            , make_image()
            , std::move(constraints)
            , settings
        );

    QCOMPARE(static_cast<qulonglong>(request.request_id()), qulonglong {7});
    QCOMPARE(request.image().width(), 1);
    QCOMPARE(request.image().height(), 1);
    QCOMPARE(static_cast<int>(request.manual_seed_regions().size()), 1);
    QCOMPARE(
        static_cast<int>(request.parameters().connectivity)
        , static_cast<int>(domain::PixelConnectivity::Eight)
    );
}

QTEST_GUILESS_MAIN(SegmentationUseCaseTests)

#include "SegmentationUseCaseTests.moc"

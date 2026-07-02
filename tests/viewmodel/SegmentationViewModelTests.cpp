#include <optional>
#include <utility>
#include <variant>

#include <QCoreApplication>
#include <QEvent>
#include <QEventLoop>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "application/settings/SettingsRepository.hpp"
#include "application/settings/SettingsService.hpp"
#include "model/domain/Segmentation.hpp"
#include "model/executor/SegmentationExecutor.hpp"
#include "presentation/image/PresentationImageCache.hpp"
#include "viewmodel/SegmentationViewModel.hpp"

namespace application = random_walker::application;
namespace domain = random_walker::domain;
namespace executor = random_walker::executor;

namespace {
    class InMemorySettingsRepository final : public application::SettingsRepository {
    public:
        [[nodiscard]] application::SettingsRepositoryLoadResult load()
            const override {
            return {
                .settings = stored
                , .repair_required = repair_required
            };
        }

        [[nodiscard]] bool save(
            const application::ApplicationSettings& settings
        ) override {
            stored = settings;
            ++save_count;
            return save_succeeds;
        }

        mutable application::ApplicationSettings stored;
        bool repair_required = false;
        bool save_succeeds = true;
        int save_count = 0;
    };

    class FakeImageCache final : public PresentationImageCache {
    public:
        void store(const QImage& image) override {
            last_image = image;
            ++store_count;
        }

        void clear() override {
            last_image = {};
            ++clear_count;
        }

        QImage last_image;
        int store_count = 0;
        int clear_count = 0;
    };

    class FakeSegmentationExecutor final : public executor::SegmentationExecutor {
    public:
        void submit(
            domain::SegmentationRequest request
            , executor::SegmentationProgressHandler progress_handler
            , executor::SegmentationCompletionHandler completion_handler
        ) override {
            last_request_id = request.request_id();
            last_image_width = request.image().width();
            last_image_height = request.image().height();
            last_background_seed_pixels = domain::seed_pixel_count(
                request.seed_regions()
                , domain::SeedLabel::Background
            );
            last_object_seed_pixels = domain::seed_pixel_count(
                request.seed_regions()
                , domain::SeedLabel::Object
            );
            last_beta = request.parameters().beta;
            last_connectivity = request.parameters().connectivity;
            progress_handler_ = std::move(progress_handler);
            completion_handler_ = std::move(completion_handler);
            ++submit_count;
        }

        void cancel() noexcept override {
            ++cancel_count;
        }

        void deliver_progress(domain::SegmentationProgress progress) const {
            progress_handler_(std::move(progress));
        }

        void deliver_completion(
            executor::SegmentationCompletion completion
        ) const {
            completion_handler_(std::move(completion));
        }

        int submit_count = 0;
        int cancel_count = 0;
        std::optional<domain::SegmentationRequestId> last_request_id;
        int last_image_width = 0;
        int last_image_height = 0;
        int last_background_seed_pixels = 0;
        int last_object_seed_pixels = 0;
        double last_beta = 0.0;
        domain::PixelConnectivity last_connectivity =
            domain::PixelConnectivity::Four;

    private:
        executor::SegmentationProgressHandler progress_handler_;
        executor::SegmentationCompletionHandler completion_handler_;
    };

    struct ViewModelFixture {
        InMemorySettingsRepository repository;
        application::SettingsService settings_service {repository};
        FakeSegmentationExecutor executor;
        FakeImageCache base_cache;
        FakeImageCache result_cache;
        SegmentationViewModel view_model {
            executor
            , settings_service
            , base_cache
            , result_cache
        };
    };

    [[nodiscard]] QString write_test_image(QTemporaryDir& directory) {
        const QString path = directory.filePath(QStringLiteral("input.bmp"));
        QImage image(3, 2, QImage::Format_Grayscale8);
        image.fill(128);
        const bool saved = image.save(path);
        Q_ASSERT(saved);
        return path;
    }

    [[nodiscard]] domain::SegmentationResult make_result(int width, int height) {
        domain::ProbabilityMap probabilities(height, width);
        probabilities.setConstant(0.75);
        domain::BinaryMask mask(height, width);
        mask.setOnes();
        return {
            .probabilities = std::move(probabilities)
            , .mask = std::move(mask)
        };
    }

    void process_queued_viewmodel_events() {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QCoreApplication::processEvents(QEventLoop::AllEvents);
    }
}

class SegmentationViewModelTests final : public QObject {
    Q_OBJECT

private slots:
    void initializes_from_settings();
    void set_beta_saves_settings_and_emits_change();
    void set_connectivity_saves_settings_and_emits_change();
    void open_image_updates_image_state_and_cache();
    void add_seed_rectangles_updates_counts_and_can_run();
    void run_segmentation_submits_request_and_updates_progress();
    void completion_with_result_updates_result_state();
    void stale_completion_is_ignored();
};

void SegmentationViewModelTests::initializes_from_settings() {
    ViewModelFixture fixture;

    QCOMPARE(fixture.view_model.beta(), domain::kDefaultRandomWalkerBeta);
    QCOMPARE(
        fixture.view_model.connectivity()
        , static_cast<int>(SegmentationViewModel::FourConnectivity)
    );
    QVERIFY(!fixture.view_model.image_loaded());
    QVERIFY(!fixture.view_model.can_run());
    QVERIFY(!fixture.view_model.busy());
    QVERIFY(!fixture.view_model.has_result());
    QCOMPARE(fixture.view_model.background_seed_count(), 0);
    QCOMPARE(fixture.view_model.object_seed_count(), 0);
}

void SegmentationViewModelTests::set_beta_saves_settings_and_emits_change() {
    ViewModelFixture fixture;
    QSignalSpy beta_changed(&fixture.view_model, &SegmentationViewModel::beta_changed);

    fixture.view_model.set_beta(0.01);

    QCOMPARE(fixture.view_model.beta(), 0.01);
    QCOMPARE(fixture.repository.save_count, 1);
    QCOMPARE(fixture.repository.stored.random_walker.beta, 0.01);
    QCOMPARE(beta_changed.count(), 1);
}

void SegmentationViewModelTests::set_connectivity_saves_settings_and_emits_change() {
    ViewModelFixture fixture;
    QSignalSpy connectivity_changed(
        &fixture.view_model
        , &SegmentationViewModel::connectivity_changed
    );

    fixture.view_model.set_connectivity(SegmentationViewModel::EightConnectivity);

    QCOMPARE(
        fixture.view_model.connectivity()
        , static_cast<int>(SegmentationViewModel::EightConnectivity)
    );
    QCOMPARE(fixture.repository.save_count, 1);
    QCOMPARE(
        static_cast<int>(fixture.repository.stored.random_walker.connectivity)
        , static_cast<int>(domain::PixelConnectivity::Eight)
    );
    QCOMPARE(connectivity_changed.count(), 1);
}

void SegmentationViewModelTests::open_image_updates_image_state_and_cache() {
    ViewModelFixture fixture;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    fixture.view_model.open_image(write_test_image(directory));

    QVERIFY(fixture.view_model.image_loaded());
    QCOMPARE(fixture.view_model.image_width(), 3);
    QCOMPARE(fixture.view_model.image_height(), 2);
    QCOMPARE(fixture.base_cache.store_count, 1);
    QVERIFY(!fixture.base_cache.last_image.isNull());
    QVERIFY(fixture.view_model.image_source().startsWith(QStringLiteral("image://")));
    QVERIFY(fixture.view_model.error_message().isEmpty());
}

void SegmentationViewModelTests::add_seed_rectangles_updates_counts_and_can_run() {
    ViewModelFixture fixture;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    fixture.view_model.open_image(write_test_image(directory));
    QSignalSpy can_run_changed(
        &fixture.view_model
        , &SegmentationViewModel::can_run_changed
    );

    fixture.view_model.add_seed_rectangle(0, 0, 1, 2);
    fixture.view_model.set_selected_label(SegmentationViewModel::Object);
    fixture.view_model.add_seed_rectangle(2, 0, 1, 2);

    QCOMPARE(fixture.view_model.background_seed_count(), 2);
    QCOMPARE(fixture.view_model.object_seed_count(), 2);
    QVERIFY(fixture.view_model.can_run());
    QCOMPARE(can_run_changed.count(), 1);
}

void SegmentationViewModelTests::run_segmentation_submits_request_and_updates_progress() {
    ViewModelFixture fixture;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    fixture.view_model.open_image(write_test_image(directory));
    fixture.view_model.add_seed_rectangle(0, 0, 1, 1);
    fixture.view_model.set_selected_label(SegmentationViewModel::Object);
    fixture.view_model.add_seed_rectangle(2, 1, 1, 1);
    fixture.view_model.set_connectivity(SegmentationViewModel::EightConnectivity);
    QSignalSpy busy_changed(&fixture.view_model, &SegmentationViewModel::busy_changed);
    QSignalSpy progress_changed(
        &fixture.view_model
        , &SegmentationViewModel::progress_changed
    );

    fixture.view_model.run_segmentation();

    QCOMPARE(fixture.executor.submit_count, 1);
    QVERIFY(fixture.executor.last_request_id.has_value());
    QCOMPARE(fixture.executor.last_image_width, 3);
    QCOMPARE(fixture.executor.last_image_height, 2);
    QCOMPARE(fixture.executor.last_background_seed_pixels, 1);
    QCOMPARE(fixture.executor.last_object_seed_pixels, 1);
    QCOMPARE(fixture.executor.last_beta, domain::kDefaultRandomWalkerBeta);
    QCOMPARE(
        static_cast<int>(fixture.executor.last_connectivity)
        , static_cast<int>(domain::PixelConnectivity::Eight)
    );
    QVERIFY(fixture.view_model.busy());
    QCOMPARE(busy_changed.count(), 1);

    fixture.executor.deliver_progress({
        .request_id = *fixture.executor.last_request_id
        , .stage = domain::SegmentationStage::BuildingGraph
        , .fraction = 0.25
    });
    process_queued_viewmodel_events();

    QVERIFY(progress_changed.count() > 0);
    QCOMPARE(
        fixture.view_model.progress_stage()
        , static_cast<int>(SegmentationViewModel::BuildingGraph)
    );
    QCOMPARE(fixture.view_model.progress_fraction(), 0.25);
}

void SegmentationViewModelTests::completion_with_result_updates_result_state() {
    ViewModelFixture fixture;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    fixture.view_model.open_image(write_test_image(directory));
    fixture.view_model.add_seed_rectangle(0, 0, 1, 1);
    fixture.view_model.set_selected_label(SegmentationViewModel::Object);
    fixture.view_model.add_seed_rectangle(2, 1, 1, 1);
    fixture.view_model.run_segmentation();
    QVERIFY(fixture.executor.last_request_id.has_value());
    QSignalSpy result_changed(
        &fixture.view_model
        , &SegmentationViewModel::result_changed
    );
    QSignalSpy busy_changed(&fixture.view_model, &SegmentationViewModel::busy_changed);

    fixture.executor.deliver_completion({
        .request_id = *fixture.executor.last_request_id
        , .outcome = domain::SegmentationOutcome {make_result(3, 2)}
    });
    process_queued_viewmodel_events();

    QVERIFY(!fixture.view_model.busy());
    QVERIFY(fixture.view_model.has_result());
    QVERIFY(fixture.view_model.result_source().startsWith(QStringLiteral("image://")));
    QCOMPARE(fixture.result_cache.store_count, 1);
    QVERIFY(!fixture.result_cache.last_image.isNull());
    QCOMPARE(result_changed.count(), 1);
    QCOMPARE(busy_changed.count(), 1);
}

void SegmentationViewModelTests::stale_completion_is_ignored() {
    ViewModelFixture fixture;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    fixture.view_model.open_image(write_test_image(directory));
    fixture.view_model.add_seed_rectangle(0, 0, 1, 1);
    fixture.view_model.set_selected_label(SegmentationViewModel::Object);
    fixture.view_model.add_seed_rectangle(2, 1, 1, 1);
    fixture.view_model.run_segmentation();
    QVERIFY(fixture.executor.last_request_id.has_value());

    fixture.executor.deliver_completion({
        .request_id = *fixture.executor.last_request_id + 1
        , .outcome = domain::SegmentationOutcome {make_result(3, 2)}
    });
    process_queued_viewmodel_events();

    QVERIFY(fixture.view_model.busy());
    QVERIFY(!fixture.view_model.has_result());
    QCOMPARE(fixture.result_cache.store_count, 0);
}

QTEST_GUILESS_MAIN(SegmentationViewModelTests)
#include "SegmentationViewModelTests.moc"

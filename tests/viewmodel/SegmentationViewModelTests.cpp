#include <optional>
#include <utility>
#include <variant>

#include <QColor>
#include <QCoreApplication>
#include <QEvent>
#include <QEventLoop>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "application/markers/AutoMarkerExecutor.hpp"
#include "application/markers/AutoMarkerService.hpp"
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
            last_distance_power = request.parameters().distance_power;
            last_edge_weight_model = request.parameters().edge_weight_model;
            last_local_contrast = request.parameters().local_contrast_scale;
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
        double last_distance_power = 0.0;
        domain::EdgeWeightModel last_edge_weight_model =
            domain::EdgeWeightModel::GlobalBeta;
        domain::LocalContrastScaleParameters last_local_contrast;

    private:
        executor::SegmentationProgressHandler progress_handler_;
        executor::SegmentationCompletionHandler completion_handler_;
    };

    class FakeAutoMarkerExecutor final : public application::AutoMarkerExecutor {
    public:
        void submit(
            application::AutoMarkerRequest request
            , application::AutoMarkerCompletionHandler completion_handler
        ) override {
            last_request_id = request.request_id();
            last_image_width = request.image().width();
            last_image_height = request.image().height();
            last_manual_seed_region_count =
                static_cast<int>(request.manual_seed_regions().size());
            pending_request.emplace(std::move(request));
            completion_handler_ = std::move(completion_handler);
            ++submit_count;
        }

        void cancel() noexcept override {
            pending_request.reset();
            completion_handler_ = {};
            ++cancel_count;
        }

        void deliver_completion(application::AutoMarkerProposalOutcome outcome) const {
            completion_handler_({
                .request_id = *last_request_id
                , .outcome = std::move(outcome)
            });
        }

        void deliver_service_result() const {
            const application::AutoMarkerService service;
            deliver_completion(service.propose(
                pending_request->image()
                , pending_request->parameters()
                , pending_request->manual_seed_regions()
            ));
        }

        int submit_count = 0;
        int cancel_count = 0;
        int last_image_width = 0;
        int last_image_height = 0;
        int last_manual_seed_region_count = 0;
        std::optional<application::AutoMarkerRequestId> last_request_id;
        std::optional<application::AutoMarkerRequest> pending_request;

    private:
        mutable application::AutoMarkerCompletionHandler completion_handler_;
    };

    struct ViewModelFixture {
        InMemorySettingsRepository repository;
        application::SettingsService settings_service {repository};
        FakeSegmentationExecutor executor;
        FakeAutoMarkerExecutor auto_marker_executor;
        FakeImageCache base_cache;
        FakeImageCache auto_marker_cache;
        FakeImageCache result_cache;
        SegmentationViewModel view_model {
            executor
            , settings_service
            , auto_marker_executor
            , base_cache
            , auto_marker_cache
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

    [[nodiscard]] QString write_bimodal_test_image(QTemporaryDir& directory) {
        const QString path = directory.filePath(QStringLiteral("bimodal.bmp"));
        QImage image(16, 8, QImage::Format_Grayscale8);
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                image.setPixelColor(
                    x
                    , y
                    , x < image.width() / 2 ? QColor(24, 24, 24) : QColor(224, 224, 224)
                );
            }
        }
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
    void set_distance_power_saves_settings_and_emits_change();
    void set_edge_weight_model_saves_settings_and_emits_change();
    void set_local_contrast_saves_settings_and_emits_change();
    void open_image_updates_image_state_and_cache();
    void add_seed_rectangles_updates_counts_and_can_run();
    void propose_markers_without_image_sets_error();
    void propose_markers_does_not_inflate_seed_model();
    void clear_automatic_markers_keeps_manual_regions();
    void clear_seeds_clears_manual_and_automatic_markers();
    void run_segmentation_includes_automatic_marker_constraints();
    void run_segmentation_is_ignored_while_auto_markers_are_running();
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
    QCOMPARE(
        fixture.view_model.distance_power()
        , domain::kDefaultRandomWalkerDistancePower
    );
    QCOMPARE(
        fixture.view_model.edge_weight_model()
        , static_cast<int>(SegmentationViewModel::GlobalBetaWeight)
    );
    QCOMPARE(
        fixture.view_model.local_contrast_radius()
        , domain::kDefaultLocalContrastRadius
    );
    QCOMPARE(
        fixture.view_model.local_contrast_minimum_variance()
        , domain::kDefaultLocalContrastMinimumVariance
    );
    QCOMPARE(
        fixture.view_model.local_contrast_minimum_variance_mode()
        , static_cast<int>(SegmentationViewModel::ManualMinimumVariance)
    );
    QCOMPARE(
        fixture.view_model.local_contrast_auto_quantile()
        , domain::kDefaultLocalContrastAutoQuantile
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

void SegmentationViewModelTests::set_distance_power_saves_settings_and_emits_change() {
    ViewModelFixture fixture;
    QSignalSpy distance_power_changed(
        &fixture.view_model
        , &SegmentationViewModel::distance_power_changed
    );

    fixture.view_model.set_distance_power(1.5);

    QCOMPARE(fixture.view_model.distance_power(), 1.5);
    QCOMPARE(fixture.repository.save_count, 1);
    QCOMPARE(fixture.repository.stored.random_walker.distance_power, 1.5);
    QCOMPARE(distance_power_changed.count(), 1);
}


void SegmentationViewModelTests::set_edge_weight_model_saves_settings_and_emits_change() {
    ViewModelFixture fixture;
    QSignalSpy edge_weight_model_changed(
        &fixture.view_model
        , &SegmentationViewModel::edge_weight_model_changed
    );

    fixture.view_model.set_edge_weight_model(
        SegmentationViewModel::LocalVarianceNormalizedWeight
    );

    QCOMPARE(
        fixture.view_model.edge_weight_model()
        , static_cast<int>(SegmentationViewModel::LocalVarianceNormalizedWeight)
    );
    QCOMPARE(fixture.repository.save_count, 1);
    QCOMPARE(
        static_cast<int>(fixture.repository.stored.random_walker.edge_weight_model)
        , static_cast<int>(domain::EdgeWeightModel::LocalVarianceNormalized)
    );
    QCOMPARE(edge_weight_model_changed.count(), 1);
}

void SegmentationViewModelTests::set_local_contrast_saves_settings_and_emits_change() {
    ViewModelFixture fixture;
    QSignalSpy local_contrast_changed(
        &fixture.view_model
        , &SegmentationViewModel::local_contrast_changed
    );

    fixture.view_model.set_local_contrast_radius(3);
    fixture.view_model.set_local_contrast_minimum_variance(4.5);
    fixture.view_model.set_local_contrast_minimum_variance_mode(
        SegmentationViewModel::AutoMinimumVariance
    );
    fixture.view_model.set_local_contrast_auto_quantile(0.25);

    QCOMPARE(fixture.view_model.local_contrast_radius(), 3);
    QCOMPARE(fixture.view_model.local_contrast_minimum_variance(), 4.5);
    QCOMPARE(
        fixture.view_model.local_contrast_minimum_variance_mode()
        , static_cast<int>(SegmentationViewModel::AutoMinimumVariance)
    );
    QCOMPARE(fixture.view_model.local_contrast_auto_quantile(), 0.25);
    QCOMPARE(fixture.repository.save_count, 4);
    QCOMPARE(
        fixture.repository.stored.random_walker.local_contrast_scale.radius
        , 3
    );
    QCOMPARE(
        fixture.repository.stored.random_walker
            .local_contrast_scale
            .minimum_variance
        , 4.5
    );
    QCOMPARE(
        static_cast<int>(
            fixture.repository.stored.random_walker
                .local_contrast_scale
                .minimum_variance_mode
        )
        , static_cast<int>(domain::MinimumVarianceMode::Auto)
    );
    QCOMPARE(
        fixture.repository.stored.random_walker
            .local_contrast_scale
            .auto_minimum_variance_quantile
        , 0.25
    );    QCOMPARE(local_contrast_changed.count(), 4);
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

void SegmentationViewModelTests::propose_markers_without_image_sets_error() {
    ViewModelFixture fixture;
    QSignalSpy error_message_changed(
        &fixture.view_model
        , &SegmentationViewModel::error_message_changed
    );

    fixture.view_model.propose_markers();

    QVERIFY(!fixture.view_model.error_message().isEmpty());
    QCOMPARE(fixture.view_model.automatic_marker_count(), 0);
    QCOMPARE(fixture.executor.submit_count, 0);
    QCOMPARE(error_message_changed.count(), 1);
}

void SegmentationViewModelTests::propose_markers_does_not_inflate_seed_model() {
    ViewModelFixture fixture;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    fixture.view_model.open_image(write_bimodal_test_image(directory));
    QSignalSpy automatic_markers_changed(
        &fixture.view_model
        , &SegmentationViewModel::automatic_markers_changed
    );
    QSignalSpy seeds_changed(
        &fixture.view_model
        , &SegmentationViewModel::seeds_changed
    );
    QSignalSpy can_run_changed(
        &fixture.view_model
        , &SegmentationViewModel::can_run_changed
    );
    QSignalSpy busy_changed(
        &fixture.view_model
        , &SegmentationViewModel::busy_changed
    );

    fixture.view_model.propose_markers();

    QCOMPARE(fixture.auto_marker_executor.submit_count, 1);
    QCOMPARE(fixture.auto_marker_executor.last_image_width, 16);
    QCOMPARE(fixture.auto_marker_executor.last_image_height, 8);
    QCOMPARE(fixture.auto_marker_executor.last_manual_seed_region_count, 0);
    QVERIFY(fixture.view_model.busy());
    QCOMPARE(fixture.view_model.automatic_marker_count(), 0);
    QCOMPARE(fixture.view_model.seed_model()->rowCount(), 0);

    fixture.auto_marker_executor.deliver_service_result();
    process_queued_viewmodel_events();

    QVERIFY(fixture.view_model.automatic_marker_count() > 0);
    QVERIFY(fixture.view_model.has_automatic_markers());
    QVERIFY(
        fixture.view_model.automatic_marker_source()
            .startsWith(QStringLiteral("image://"))
    );
    QCOMPARE(fixture.auto_marker_cache.store_count, 1);
    QVERIFY(!fixture.auto_marker_cache.last_image.isNull());
    QCOMPARE(fixture.view_model.background_seed_count(), 0);
    QCOMPARE(fixture.view_model.object_seed_count(), 0);
    QVERIFY(fixture.view_model.can_run());
    QVERIFY(!fixture.view_model.busy());
    QCOMPARE(fixture.view_model.seed_model()->rowCount(), 0);
    QCOMPARE(fixture.executor.submit_count, 0);
    QCOMPARE(automatic_markers_changed.count(), 1);
    QCOMPARE(seeds_changed.count(), 1);
    QCOMPARE(can_run_changed.count(), 1);
    QCOMPARE(busy_changed.count(), 2);
    QVERIFY(fixture.view_model.error_message().isEmpty());
}

void SegmentationViewModelTests::clear_automatic_markers_keeps_manual_regions() {
    ViewModelFixture fixture;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    fixture.view_model.open_image(write_bimodal_test_image(directory));
    fixture.view_model.add_seed_rectangle(0, 0, 1, 1);
    fixture.view_model.set_selected_label(SegmentationViewModel::Object);
    fixture.view_model.add_seed_rectangle(15, 7, 1, 1);
    fixture.view_model.propose_markers();
    fixture.auto_marker_executor.deliver_service_result();
    process_queued_viewmodel_events();
    QVERIFY(fixture.view_model.automatic_marker_count() > 0);
    const int background_before_clear = fixture.view_model.background_seed_count();
    const int object_before_clear = fixture.view_model.object_seed_count();
    QSignalSpy automatic_markers_changed(
        &fixture.view_model
        , &SegmentationViewModel::automatic_markers_changed
    );
    QSignalSpy seeds_changed(
        &fixture.view_model
        , &SegmentationViewModel::seeds_changed
    );

    fixture.view_model.clear_automatic_markers();

    QCOMPARE(fixture.view_model.automatic_marker_count(), 0);
    QVERIFY(!fixture.view_model.has_automatic_markers());
    QCOMPARE(fixture.auto_marker_cache.clear_count, 1);
    QCOMPARE(fixture.view_model.background_seed_count(), 1);
    QCOMPARE(fixture.view_model.object_seed_count(), 1);
    QCOMPARE(background_before_clear, fixture.view_model.background_seed_count());
    QCOMPARE(object_before_clear, fixture.view_model.object_seed_count());
    QCOMPARE(automatic_markers_changed.count(), 1);
    QCOMPARE(seeds_changed.count(), 1);
}

void SegmentationViewModelTests::clear_seeds_clears_manual_and_automatic_markers() {
    ViewModelFixture fixture;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    fixture.view_model.open_image(write_bimodal_test_image(directory));
    fixture.view_model.add_seed_rectangle(0, 0, 1, 1);
    fixture.view_model.set_selected_label(SegmentationViewModel::Object);
    fixture.view_model.add_seed_rectangle(15, 7, 1, 1);
    fixture.view_model.propose_markers();
    fixture.auto_marker_executor.deliver_service_result();
    process_queued_viewmodel_events();
    QVERIFY(fixture.view_model.automatic_marker_count() > 0);
    QVERIFY(fixture.view_model.has_automatic_markers());
    QCOMPARE(fixture.view_model.background_seed_count(), 1);
    QCOMPARE(fixture.view_model.object_seed_count(), 1);

    QSignalSpy automatic_markers_changed(
        &fixture.view_model
        , &SegmentationViewModel::automatic_markers_changed
    );
    QSignalSpy seeds_changed(
        &fixture.view_model
        , &SegmentationViewModel::seeds_changed
    );

    fixture.view_model.clear_seeds();

    QCOMPARE(fixture.view_model.background_seed_count(), 0);
    QCOMPARE(fixture.view_model.object_seed_count(), 0);
    QCOMPARE(fixture.view_model.automatic_marker_count(), 0);
    QVERIFY(!fixture.view_model.has_automatic_markers());
    QCOMPARE(fixture.view_model.seed_model()->rowCount(), 0);
    QCOMPARE(fixture.auto_marker_cache.clear_count, 1);
    QCOMPARE(automatic_markers_changed.count(), 1);
    QCOMPARE(seeds_changed.count(), 1);
    QVERIFY(!fixture.view_model.can_run());
}

void SegmentationViewModelTests::run_segmentation_includes_automatic_marker_constraints() {
    ViewModelFixture fixture;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    fixture.view_model.open_image(write_bimodal_test_image(directory));
    fixture.view_model.propose_markers();
    fixture.auto_marker_executor.deliver_service_result();
    process_queued_viewmodel_events();
    const int automatic_marker_count_before_run =
        fixture.view_model.automatic_marker_count();
    QVERIFY(automatic_marker_count_before_run > 0);
    QSignalSpy automatic_markers_changed(
        &fixture.view_model
        , &SegmentationViewModel::automatic_markers_changed
    );
    QSignalSpy seeds_changed(
        &fixture.view_model
        , &SegmentationViewModel::seeds_changed
    );
    QSignalSpy can_run_changed(
        &fixture.view_model
        , &SegmentationViewModel::can_run_changed
    );

    fixture.view_model.run_segmentation();

    QCOMPARE(fixture.executor.submit_count, 1);
    QVERIFY(fixture.executor.last_background_seed_pixels > 0);
    QVERIFY(fixture.executor.last_object_seed_pixels > 0);
    QCOMPARE(
        fixture.executor.last_background_seed_pixels
        + fixture.executor.last_object_seed_pixels
        , automatic_marker_count_before_run
    );
    QCOMPARE(fixture.view_model.automatic_marker_count(), 0);
    QVERIFY(!fixture.view_model.has_automatic_markers());
    QCOMPARE(fixture.auto_marker_cache.clear_count, 1);
    QCOMPARE(fixture.view_model.seed_model()->rowCount(), 0);
    QCOMPARE(automatic_markers_changed.count(), 1);
    QCOMPARE(seeds_changed.count(), 1);
    QCOMPARE(can_run_changed.count(), 1);
    QVERIFY(!fixture.view_model.can_run());
}

void SegmentationViewModelTests::run_segmentation_is_ignored_while_auto_markers_are_running() {
    ViewModelFixture fixture;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    fixture.view_model.open_image(write_test_image(directory));
    fixture.view_model.add_seed_rectangle(0, 0, 1, 1);
    fixture.view_model.set_selected_label(SegmentationViewModel::Object);
    fixture.view_model.add_seed_rectangle(2, 1, 1, 1);
    QVERIFY(fixture.view_model.can_run());

    fixture.view_model.propose_markers();
    fixture.view_model.run_segmentation();

    QCOMPARE(fixture.auto_marker_executor.submit_count, 1);
    QCOMPARE(fixture.auto_marker_executor.last_manual_seed_region_count, 2);
    QVERIFY(fixture.view_model.busy());
    QCOMPARE(fixture.executor.submit_count, 0);
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
    fixture.view_model.set_distance_power(1.5);
    fixture.view_model.set_edge_weight_model(
        SegmentationViewModel::LocalVarianceNormalizedWeight
    );
    fixture.view_model.set_local_contrast_radius(3);
    fixture.view_model.set_local_contrast_minimum_variance(4.5);
    fixture.view_model.set_local_contrast_minimum_variance_mode(
        SegmentationViewModel::AutoMinimumVariance
    );
    fixture.view_model.set_local_contrast_auto_quantile(0.25);
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
    QCOMPARE(fixture.executor.last_distance_power, 1.5);
    QCOMPARE(
        static_cast<int>(fixture.executor.last_edge_weight_model)
        , static_cast<int>(domain::EdgeWeightModel::LocalVarianceNormalized)
    );
    QCOMPARE(fixture.executor.last_local_contrast.radius, 3);
    QCOMPARE(fixture.executor.last_local_contrast.minimum_variance, 4.5);
    QCOMPARE(
        static_cast<int>(
            fixture.executor.last_local_contrast.minimum_variance_mode
        )
        , static_cast<int>(domain::MinimumVarianceMode::Auto)
    );
    QCOMPARE(
        fixture.executor.last_local_contrast.auto_minimum_variance_quantile
        , 0.25
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

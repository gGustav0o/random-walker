#include "SegmentationViewModel.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <utility>
#include <variant>

#include <QImage>
#include <QMetaObject>
#include <QThread>
#include <QUrl>

#include "presentation/adapter/MaskRenderer.hpp"
#include "presentation/adapter/QImageAdapter.hpp"
#include "presentation/qml/qml_names.hpp"

namespace {
    const double kMinimumBetaExponent =
        std::log10(random_walker::domain::kMinimumRandomWalkerBeta);
    const double kMaximumBetaExponent =
        std::log10(random_walker::domain::kMaximumRandomWalkerBeta);

    [[nodiscard]] QString error_message(
        random_walker::domain::SegmentationError error) {
        using Error = random_walker::domain::SegmentationError;

        switch (error) {
        case Error::EmptyImage:
            return QStringLiteral("No image is loaded.");
        case Error::InvalidBeta:
            return QStringLiteral(
                "Beta is outside the supported range.");
        case Error::MissingBackgroundSeeds:
            return QStringLiteral("At least one background seed is required.");
        case Error::MissingObjectSeeds:
            return QStringLiteral("At least one object seed is required.");
        case Error::SeedOutOfBounds:
            return QStringLiteral("A seed lies outside the image.");
        case Error::ConflictingSeedLabels:
            return QStringLiteral(
                "Background and object seeds must not overlap.");
        case Error::LaplacianDecompositionFailed:
            return QStringLiteral("Failed to decompose the graph Laplacian.");
        case Error::LinearSystemSolveFailed:
            return QStringLiteral("Failed to solve the Random Walker system.");
        case Error::NonFiniteSolution:
            return QStringLiteral("The Random Walker solution is not finite.");
        }

        return QStringLiteral("Unknown segmentation error.");
    }

    [[nodiscard]] int progress_stage(
        random_walker::domain::SegmentationStage stage) {
        using DomainStage = random_walker::domain::SegmentationStage;

        switch (stage) {
        case DomainStage::ValidatingInput:
            return SegmentationViewModel::ValidatingInput;
        case DomainStage::ExpandingSeeds:
            return SegmentationViewModel::ExpandingSeeds;
        case DomainStage::BuildingGraph:
            return SegmentationViewModel::BuildingGraph;
        case DomainStage::BuildingLabels:
            return SegmentationViewModel::BuildingLabels;
        case DomainStage::PartitioningSystem:
            return SegmentationViewModel::PartitioningSystem;
        case DomainStage::Factorizing:
            return SegmentationViewModel::Factorizing;
        case DomainStage::Solving:
            return SegmentationViewModel::Solving;
        case DomainStage::AssemblingProbabilities:
            return SegmentationViewModel::AssemblingProbabilities;
        case DomainStage::Thresholding:
            return SegmentationViewModel::Thresholding;
        }

        return SegmentationViewModel::Idle;
    }
}

struct SegmentationViewModel::CompletionDeliveryGate {
    std::mutex mutex;
    SegmentationViewModel* receiver = nullptr;
};

SegmentationViewModel::SegmentationViewModel(
    random_walker::executor::SegmentationExecutor& segmentation_executor
    , random_walker::application::SettingsService& settings_service
    , PresentationImageCache& base_image_cache
    , PresentationImageCache& result_image_cache
    , QObject* parent
)
    : QObject(parent)
    , segmentation_executor_(segmentation_executor)
    , settings_service_(settings_service)
    , base_image_cache_(base_image_cache)
    , result_image_cache_(result_image_cache)
    , seed_model_(seed_regions_)
    , completion_delivery_(std::make_shared<CompletionDeliveryGate>())
    , application_settings_(settings_service_.load()) {
    completion_delivery_->receiver = this;
}

SegmentationViewModel::~SegmentationViewModel() {
    assert_ui_thread();

    {
        std::lock_guard lock(completion_delivery_->mutex);
        completion_delivery_->receiver = nullptr;
    }

    segmentation_executor_.cancel();
}

QString SegmentationViewModel::image_source() const {
    return image_loaded()
        ? QStringLiteral("image://%1/processed").arg(qml_names::kBaseImageProvider)
        : QString();
}

bool SegmentationViewModel::image_loaded() const noexcept {
    return !image_.empty();
}

int SegmentationViewModel::image_width() const noexcept {
    return image_.width();
}

int SegmentationViewModel::image_height() const noexcept {
    return image_.height();
}

quint64 SegmentationViewModel::image_version() const noexcept {
    return image_version_;
}

bool SegmentationViewModel::can_run() const noexcept {
    return image_loaded()
        && background_seed_count() > 0
        && object_seed_count() > 0;
}

bool SegmentationViewModel::busy() const noexcept {
    return busy_;
}

int SegmentationViewModel::progress_stage() const noexcept {
    return progress_stage_;
}

double SegmentationViewModel::progress_fraction() const noexcept {
    return progress_fraction_;
}

bool SegmentationViewModel::progress_indeterminate() const noexcept {
    return progress_indeterminate_;
}

QString SegmentationViewModel::status_text() const {
    switch (progress_stage_) {
    case Idle:
        return {};
    case ValidatingInput:
        return QStringLiteral("Validating input");
    case ExpandingSeeds:
        return QStringLiteral("Preparing seed regions");
    case BuildingGraph:
        return QStringLiteral("Building pixel graph");
    case BuildingLabels:
        return QStringLiteral("Building label constraints");
    case PartitioningSystem:
        return QStringLiteral("Building linear system");
    case Factorizing:
        return QStringLiteral("Factorizing Laplacian");
    case Solving:
        return QStringLiteral("Solving Random Walker system");
    case AssemblingProbabilities:
        return QStringLiteral("Assembling probability map");
    case Thresholding:
        return QStringLiteral("Building segmentation mask");
    }

    return {};
}

double SegmentationViewModel::beta() const noexcept {
    return application_settings_.random_walker.beta;
}

double SegmentationViewModel::beta_slider_position() const noexcept {
    const double exponent =
        std::log10(application_settings_.random_walker.beta);
    return std::clamp(
        (exponent - kMinimumBetaExponent)
            / (kMaximumBetaExponent - kMinimumBetaExponent)
        , 0.0
        , 1.0
    );
}

bool SegmentationViewModel::has_result() const noexcept {
    return result_.has_value();
}

QString SegmentationViewModel::result_source() const {
    return has_result()
        ? QStringLiteral("image://%1/mask").arg(qml_names::kResultImageProvider)
        : QString();
}

quint64 SegmentationViewModel::result_version() const noexcept {
    return result_version_;
}

QAbstractItemModel* SegmentationViewModel::seed_model() noexcept {
    return &seed_model_;
}

int SegmentationViewModel::selected_label() const noexcept {
    return selected_label_;
}

QString SegmentationViewModel::error_message() const {
    return error_message_;
}

int SegmentationViewModel::background_seed_count() const noexcept {
    return random_walker::domain::seed_pixel_count(
        seed_regions_
        , random_walker::domain::SeedLabel::Background
    );
}

int SegmentationViewModel::object_seed_count() const noexcept {
    return random_walker::domain::seed_pixel_count(
        seed_regions_
        , random_walker::domain::SeedLabel::Object
    );
}

void SegmentationViewModel::set_selected_label(int label) {
    assert_ui_thread();

    if (label != Background && label != Object) {
        return;
    }
    if (selected_label_ == label) {
        return;
    }

    selected_label_ = label;
    emit selected_label_changed();
}

void SegmentationViewModel::set_beta(double value) {
    assert_ui_thread();

    auto updated_parameters = application_settings_.random_walker;
    updated_parameters.beta = value;
    update_random_walker_parameters(updated_parameters);
}

void SegmentationViewModel::set_beta_slider_position(double position) {
    assert_ui_thread();

    if (!std::isfinite(position)) {
        return;
    }

    const double normalized_position =
        std::clamp(position, 0.0, 1.0);
    const double exponent =
        kMinimumBetaExponent
        + normalized_position
            * (kMaximumBetaExponent - kMinimumBetaExponent);
    set_beta(std::pow(10.0, exponent));
}

void SegmentationViewModel::open_image(const QString& path) {
    assert_ui_thread();

    const QUrl url(path);
    const QString local_path = url.isLocalFile() ? url.toLocalFile() : path;
    const QImage loaded(local_path);

    if (loaded.isNull()) {
        set_error(QStringLiteral("Failed to load the selected image."));
        return;
    }

    const bool previous_can_run = can_run();
    const bool was_loaded = image_loaded();

    cancel_active_request();
    image_ = random_walker::qt_adapter::to_gray_image(loaded);
    seed_model_.reset([this] {
        seed_regions_.clear();
    });
    invalidate_result();
    set_error({});

    base_image_cache_.store(
        random_walker::qt_adapter::to_qimage(image_));
    ++image_version_;

    emit image_version_changed();
    emit image_source_changed();
    emit image_dimensions_changed();
    emit seeds_changed();
    if (!was_loaded) {
        emit image_loaded_changed(true);
    }
    notify_can_run_if_changed(previous_can_run);
}

void SegmentationViewModel::clear() {
    assert_ui_thread();

    if (!image_loaded() && seed_regions_.empty() && !has_result()
        && error_message_.isEmpty()) {
        return;
    }

    const bool previous_can_run = can_run();
    const bool was_loaded = image_loaded();

    cancel_active_request();
    image_ = {};
    seed_model_.reset([this] {
        seed_regions_.clear();
    });
    invalidate_result();
    set_error({});
    base_image_cache_.clear();
    ++image_version_;

    emit image_version_changed();
    emit image_source_changed();
    emit image_dimensions_changed();
    emit seeds_changed();
    if (was_loaded) {
        emit image_loaded_changed(false);
    }
    notify_can_run_if_changed(previous_can_run);
}

void SegmentationViewModel::clear_seeds() {
    assert_ui_thread();

    if (seed_regions_.empty()) {
        return;
    }

    const bool previous_can_run = can_run();
    cancel_active_request();
    seed_model_.reset([this] {
        seed_regions_.clear();
    });
    invalidate_result();
    set_error({});

    emit seeds_changed();
    notify_can_run_if_changed(previous_can_run);
}

void SegmentationViewModel::add_seed_rectangle(
    int x
    , int y
    , int width
    , int height
) {
    assert_ui_thread();

    if (!image_loaded() || width <= 0 || height <= 0) {
        return;
    }

    const int left = std::clamp(x, 0, image_.width());
    const int top = std::clamp(y, 0, image_.height());
    const auto right_edge = static_cast<long long>(x) + width;
    const auto bottom_edge = static_cast<long long>(y) + height;
    const int right = static_cast<int>(std::clamp(
        right_edge
        , 0LL
        , static_cast<long long>(image_.width()))
    );
    const int bottom = static_cast<int>(std::clamp(
        bottom_edge
        , 0LL
        , static_cast<long long>(image_.height()))
    );

    if (left >= right || top >= bottom) {
        return;
    }

    const bool previous_can_run = can_run();
    const DomainSeedLabel label = domain_seed_label();

    cancel_active_request();
    seed_model_.reset([this, left, top, right, bottom, label] {
        seed_regions_.push_back({
            .area = {
                .x = left
                , .y = top
                , .width = right - left
                , .height = bottom - top
            }
            , .label = label
        });
    });

    invalidate_result();
    set_error({});
    emit seeds_changed();
    notify_can_run_if_changed(previous_can_run);
}

void SegmentationViewModel::run_segmentation() {
    assert_ui_thread();

    if (!can_run()) {
        return;
    }

    const random_walker::domain::SegmentationRequestId request_id =
        next_request_id_++;
    random_walker::domain::SegmentationRequest request(
        request_id
        , image_
        , seed_regions_
        , application_settings_.random_walker
    );

    active_request_id_ = request_id;
    reset_progress();
    set_busy(true);
    set_error({});

    const std::shared_ptr<CompletionDeliveryGate> delivery_gate =
        completion_delivery_;
    segmentation_executor_.submit(
        std::move(request)
        , [delivery_gate](random_walker::domain::SegmentationProgress progress
    ) {
            SegmentationViewModel::dispatch_progress(
                delivery_gate
                , std::move(progress)
            );
        }
        , [delivery_gate](
            random_walker::executor::SegmentationCompletion completion) {
            SegmentationViewModel::dispatch_completion(
                delivery_gate
                , std::move(completion)
            );
        });
}

SegmentationViewModel::DomainSeedLabel
SegmentationViewModel::domain_seed_label() const noexcept {
    return selected_label_ == Object
        ? DomainSeedLabel::Object
        : DomainSeedLabel::Background;
}

void SegmentationViewModel::update_random_walker_parameters(
    random_walker::domain::RandomWalkerParameters parameters) {
    assert_ui_thread();

    if (parameters == application_settings_.random_walker) {
        return;
    }

    auto updated_settings = application_settings_;
    updated_settings.random_walker = parameters;
    if (!settings_service_.try_save(updated_settings)) {
        return;
    }

    cancel_active_request();
    invalidate_result();
    set_error({});

    application_settings_ = std::move(updated_settings);
    emit beta_changed();
}

void SegmentationViewModel::dispatch_completion(
    const std::shared_ptr<CompletionDeliveryGate>& delivery_gate
    , random_walker::executor::SegmentationCompletion completion
) {
    auto payload =
        std::make_shared<random_walker::executor::SegmentationCompletion>(
            std::move(completion));

    std::lock_guard lock(delivery_gate->mutex);
    SegmentationViewModel* receiver = delivery_gate->receiver;
    if (!receiver) {
        return;
    }

    QMetaObject::invokeMethod(
        receiver
        , [receiver, payload] {
            receiver->handle_completion(std::move(*payload));
        }
        , Qt::QueuedConnection
    );
}

void SegmentationViewModel::dispatch_progress(
    const std::shared_ptr<CompletionDeliveryGate>& delivery_gate
    , random_walker::domain::SegmentationProgress progress
) {
    auto payload =
        std::make_shared<random_walker::domain::SegmentationProgress>(
            std::move(progress));

    std::lock_guard lock(delivery_gate->mutex);
    SegmentationViewModel* receiver = delivery_gate->receiver;
    if (!receiver) {
        return;
    }

    QMetaObject::invokeMethod(
        receiver
        , [receiver, payload] {
            receiver->handle_progress(std::move(*payload));
        }
        , Qt::QueuedConnection
    );
}

void SegmentationViewModel::handle_completion(
    random_walker::executor::SegmentationCompletion completion) {
    assert_ui_thread();

    if (!active_request_id_.has_value()
        || *active_request_id_ != completion.request_id) {
        return;
    }

    active_request_id_.reset();
    set_busy(false);

    if (auto* segmentation_result =
            std::get_if<random_walker::domain::SegmentationResult>(
                &completion.outcome)) {
        result_ = std::move(*segmentation_result);
        result_image_cache_.store(
            random_walker::qt_adapter::render_binary_mask(result_->mask));
        ++result_version_;
        emit result_changed();
    } else if (auto* error =
                   std::get_if<random_walker::domain::SegmentationError>(
                       &completion.outcome)) {
        invalidate_result();
        set_error(::error_message(*error));
    } else {
        invalidate_result();
    }

    reset_progress();
}

void SegmentationViewModel::handle_progress(
    random_walker::domain::SegmentationProgress progress) {
    assert_ui_thread();

    if (!active_request_id_.has_value()
        || *active_request_id_ != progress.request_id) {
        return;
    }

    const int stage = ::progress_stage(progress.stage);
    const bool indeterminate = !progress.fraction.has_value();
    const double fraction = progress.fraction.value_or(0.0);

    if (progress_stage_ == stage
        && progress_indeterminate_ == indeterminate
        && qFuzzyCompare(progress_fraction_, fraction)) {
        return;
    }

    progress_stage_ = stage;
    progress_indeterminate_ = indeterminate;
    progress_fraction_ = fraction;
    emit progress_changed();
}

void SegmentationViewModel::cancel_active_request() {
    assert_ui_thread();

    if (!active_request_id_.has_value()) {
        return;
    }

    segmentation_executor_.cancel();
    active_request_id_.reset();
    set_busy(false);
    reset_progress();
}

void SegmentationViewModel::reset_progress() {
    assert_ui_thread();

    if (progress_stage_ == Idle
        && qFuzzyIsNull(progress_fraction_)
        && !progress_indeterminate_) {
        return;
    }

    progress_stage_ = Idle;
    progress_fraction_ = 0.0;
    progress_indeterminate_ = false;
    emit progress_changed();
}

void SegmentationViewModel::invalidate_result() {
    assert_ui_thread();

    if (!result_.has_value()) {
        return;
    }

    result_.reset();
    result_image_cache_.clear();
    ++result_version_;
    emit result_changed();
}

void SegmentationViewModel::set_busy(bool value) {
    assert_ui_thread();

    if (busy_ == value) {
        return;
    }

    const bool previous_can_run = can_run();
    busy_ = value;
    emit busy_changed();
    notify_can_run_if_changed(previous_can_run);
}

void SegmentationViewModel::set_error(QString message) {
    assert_ui_thread();

    if (error_message_ == message) {
        return;
    }

    error_message_ = std::move(message);
    emit error_message_changed();
}

void SegmentationViewModel::notify_can_run_if_changed(bool previous_value) {
    if (previous_value != can_run()) {
        emit can_run_changed();
    }
}

void SegmentationViewModel::assert_ui_thread() const {
    Q_ASSERT(QThread::currentThread() == thread());
}

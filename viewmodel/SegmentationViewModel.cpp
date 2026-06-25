#include "SegmentationViewModel.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <variant>

#include <QImage>
#include <QMetaObject>
#include <QThread>

#include "infrastructure/logging/Logging.hpp"
#include "presentation/image/ImageLoader.hpp"

namespace {
    const double kMinimumBetaExponent =
        std::log10(random_walker::domain::kMinimumRandomWalkerBeta);
    const double kMaximumBetaExponent =
        std::log10(random_walker::domain::kMaximumRandomWalkerBeta);

    [[nodiscard]] random_walker::application::ApplicationError
    application_error(random_walker::executor::ExecutionError error) noexcept {
        using Error = random_walker::executor::ExecutionError;

        switch (error) {
        case Error::UnexpectedInternalFailure:
            return random_walker::application::ApplicationError::
                UnexpectedInternalFailure;
        }

        return random_walker::application::ApplicationError::
            UnexpectedInternalFailure;
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
    , image_state_(base_image_cache)
    , result_state_(result_image_cache)
    , seed_model_(seed_state_.mutable_regions())
    , completion_delivery_(std::make_shared<CompletionDeliveryGate>()) {
    const auto loaded_settings = settings_service_.load();
    application_settings_ = loaded_settings.settings;
    random_walker::infrastructure::log_info(
        "viewmodel"
        , loaded_settings.repair_required
            ? "Application settings loaded; repair required"
            : "Application settings loaded"
    );
    if (loaded_settings.repair_required) {
        if (const auto error = settings_service_.save(application_settings_);
            error.has_value()) {
            error_state_.set(*error);
        }
    }

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
    return image_state_.source();
}

bool SegmentationViewModel::image_loaded() const noexcept {
    return image_state_.loaded();
}

int SegmentationViewModel::image_width() const noexcept {
    return image_state_.width();
}

int SegmentationViewModel::image_height() const noexcept {
    return image_state_.height();
}

quint64 SegmentationViewModel::image_version() const noexcept {
    return image_state_.version();
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
    return progress_state_.stage();
}

double SegmentationViewModel::progress_fraction() const noexcept {
    return progress_state_.fraction();
}

bool SegmentationViewModel::progress_indeterminate() const noexcept {
    return progress_state_.indeterminate();
}

QString SegmentationViewModel::status_text() const {
    return progress_state_.status_text();
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
    return result_state_.has_result();
}

QString SegmentationViewModel::result_source() const {
    return result_state_.source();
}

quint64 SegmentationViewModel::result_version() const noexcept {
    return result_state_.version();
}

QAbstractItemModel* SegmentationViewModel::seed_model() noexcept {
    return &seed_model_;
}

int SegmentationViewModel::selected_label() const noexcept {
    return selected_label_;
}

QString SegmentationViewModel::error_message() const {
    return error_state_.message();
}

int SegmentationViewModel::background_seed_count() const noexcept {
    return seed_state_.pixel_count(
        random_walker::domain::SeedLabel::Background
    );
}

int SegmentationViewModel::object_seed_count() const noexcept {
    return seed_state_.pixel_count(
        random_walker::domain::SeedLabel::Object
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
    random_walker::infrastructure::log_info(
        "viewmodel"
        , "Image open requested"
    );

    random_walker::presentation::ImageLoadOutcome load_outcome =
        random_walker::presentation::load_image(path);
    if (const auto* error =
            std::get_if<random_walker::application::ImageLoadError>(
                &load_outcome)) {
        random_walker::infrastructure::log_warning(
            "viewmodel"
            , "Image loading failed"
        );
        set_error(*error);
        return;
    }
    QImage loaded = std::get<QImage>(std::move(load_outcome));

    const bool previous_can_run = can_run();
    const bool was_loaded = image_loaded();

    cancel_active_request();
    image_state_.set(loaded);
    random_walker::infrastructure::log_info(
        "viewmodel"
        , std::string("Image loaded: ")
            + std::to_string(image_state_.width())
            + "x"
            + std::to_string(image_state_.height())
    );
    clear_seed_regions();
    invalidate_result();
    clear_error();

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

    if (!image_loaded() && seed_state_.empty() && !has_result()
        && !error_state_.has_error()) {
        return;
    }

    const bool previous_can_run = can_run();
    const bool was_loaded = image_loaded();

    random_walker::infrastructure::log_info(
        "viewmodel"
        , "Clearing image and segmentation state"
    );
    cancel_active_request();
    image_state_.clear();
    clear_seed_regions();
    invalidate_result();
    clear_error();

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

    if (seed_state_.empty()) {
        return;
    }

    const bool previous_can_run = can_run();
    random_walker::infrastructure::log_info(
        "viewmodel"
        , "Clearing seed regions"
    );
    cancel_active_request();
    clear_seed_regions();
    invalidate_result();
    clear_error();

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

    const int left = std::clamp(x, 0, image_state_.width());
    const int top = std::clamp(y, 0, image_state_.height());
    const auto right_edge = static_cast<long long>(x) + width;
    const auto bottom_edge = static_cast<long long>(y) + height;
    const int right = static_cast<int>(std::clamp(
        right_edge
        , 0LL
        , static_cast<long long>(image_state_.width()))
    );
    const int bottom = static_cast<int>(std::clamp(
        bottom_edge
        , 0LL
        , static_cast<long long>(image_state_.height()))
    );

    if (left >= right || top >= bottom) {
        return;
    }

    const bool previous_can_run = can_run();
    const DomainSeedLabel label = domain_seed_label();

    cancel_active_request();
    random_walker::infrastructure::log_debug(
        "viewmodel"
        , std::string("Adding seed rectangle: ")
            + std::to_string(right - left)
            + "x"
            + std::to_string(bottom - top)
    );
    add_seed_region({
        .area = {
            .x = left
            , .y = top
            , .width = right - left
            , .height = bottom - top
        }
        , .label = label
    });

    invalidate_result();
    clear_error();
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
        , image_state_.image()
        , seed_state_.regions()
        , application_settings_.random_walker
    );

    random_walker::infrastructure::log_info(
        "viewmodel"
        , std::string("Segmentation requested: request_id=")
            + std::to_string(request_id)
            + ", image="
            + std::to_string(image_state_.width())
            + "x"
            + std::to_string(image_state_.height())
            + ", background_seed_pixels="
            + std::to_string(background_seed_count())
            + ", object_seed_pixels="
            + std::to_string(object_seed_count())
            + ", beta="
            + std::to_string(application_settings_.random_walker.beta)
    );

    active_request_id_ = request_id;
    reset_progress();
    set_busy(true);
    clear_error();

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
    if (const auto error = settings_service_.save(updated_settings);
        error.has_value()) {
        random_walker::infrastructure::log_warning(
            "viewmodel"
            , "Failed to save Random Walker parameters"
        );
        set_error(*error);
        return;
    }

    random_walker::infrastructure::log_info(
        "viewmodel"
        , std::string("Random Walker parameters updated: beta=")
            + std::to_string(updated_settings.random_walker.beta)
    );

    cancel_active_request();
    invalidate_result();
    clear_error();

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

    random_walker::infrastructure::log_info(
        "viewmodel"
        , std::string("Segmentation completion received: request_id=")
            + std::to_string(completion.request_id)
    );
    active_request_id_.reset();
    set_busy(false);

    if (auto* execution_error =
            std::get_if<random_walker::executor::ExecutionError>(
                &completion.outcome)) {
        random_walker::infrastructure::log_error(
            "viewmodel"
            , "Segmentation finished with executor error"
        );
        invalidate_result();
        set_error(application_error(*execution_error));
        reset_progress();
        return;
    }

    auto* segmentation_outcome =
        std::get_if<random_walker::domain::SegmentationOutcome>(
            &completion.outcome);
    if (!segmentation_outcome) {
        random_walker::infrastructure::log_warning(
            "viewmodel"
            , "Segmentation completion payload was unexpected"
        );
        invalidate_result();
        set_error(
            random_walker::application::ApplicationError::
                UnexpectedInternalFailure
        );
        reset_progress();
        return;
    }

    if (auto* segmentation_result =
            std::get_if<random_walker::domain::SegmentationResult>(
                segmentation_outcome)) {
        random_walker::infrastructure::log_info(
            "viewmodel"
            , "Segmentation completed successfully"
        );
        result_state_.set(std::move(*segmentation_result));
        emit result_changed();
    } else if (auto* error =
                   std::get_if<random_walker::domain::SegmentationError>(
                       segmentation_outcome)) {
        random_walker::infrastructure::log_warning(
            "viewmodel"
            , "Segmentation finished with domain error"
        );
        invalidate_result();
        set_error(*error);
    } else {
        random_walker::infrastructure::log_info(
            "viewmodel"
            , "Segmentation was cancelled"
        );
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

    if (progress_state_.apply(progress)) {
        emit progress_changed();
    }
}

void SegmentationViewModel::clear_seed_regions() {
    assert_ui_thread();

    seed_model_.reset([this] {
        seed_state_.clear();
    });
}

void SegmentationViewModel::add_seed_region(
    random_walker::domain::SeedRegion region) {
    assert_ui_thread();

    seed_model_.reset([this, region = std::move(region)] mutable {
        seed_state_.add(std::move(region));
    });
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

    if (progress_state_.reset()) {
        emit progress_changed();
    }
}

void SegmentationViewModel::invalidate_result() {
    assert_ui_thread();

    if (result_state_.clear()) {
        emit result_changed();
    }
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

void SegmentationViewModel::set_error(
    random_walker::application::UserError error) {
    assert_ui_thread();

    if (error_state_.set(std::move(error))) {
        emit error_message_changed();
    }
}

void SegmentationViewModel::clear_error() {
    assert_ui_thread();

    if (error_state_.clear()) {
        emit error_message_changed();
    }
}

void SegmentationViewModel::notify_can_run_if_changed(bool previous_value) {
    if (previous_value != can_run()) {
        emit can_run_changed();
    }
}

void SegmentationViewModel::assert_ui_thread() const {
    Q_ASSERT(QThread::currentThread() == thread());
}

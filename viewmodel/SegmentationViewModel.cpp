#include "SegmentationViewModel.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <QImage>
#include <QMetaObject>
#include <QThread>
#include <QtGlobal>

#include "application/diagnostics/Logging.hpp"
#include "application/segmentation/SegmentationUseCase.hpp"
#include "presentation/image/ImageLoader.hpp"

namespace {

    const double kMinimumBetaExponent =
        std::log10(random_walker::domain::kMinimumRandomWalkerBeta);

    const double kMaximumBetaExponent =
        std::log10(random_walker::domain::kMaximumRandomWalkerBeta);

    [[nodiscard]] const char* auto_marker_error_name(
        random_walker::application::AutoMarkerError error
    ) noexcept {
        using Error = random_walker::application::AutoMarkerError;
        switch (error) {
        case Error::EmptyImage:
            return "EmptyImage";
        case Error::InvalidParameters:
            return "InvalidParameters";
        case Error::ProposalFailed:
            return "ProposalFailed";
        }

        return "Unknown";
    }

    [[nodiscard]] random_walker::application::ApplicationError
    application_error(random_walker::executor::ExecutionError error
    ) noexcept {
        using Error = random_walker::executor::ExecutionError;

        switch (error) {
        case Error::UnexpectedInternalFailure:
            return random_walker::application::ApplicationError::
                UnexpectedInternalFailure;
        }

        Q_ASSERT_X(false, "application_error", "Unhandled executor error");
        return random_walker::application::ApplicationError::
            UnexpectedInternalFailure;
    }

    [[nodiscard]] int view_connectivity(
        random_walker::domain::PixelConnectivity connectivity
    ) noexcept {
        switch (connectivity) {
        case random_walker::domain::PixelConnectivity::Four:
            return SegmentationViewModel::FourConnectivity;
        case random_walker::domain::PixelConnectivity::Eight:
            return SegmentationViewModel::EightConnectivity;
        }

        Q_ASSERT_X(false, "view_connectivity", "Unhandled connectivity");
        return SegmentationViewModel::FourConnectivity;
    }

    [[nodiscard]] std::optional<random_walker::domain::PixelConnectivity>
    domain_connectivity(int connectivity) noexcept {
        switch (connectivity) {
        case SegmentationViewModel::FourConnectivity:
            return random_walker::domain::PixelConnectivity::Four;
        case SegmentationViewModel::EightConnectivity:
            return random_walker::domain::PixelConnectivity::Eight;
        default:
            return std::nullopt;
        }
    }

    [[nodiscard]] int view_foreground_polarity(
        random_walker::domain::ForegroundPolarity foreground_polarity
    ) noexcept {
        switch (foreground_polarity) {
        case random_walker::domain::ForegroundPolarity::DarkObject:
            return SegmentationViewModel::DarkObjectForeground;
        case random_walker::domain::ForegroundPolarity::BrightObject:
            return SegmentationViewModel::BrightObjectForeground;
        }

        Q_ASSERT_X(
            false
            , "view_foreground_polarity"
            , "Unhandled foreground polarity"
        );
        return SegmentationViewModel::BrightObjectForeground;
    }

    [[nodiscard]] std::optional<random_walker::domain::ForegroundPolarity>
    domain_foreground_polarity(int foreground_polarity) noexcept {
        switch (foreground_polarity) {
        case SegmentationViewModel::DarkObjectForeground:
            return random_walker::domain::ForegroundPolarity::DarkObject;
        case SegmentationViewModel::BrightObjectForeground:
            return random_walker::domain::ForegroundPolarity::BrightObject;
        default:
            return std::nullopt;
        }
    }

    [[nodiscard]] std::optional<random_walker::domain::PixelRectangle>
    clipped_seed_rectangle(
        int x
        , int y
        , int width
        , int height
        , int image_width
        , int image_height
    ) noexcept {
        if (width <= 0 || height <= 0) {
            return std::nullopt;
        }

        const int left = std::clamp(x, 0, image_width);
        const int top = std::clamp(y, 0, image_height);
        const auto right_edge = static_cast<long long>(x) + width;
        const auto bottom_edge = static_cast<long long>(y) + height;
        const int right = static_cast<int>(std::clamp(
            right_edge
            , 0LL
            , static_cast<long long>(image_width))
        );
        const int bottom = static_cast<int>(std::clamp(
            bottom_edge
            , 0LL
            , static_cast<long long>(image_height))
        );

        if (left >= right || top >= bottom) {
            return std::nullopt;
        }

        return random_walker::domain::PixelRectangle {
            .x = left
            , .y = top
            , .width = right - left
            , .height = bottom - top
        };
    }

}

struct SegmentationViewModel::CompletionDeliveryGate {
    std::mutex mutex;
    SegmentationViewModel* receiver = nullptr;
};

SegmentationViewModel::SegmentationViewModel(
    random_walker::executor::SegmentationExecutor& segmentation_executor
    , random_walker::application::SettingsService& settings_service
    , random_walker::application::AutoMarkerExecutor& auto_marker_executor
    , PresentationImageCache& base_image_cache
    , PresentationImageCache& auto_marker_image_cache
    , PresentationImageCache& result_image_cache
    , QObject* parent
)
    : QObject(parent)
    , segmentation_executor_(segmentation_executor)
    , settings_service_(settings_service)
    , auto_marker_executor_(auto_marker_executor)
    , image_state_(base_image_cache)
    , result_state_(result_image_cache)
    , automatic_marker_state_(auto_marker_image_cache)
    , seed_model_(seed_state_.mutable_regions())
    , completion_delivery_(std::make_shared<CompletionDeliveryGate>()) {
    const auto loaded_settings = settings_service_.load();
    application_settings_ = loaded_settings.settings;
    Q_ASSERT(random_walker::domain::is_valid(
        application_settings_.random_walker
    ));
    Q_ASSERT(random_walker::domain::is_valid(
        application_settings_.auto_markers
    ));
    random_walker::application::log_info(
        random_walker::application::log_category::viewmodel
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
    auto_marker_executor_.cancel();
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
        && background_constraint_count() > 0
        && object_constraint_count() > 0;
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

int SegmentationViewModel::connectivity() const noexcept {
    return view_connectivity(application_settings_.random_walker.connectivity);
}

double SegmentationViewModel::distance_power() const noexcept {
    return application_settings_.random_walker.distance_power;
}

double SegmentationViewModel::auto_marker_confidence_threshold()
    const noexcept {
    return application_settings_.auto_markers.confidence_threshold;
}

int SegmentationViewModel::auto_marker_minimum_boundary_distance()
    const noexcept {
    return application_settings_.auto_markers.minimum_boundary_distance;
}

int SegmentationViewModel::auto_marker_minimum_component_area()
    const noexcept {
    return application_settings_.auto_markers.minimum_component_area;
}

int SegmentationViewModel::auto_marker_foreground_polarity() const noexcept {
    return view_foreground_polarity(
        application_settings_.auto_markers.foreground_polarity
    );
}

double SegmentationViewModel::minimum_auto_marker_confidence_threshold()
    const noexcept {
    return random_walker::domain::kMinimumAutoMarkerConfidenceThreshold;
}

double SegmentationViewModel::maximum_auto_marker_confidence_threshold()
    const noexcept {
    return random_walker::domain::kMaximumAutoMarkerConfidenceThreshold;
}

int SegmentationViewModel::minimum_auto_marker_boundary_distance()
    const noexcept {
    return random_walker::domain::kMinimumAutoMarkerBoundaryDistance;
}

int SegmentationViewModel::maximum_auto_marker_boundary_distance()
    const noexcept {
    return random_walker::domain::kMaximumAutoMarkerBoundaryDistance;
}

int SegmentationViewModel::minimum_auto_marker_component_area()
    const noexcept {
    return random_walker::domain::kMinimumAutoMarkerComponentArea;
}

int SegmentationViewModel::maximum_auto_marker_component_area()
    const noexcept {
    return random_walker::domain::kMaximumAutoMarkerComponentArea;
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

bool SegmentationViewModel::has_automatic_markers() const noexcept {
    return automatic_marker_state_.has_markers();
}

QString SegmentationViewModel::automatic_marker_source() const {
    return automatic_marker_state_.source();
}

quint64 SegmentationViewModel::automatic_marker_version() const noexcept {
    return automatic_marker_state_.version();
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

int SegmentationViewModel::automatic_marker_count() const noexcept {
    return automatic_marker_state_.marker_count();
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

void SegmentationViewModel::set_connectivity(int connectivity) {
    assert_ui_thread();

    const std::optional<DomainConnectivity> updated_connectivity =
        domain_connectivity(connectivity);
    if (!updated_connectivity.has_value()) {
        return;
    }

    auto updated_parameters = application_settings_.random_walker;
    updated_parameters.connectivity = *updated_connectivity;
    update_random_walker_parameters(updated_parameters);
}

void SegmentationViewModel::set_distance_power(double value) {
    assert_ui_thread();

    auto updated_parameters = application_settings_.random_walker;
    updated_parameters.distance_power = value;
    update_random_walker_parameters(updated_parameters);
}

void SegmentationViewModel::set_auto_marker_confidence_threshold(
    double value
) {
    assert_ui_thread();

    auto updated_parameters = application_settings_.auto_markers;
    updated_parameters.confidence_threshold = value;
    update_auto_marker_parameters(updated_parameters);
}

void SegmentationViewModel::set_auto_marker_minimum_boundary_distance(
    int value
) {
    assert_ui_thread();

    auto updated_parameters = application_settings_.auto_markers;
    updated_parameters.minimum_boundary_distance = value;
    update_auto_marker_parameters(updated_parameters);
}

void SegmentationViewModel::set_auto_marker_minimum_component_area(
    int value
) {
    assert_ui_thread();

    auto updated_parameters = application_settings_.auto_markers;
    updated_parameters.minimum_component_area = value;
    update_auto_marker_parameters(updated_parameters);
}

void SegmentationViewModel::set_auto_marker_foreground_polarity(int polarity) {
    assert_ui_thread();

    const std::optional<DomainForegroundPolarity> updated_polarity =
        domain_foreground_polarity(polarity);
    if (!updated_polarity.has_value()) {
        return;
    }

    auto updated_parameters = application_settings_.auto_markers;
    updated_parameters.foreground_polarity = *updated_polarity;
    update_auto_marker_parameters(updated_parameters);
}

void SegmentationViewModel::open_image(const QString& path) {
    assert_ui_thread();
    random_walker::application::log_info(
        random_walker::application::log_category::viewmodel
        , "Image open requested"
    );

    random_walker::presentation::ImageLoadOutcome load_outcome =
        random_walker::presentation::load_image(path);
    if (
        const auto* error =
            std::get_if<random_walker::application::ImageLoadError>(
                &load_outcome
            )
    ) {
        random_walker::application::log_warning(
            random_walker::application::log_category::viewmodel
            , "Image loading failed"
        );
        set_error(*error);
        return;
    }
    QImage loaded = std::get<QImage>(std::move(load_outcome));

    const bool previous_can_run = can_run();
    const bool was_loaded = image_loaded();

    cancel_active_request();
    cancel_active_auto_marker_request();
    image_state_.set(loaded);
    const bool automatic_markers_were_cleared = automatic_marker_state_.clear();
    random_walker::application::log_info(
        random_walker::application::log_category::viewmodel
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
    if (automatic_markers_were_cleared) {
        emit automatic_markers_changed();
    }
    emit seeds_changed();
    if (!was_loaded) {
        emit image_loaded_changed(true);
    }
    notify_can_run_if_changed(previous_can_run);
}

void SegmentationViewModel::clear() {
    assert_ui_thread();

    if (!image_loaded() && seed_state_.empty()
        && !automatic_marker_state_.has_markers() && !has_result()
        && !error_state_.has_error()) {
        return;
    }

    const bool previous_can_run = can_run();
    const bool was_loaded = image_loaded();

    random_walker::application::log_info(
        random_walker::application::log_category::viewmodel
        , "Clearing image and segmentation state"
    );
    cancel_active_request();
    cancel_active_auto_marker_request();
    image_state_.clear();
    clear_seed_regions();
    const bool automatic_markers_were_cleared = automatic_marker_state_.clear();
    invalidate_result();
    clear_error();

    emit image_version_changed();
    emit image_source_changed();
    emit image_dimensions_changed();
    if (automatic_markers_were_cleared) {
        emit automatic_markers_changed();
    }
    emit seeds_changed();
    if (was_loaded) {
        emit image_loaded_changed(false);
    }
    notify_can_run_if_changed(previous_can_run);
}

void SegmentationViewModel::clear_seeds() {
    assert_ui_thread();

    if (seed_state_.empty() && !automatic_marker_state_.has_markers()) {
        return;
    }

    const bool previous_can_run = can_run();
    random_walker::application::log_info(
        random_walker::application::log_category::viewmodel
        , "Clearing seed regions"
    );
    cancel_active_request();
    cancel_active_auto_marker_request();
    clear_seed_regions();
    const bool automatic_markers_were_cleared = automatic_marker_state_.clear();
    invalidate_result();
    clear_error();

    if (automatic_markers_were_cleared) {
        emit automatic_markers_changed();
    }
    emit seeds_changed();
    notify_can_run_if_changed(previous_can_run);
}

void SegmentationViewModel::clear_automatic_markers() {
    assert_ui_thread();

    if (automatic_marker_count() == 0) {
        return;
    }

    const bool previous_can_run = can_run();
    random_walker::application::log_info(
        random_walker::application::log_category::viewmodel
        , "Clearing automatic markers"
    );
    cancel_active_request();
    cancel_active_auto_marker_request();
    automatic_marker_state_.clear();
    invalidate_result();
    clear_error();

    emit automatic_markers_changed();
    emit seeds_changed();
    notify_can_run_if_changed(previous_can_run);
}

void SegmentationViewModel::propose_markers() {
    assert_ui_thread();

    if (busy_) {
        return;
    }

    if (!image_loaded()) {
        random_walker::application::log_warning(
            random_walker::application::log_category::viewmodel
            , "Automatic marker proposal rejected: image is not loaded"
        );
        set_error(random_walker::application::AutoMarkerError::EmptyImage);
        return;
    }

    const random_walker::application::AutoMarkerRequestId request_id =
        next_auto_marker_request_id_++;
    random_walker::application::AutoMarkerRequest request(
        request_id
        , image_state_.image()
        , application_settings_.auto_markers
        , seed_state_.regions()
    );

    random_walker::application::log_info(
        random_walker::application::log_category::viewmodel
        , std::string("Automatic marker proposal requested: request_id=")
            + std::to_string(request_id)
    );

    active_auto_marker_request_id_ = request_id;
    set_busy(true);
    clear_error();

    const std::shared_ptr<CompletionDeliveryGate> delivery_gate =
        completion_delivery_;
    auto_marker_executor_.submit(
        std::move(request)
        , [delivery_gate](
            random_walker::application::AutoMarkerCompletion completion) {
            SegmentationViewModel::dispatch_auto_marker_completion(
                delivery_gate
                , std::move(completion)
            );
        }
    );
}

void SegmentationViewModel::add_seed_rectangle(
    int x
    , int y
    , int width
    , int height
) {
    assert_ui_thread();

    if (!image_loaded()) {
        return;
    }

    const std::optional<random_walker::domain::PixelRectangle> area =
        clipped_seed_rectangle(
            x
            , y
            , width
            , height
            , image_state_.width()
            , image_state_.height()
        );
    if (!area.has_value()) {
        return;
    }

    const bool previous_can_run = can_run();
    const DomainSeedLabel label = domain_seed_label();

    cancel_active_request();
    cancel_active_auto_marker_request();
    const bool automatic_markers_were_cleared = automatic_marker_state_.clear();
    random_walker::application::log_debug(
        random_walker::application::log_category::viewmodel
        , std::string("Adding seed rectangle: ")
            + std::to_string(area->width)
            + "x"
            + std::to_string(area->height)
    );
    add_seed_region({
        .area = *area
        , .label = label
    });

    invalidate_result();
    clear_error();
    if (automatic_markers_were_cleared) {
        emit automatic_markers_changed();
    }
    emit seeds_changed();
    notify_can_run_if_changed(previous_can_run);
}

void SegmentationViewModel::run_segmentation() {
    assert_ui_thread();

    if (busy_ || !can_run()) {
        return;
    }
    Q_ASSERT(image_loaded());
    Q_ASSERT(background_constraint_count() > 0);
    Q_ASSERT(object_constraint_count() > 0);
    Q_ASSERT(random_walker::domain::is_valid(
        application_settings_.random_walker
    ));

    const bool previous_can_run = can_run();
    const random_walker::domain::SegmentationRequestId request_id =
        next_request_id_++;
    std::vector<random_walker::domain::SeedRegion> seed_regions =
        segmentation_seed_regions();
    const int background_constraints =
        random_walker::domain::seed_pixel_count(
            seed_regions
            , DomainSeedLabel::Background
        );
    const int object_constraints =
        random_walker::domain::seed_pixel_count(
            seed_regions
            , DomainSeedLabel::Object
        );
    Q_ASSERT(background_constraints > 0);
    Q_ASSERT(object_constraints > 0);
    random_walker::domain::SegmentationRequest request =
        random_walker::application::make_segmentation_request(
            request_id
            , image_state_.image()
            , std::move(seed_regions)
            , application_settings_
        );
    const bool automatic_markers_were_cleared = automatic_marker_state_.clear();

    random_walker::application::log_info(
        random_walker::application::log_category::viewmodel
        , std::string("Segmentation requested: request_id=")
            + std::to_string(request_id)
            + ", image="
            + std::to_string(image_state_.width())
            + "x"
            + std::to_string(image_state_.height())
            + ", background_seed_pixels="
            + std::to_string(background_constraints)
            + ", object_seed_pixels="
            + std::to_string(object_constraints)
            + ", beta="
            + std::to_string(application_settings_.random_walker.beta)
            + ", connectivity="
            + std::to_string(connectivity())
            + ", distance_power="
            + std::to_string(application_settings_.random_walker.distance_power)
    );

    active_request_id_ = request_id;
    reset_progress();
    set_busy(true);
    clear_error();
    if (automatic_markers_were_cleared) {
        emit automatic_markers_changed();
        emit seeds_changed();
    }
    notify_can_run_if_changed(previous_can_run);

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

int SegmentationViewModel::background_constraint_count() const noexcept {
    return background_seed_count() + automatic_marker_state_.background_count();
}

int SegmentationViewModel::object_constraint_count() const noexcept {
    return object_seed_count() + automatic_marker_state_.object_count();
}

std::vector<random_walker::domain::SeedRegion>
SegmentationViewModel::segmentation_seed_regions() const {
    std::vector<random_walker::domain::SeedRegion> regions =
        seed_state_.regions();
    std::vector<random_walker::domain::SeedRegion> automatic_regions =
        automatic_marker_state_.seed_regions();
    regions.insert(
        regions.end()
        , std::make_move_iterator(automatic_regions.begin())
        , std::make_move_iterator(automatic_regions.end())
    );
    return regions;
}

void SegmentationViewModel::update_random_walker_parameters(
    random_walker::domain::RandomWalkerParameters parameters) {
    assert_ui_thread();

    if (parameters == application_settings_.random_walker) {
        return;
    }

    const bool beta_was_changed =
        parameters.beta != application_settings_.random_walker.beta;
    const bool connectivity_was_changed =
        parameters.connectivity
            != application_settings_.random_walker.connectivity;
    const bool distance_power_was_changed =
        parameters.distance_power
            != application_settings_.random_walker.distance_power;
    auto updated_settings = application_settings_;
    updated_settings.random_walker = parameters;
    if (const auto error = settings_service_.save(updated_settings);
        error.has_value()) {
        random_walker::application::log_warning(
            random_walker::application::log_category::viewmodel
            , "Failed to save Random Walker parameters"
        );
        set_error(*error);
        return;
    }

    random_walker::application::log_info(
        random_walker::application::log_category::viewmodel
        , std::string("Random Walker parameters updated: beta=")
            + std::to_string(updated_settings.random_walker.beta)
            + ", connectivity="
            + std::to_string(
                view_connectivity(updated_settings.random_walker.connectivity)
            )
            + ", distance_power="
            + std::to_string(updated_settings.random_walker.distance_power)
    );

    cancel_active_request();
    invalidate_result();
    clear_error();

    application_settings_ = std::move(updated_settings);
    if (beta_was_changed) {
        emit beta_changed();
    }
    if (connectivity_was_changed) {
        emit connectivity_changed();
    }
    if (distance_power_was_changed) {
        emit distance_power_changed();
    }
}

void SegmentationViewModel::update_auto_marker_parameters(
    random_walker::domain::AutoMarkerParameters parameters) {
    assert_ui_thread();

    if (parameters == application_settings_.auto_markers) {
        return;
    }

    const bool confidence_threshold_was_changed =
        parameters.confidence_threshold
            != application_settings_.auto_markers.confidence_threshold;
    const bool minimum_boundary_distance_was_changed =
        parameters.minimum_boundary_distance
            != application_settings_.auto_markers.minimum_boundary_distance;
    const bool minimum_component_area_was_changed =
        parameters.minimum_component_area
            != application_settings_.auto_markers.minimum_component_area;
    const bool foreground_polarity_was_changed =
        parameters.foreground_polarity
            != application_settings_.auto_markers.foreground_polarity;
    auto updated_settings = application_settings_;
    updated_settings.auto_markers = parameters;
    if (const auto error = settings_service_.save(updated_settings);
        error.has_value()) {
        random_walker::application::log_warning(
            random_walker::application::log_category::viewmodel
            , "Failed to save automatic marker parameters"
        );
        set_error(*error);
        return;
    }

    random_walker::application::log_info(
        random_walker::application::log_category::viewmodel
        , std::string("Automatic marker parameters updated: ")
            + "confidence_threshold="
            + std::to_string(updated_settings.auto_markers.confidence_threshold)
            + ", minimum_component_area="
            + std::to_string(
                updated_settings.auto_markers.minimum_component_area
            )
            + ", minimum_boundary_distance="
            + std::to_string(
                updated_settings.auto_markers.minimum_boundary_distance
            )
            + ", foreground_polarity="
            + std::to_string(view_foreground_polarity(
                updated_settings.auto_markers.foreground_polarity
            ))
    );

    const bool previous_can_run = can_run();
    cancel_active_auto_marker_request();
    const bool automatic_markers_were_cleared =
        automatic_marker_state_.clear();
    invalidate_result();
    clear_error();

    application_settings_ = std::move(updated_settings);
    if (confidence_threshold_was_changed) {
        emit auto_marker_confidence_threshold_changed();
    }
    if (minimum_boundary_distance_was_changed) {
        emit auto_marker_minimum_boundary_distance_changed();
    }
    if (minimum_component_area_was_changed) {
        emit auto_marker_minimum_component_area_changed();
    }
    if (foreground_polarity_was_changed) {
        emit auto_marker_foreground_polarity_changed();
    }
    if (automatic_markers_were_cleared) {
        emit automatic_markers_changed();
        emit seeds_changed();
    }
    notify_can_run_if_changed(previous_can_run);
}

void SegmentationViewModel::dispatch_completion(
    const std::shared_ptr<CompletionDeliveryGate>& delivery_gate
    , random_walker::executor::SegmentationCompletion completion
) {
    Q_ASSERT(delivery_gate);

    auto payload =
        std::make_shared<random_walker::executor::SegmentationCompletion>(
            std::move(completion)
        );

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

void SegmentationViewModel::dispatch_auto_marker_completion(
    const std::shared_ptr<CompletionDeliveryGate>& delivery_gate
    , random_walker::application::AutoMarkerCompletion completion
) {
    Q_ASSERT(delivery_gate);

    auto payload =
        std::make_shared<random_walker::application::AutoMarkerCompletion>(
            std::move(completion)
        );

    std::lock_guard lock(delivery_gate->mutex);
    SegmentationViewModel* receiver = delivery_gate->receiver;
    if (!receiver) {
        return;
    }

    QMetaObject::invokeMethod(
        receiver
        , [receiver, payload] {
            receiver->handle_auto_marker_completion(std::move(*payload));
        }
        , Qt::QueuedConnection
    );
}

void SegmentationViewModel::dispatch_progress(
    const std::shared_ptr<CompletionDeliveryGate>& delivery_gate
    , random_walker::domain::SegmentationProgress progress
) {
    Q_ASSERT(delivery_gate);

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

    random_walker::application::log_info(
        random_walker::application::log_category::viewmodel
        , std::string("Segmentation completion received: request_id=")
            + std::to_string(completion.request_id)
    );
    active_request_id_.reset();
    set_busy(false);

    if (
        auto* execution_error =
            std::get_if<random_walker::executor::ExecutionError>(
                &completion.outcome
            )
    ) {
        random_walker::application::log_error(
            random_walker::application::log_category::viewmodel
            , "Segmentation finished with executor error"
        );
        invalidate_result();
        set_error(application_error(*execution_error));
        reset_progress();
        return;
    }

    auto* segmentation_outcome =
        std::get_if<random_walker::domain::SegmentationOutcome>(
            &completion.outcome
        );

    if (!segmentation_outcome) {
        random_walker::application::log_warning(
            random_walker::application::log_category::viewmodel
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

    if (
        auto* segmentation_result =
            std::get_if<random_walker::domain::SegmentationResult>(
                segmentation_outcome
            )
    ) {
        random_walker::application::log_info(
            random_walker::application::log_category::viewmodel
            , "Segmentation completed successfully"
        );
        result_state_.set(std::move(*segmentation_result));
        emit result_changed();
    } else if (
        auto* error = std::get_if<random_walker::domain::SegmentationError>(
            segmentation_outcome
        )
    ) {
        random_walker::application::log_warning(
            random_walker::application::log_category::viewmodel
            , "Segmentation finished with domain error"
        );
        invalidate_result();
        set_error(*error);
    } else {
        random_walker::application::log_info(
            random_walker::application::log_category::viewmodel
            , "Segmentation was cancelled"
        );
        invalidate_result();
    }

    reset_progress();
}

void SegmentationViewModel::handle_auto_marker_completion(
    random_walker::application::AutoMarkerCompletion completion
) {
    assert_ui_thread();

    if (!active_auto_marker_request_id_.has_value()
        || *active_auto_marker_request_id_ != completion.request_id) {
        return;
    }

    random_walker::application::log_info(
        random_walker::application::log_category::viewmodel
        , std::string("Automatic marker completion received: request_id=")
            + std::to_string(completion.request_id)
    );
    active_auto_marker_request_id_.reset();
    set_busy(false);

    if (const auto* error =
            std::get_if<random_walker::application::AutoMarkerError>(
                &completion.outcome
            )
    ) {
        random_walker::application::log_warning(
            random_walker::application::log_category::viewmodel
            , std::string("Automatic marker proposal failed: error=")
                + auto_marker_error_name(*error)
        );
        set_error(*error);
        return;
    }

    const random_walker::domain::MarkerProposal& proposal =
        std::get<random_walker::domain::MarkerProposal>(completion.outcome);
    const bool previous_can_run = can_run();
    automatic_marker_state_.set(proposal.mask);

    random_walker::application::log_info(
        random_walker::application::log_category::viewmodel
        , std::string("Automatic marker mask applied: seeds=")
            + std::to_string(proposal.diagnostics.proposed_seed_count)
            + ", mask="
            + std::to_string(proposal.mask.width())
            + "x"
            + std::to_string(proposal.mask.height())
    );
    invalidate_result();
    clear_error();

    emit automatic_markers_changed();
    emit seeds_changed();
    notify_can_run_if_changed(previous_can_run);
}

void SegmentationViewModel::handle_progress(
    random_walker::domain::SegmentationProgress progress) {
    assert_ui_thread();

    if (
        !active_request_id_.has_value()
        || *active_request_id_ != progress.request_id
    ) {
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

void SegmentationViewModel::cancel_active_auto_marker_request() {
    assert_ui_thread();

    if (!active_auto_marker_request_id_.has_value()) {
        return;
    }

    auto_marker_executor_.cancel();
    active_auto_marker_request_id_.reset();
    set_busy(false);
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
    assert_ui_thread();

    if (previous_value != can_run()) {
        emit can_run_changed();
    }
}

void SegmentationViewModel::assert_ui_thread() const {
    Q_ASSERT(QThread::currentThread() == thread());
}

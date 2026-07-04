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

    [[nodiscard]] int view_edge_weight_model(
        random_walker::domain::EdgeWeightModel model
    ) noexcept {
        switch (model) {
        case random_walker::domain::EdgeWeightModel::GlobalBeta:
            return SegmentationViewModel::GlobalBetaWeight;
        case random_walker::domain::EdgeWeightModel::LocalVarianceNormalized:
            return SegmentationViewModel::LocalVarianceNormalizedWeight;
        }

        Q_ASSERT_X(
            false
            , "view_edge_weight_model"
            , "Unhandled edge weight model"
        );
        return SegmentationViewModel::GlobalBetaWeight;
    }

    [[nodiscard]] std::optional<random_walker::domain::EdgeWeightModel>
    domain_edge_weight_model(int model) noexcept {
        switch (model) {
        case SegmentationViewModel::GlobalBetaWeight:
            return random_walker::domain::EdgeWeightModel::GlobalBeta;
        case SegmentationViewModel::LocalVarianceNormalizedWeight:
            return random_walker::domain::EdgeWeightModel::LocalVarianceNormalized;
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
    , random_walker::application::AutoMarkerService& auto_marker_service
    , PresentationImageCache& base_image_cache
    , PresentationImageCache& auto_marker_image_cache
    , PresentationImageCache& result_image_cache
    , QObject* parent
)
    : QObject(parent)
    , segmentation_executor_(segmentation_executor)
    , settings_service_(settings_service)
    , auto_marker_service_(auto_marker_service)
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

int SegmentationViewModel::edge_weight_model() const noexcept {
    return view_edge_weight_model(
        application_settings_.random_walker.edge_weight_model
    );
}

int SegmentationViewModel::local_contrast_radius() const noexcept {
    return application_settings_.random_walker.local_contrast_scale.radius;
}

double SegmentationViewModel::local_contrast_minimum_variance() const noexcept {
    return application_settings_.random_walker
        .local_contrast_scale
        .minimum_variance;
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

void SegmentationViewModel::set_edge_weight_model(int model) {
    assert_ui_thread();

    const std::optional<DomainEdgeWeightModel> updated_model =
        domain_edge_weight_model(model);
    if (!updated_model.has_value()) {
        return;
    }

    auto updated_parameters = application_settings_.random_walker;
    updated_parameters.edge_weight_model = *updated_model;
    update_random_walker_parameters(updated_parameters);
}

void SegmentationViewModel::set_local_contrast_radius(int radius) {
    assert_ui_thread();

    auto updated_parameters = application_settings_.random_walker;
    updated_parameters.local_contrast_scale.radius = radius;
    update_random_walker_parameters(updated_parameters);
}

void SegmentationViewModel::set_local_contrast_minimum_variance(double value) {
    assert_ui_thread();

    auto updated_parameters = application_settings_.random_walker;
    updated_parameters.local_contrast_scale.minimum_variance = value;
    update_random_walker_parameters(updated_parameters);
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

    if (seed_state_.empty()) {
        return;
    }

    const bool previous_can_run = can_run();
    random_walker::application::log_info(
        random_walker::application::log_category::viewmodel
        , "Clearing seed regions"
    );
    cancel_active_request();
    clear_seed_regions();
    invalidate_result();
    clear_error();

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
    automatic_marker_state_.clear();
    invalidate_result();
    clear_error();

    emit automatic_markers_changed();
    emit seeds_changed();
    notify_can_run_if_changed(previous_can_run);
}

void SegmentationViewModel::propose_markers() {
    assert_ui_thread();

    if (!image_loaded()) {
        random_walker::application::log_warning(
            random_walker::application::log_category::viewmodel
            , "Automatic marker proposal rejected: image is not loaded"
        );
        set_error(random_walker::application::AutoMarkerError::EmptyImage);
        return;
    }

    random_walker::application::log_info(
        random_walker::application::log_category::viewmodel
        , "Automatic marker proposal requested"
    );

    const random_walker::application::AutoMarkerProposalOutcome outcome =
        auto_marker_service_.propose(
            image_state_.image()
            , random_walker::domain::AutoMarkerParameters {}
            , seed_state_.regions()
        );
    if (const auto* error =
            std::get_if<random_walker::application::AutoMarkerError>(&outcome)
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
        std::get<random_walker::domain::MarkerProposal>(outcome);
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

    if (!can_run()) {
        return;
    }
    Q_ASSERT(image_loaded());
    Q_ASSERT(background_constraint_count() > 0);
    Q_ASSERT(object_constraint_count() > 0);
    Q_ASSERT(random_walker::domain::is_valid(
        application_settings_.random_walker
    ));

    const random_walker::domain::SegmentationRequestId request_id =
        next_request_id_++;
    random_walker::domain::SegmentationRequest request(
        request_id
        , image_state_.image()
        , segmentation_seed_regions()
        , application_settings_.random_walker
    );

    random_walker::application::log_info(
        random_walker::application::log_category::viewmodel
        , std::string("Segmentation requested: request_id=")
            + std::to_string(request_id)
            + ", image="
            + std::to_string(image_state_.width())
            + "x"
            + std::to_string(image_state_.height())
            + ", background_seed_pixels="
            + std::to_string(background_constraint_count())
            + ", object_seed_pixels="
            + std::to_string(object_constraint_count())
            + ", beta="
            + std::to_string(application_settings_.random_walker.beta)
            + ", connectivity="
            + std::to_string(connectivity())
            + ", distance_power="
            + std::to_string(application_settings_.random_walker.distance_power)
            + ", edge_weight_model="
            + std::to_string(edge_weight_model())
            + ", local_contrast_radius="
            + std::to_string(
                application_settings_.random_walker.local_contrast_scale.radius
            )
            + ", local_contrast_minimum_variance="
            + std::to_string(
                application_settings_.random_walker
                    .local_contrast_scale
                    .minimum_variance
            )
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
    const bool edge_weight_model_was_changed =
        parameters.edge_weight_model
            != application_settings_.random_walker.edge_weight_model;
    const bool local_contrast_was_changed =
        parameters.local_contrast_scale
            != application_settings_.random_walker.local_contrast_scale;

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
            + ", edge_weight_model="
            + std::to_string(
                view_edge_weight_model(
                    updated_settings.random_walker.edge_weight_model
                )
            )
            + ", local_contrast_radius="
            + std::to_string(
                updated_settings.random_walker.local_contrast_scale.radius
            )
            + ", local_contrast_minimum_variance="
            + std::to_string(
                updated_settings.random_walker
                    .local_contrast_scale
                    .minimum_variance
            )
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
    if (edge_weight_model_was_changed) {
        emit edge_weight_model_changed();
    }
    if (local_contrast_was_changed) {
        emit local_contrast_changed();
    }
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

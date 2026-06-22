#include "SegmentationViewModel.hpp"

#include <algorithm>
#include <memory>
#include <mutex>
#include <utility>
#include <variant>

#include <QImage>
#include <QMetaObject>
#include <QThread>
#include <QUrl>

#include "app/adapter/QImageAdapter.hpp"
#include "app/adapter/MaskRenderer.hpp"
#include "app/qml/qml_names.hpp"

namespace
{
    [[nodiscard]] QString error_message(
        random_walker::domain::SegmentationError error)
    {
        using Error = random_walker::domain::SegmentationError;

        switch (error) {
        case Error::EmptyImage:
            return QStringLiteral("No image is loaded.");
        case Error::MissingBackgroundSeeds:
            return QStringLiteral("At least one background seed is required.");
        case Error::MissingObjectSeeds:
            return QStringLiteral("At least one object seed is required.");
        case Error::SeedOutOfBounds:
            return QStringLiteral("A seed lies outside the image.");
        case Error::LaplacianDecompositionFailed:
            return QStringLiteral("Failed to decompose the graph Laplacian.");
        case Error::LinearSystemSolveFailed:
            return QStringLiteral("Failed to solve the Random Walker system.");
        case Error::NonFiniteSolution:
            return QStringLiteral("The Random Walker solution is not finite.");
        }

        return QStringLiteral("Unknown segmentation error.");
    }
}

struct SegmentationViewModel::CompletionDeliveryGate
{
    std::mutex mutex;
    SegmentationViewModel* receiver = nullptr;
};

SegmentationViewModel::SegmentationViewModel(
    random_walker::executor::SegmentationExecutor& segmentation_executor,
    PresentationImageCache& base_image_cache,
    PresentationImageCache& result_image_cache,
    QObject* parent)
    : QObject(parent)
    , segmentation_executor_(segmentation_executor)
    , base_image_cache_(base_image_cache)
    , result_image_cache_(result_image_cache)
    , seed_model_(seed_regions_)
    , completion_delivery_(std::make_shared<CompletionDeliveryGate>())
{
    completion_delivery_->receiver = this;
}

SegmentationViewModel::~SegmentationViewModel()
{
    assert_ui_thread();

    {
        std::lock_guard lock(completion_delivery_->mutex);
        completion_delivery_->receiver = nullptr;
    }

    segmentation_executor_.cancel();
}

QString SegmentationViewModel::image_source() const
{
    return image_loaded()
        ? QStringLiteral("image://%1/processed").arg(qml_names::kBaseImageProvider)
        : QString();
}

bool SegmentationViewModel::image_loaded() const noexcept
{
    return !image_.empty();
}

int SegmentationViewModel::image_width() const noexcept
{
    return image_.width();
}

int SegmentationViewModel::image_height() const noexcept
{
    return image_.height();
}

quint64 SegmentationViewModel::image_version() const noexcept
{
    return image_version_;
}

bool SegmentationViewModel::can_run() const noexcept
{
    return image_loaded()
        && background_seed_count() > 0
        && object_seed_count() > 0;
}

bool SegmentationViewModel::busy() const noexcept
{
    return busy_;
}

bool SegmentationViewModel::has_result() const noexcept
{
    return result_.has_value();
}

QString SegmentationViewModel::result_source() const
{
    return has_result()
        ? QStringLiteral("image://%1/mask").arg(qml_names::kResultImageProvider)
        : QString();
}

quint64 SegmentationViewModel::result_version() const noexcept
{
    return result_version_;
}

QAbstractItemModel* SegmentationViewModel::seed_model() noexcept
{
    return &seed_model_;
}

int SegmentationViewModel::selected_label() const noexcept
{
    return selected_label_;
}

QString SegmentationViewModel::error_message() const
{
    return error_message_;
}

int SegmentationViewModel::background_seed_count() const noexcept
{
    return random_walker::domain::seed_pixel_count(
        seed_regions_,
        random_walker::domain::SeedLabel::Background);
}

int SegmentationViewModel::object_seed_count() const noexcept
{
    return random_walker::domain::seed_pixel_count(
        seed_regions_,
        random_walker::domain::SeedLabel::Object);
}

void SegmentationViewModel::set_selected_label(int label)
{
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

void SegmentationViewModel::open_image(const QString& path)
{
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

void SegmentationViewModel::clear()
{
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

void SegmentationViewModel::clear_seeds()
{
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
    int x,
    int y,
    int width,
    int height)
{
    assert_ui_thread();

    if (!image_loaded() || width <= 0 || height <= 0) {
        return;
    }

    const int left = std::clamp(x, 0, image_.width());
    const int top = std::clamp(y, 0, image_.height());
    const auto right_edge = static_cast<long long>(x) + width;
    const auto bottom_edge = static_cast<long long>(y) + height;
    const int right = static_cast<int>(std::clamp(
        right_edge,
        0LL,
        static_cast<long long>(image_.width())));
    const int bottom = static_cast<int>(std::clamp(
        bottom_edge,
        0LL,
        static_cast<long long>(image_.height())));

    if (left >= right || top >= bottom) {
        return;
    }

    const bool previous_can_run = can_run();
    const DomainSeedLabel label = domain_seed_label();

    cancel_active_request();
    seed_model_.reset([this, left, top, right, bottom, label] {
        seed_regions_.push_back({
            .area = {
                .x = left,
                .y = top,
                .width = right - left,
                .height = bottom - top
            },
            .label = label
        });
    });

    invalidate_result();
    set_error({});
    emit seeds_changed();
    notify_can_run_if_changed(previous_can_run);
}

void SegmentationViewModel::run_segmentation()
{
    assert_ui_thread();

    if (!can_run()) {
        return;
    }

    const random_walker::domain::SegmentationRequestId request_id =
        next_request_id_++;
    random_walker::domain::SegmentationRequest request(
        request_id,
        image_,
        seed_regions_);

    active_request_id_ = request_id;
    set_busy(true);
    set_error({});

    const std::shared_ptr<CompletionDeliveryGate> delivery_gate =
        completion_delivery_;
    segmentation_executor_.submit(
        std::move(request),
        [delivery_gate](
            random_walker::executor::SegmentationCompletion completion) {
            SegmentationViewModel::dispatch_completion(
                delivery_gate,
                std::move(completion));
        });
}

SegmentationViewModel::DomainSeedLabel
SegmentationViewModel::domain_seed_label() const noexcept
{
    return selected_label_ == Object
        ? DomainSeedLabel::Object
        : DomainSeedLabel::Background;
}

void SegmentationViewModel::dispatch_completion(
    const std::shared_ptr<CompletionDeliveryGate>& delivery_gate,
    random_walker::executor::SegmentationCompletion completion)
{
    auto payload =
        std::make_shared<random_walker::executor::SegmentationCompletion>(
            std::move(completion));

    std::lock_guard lock(delivery_gate->mutex);
    SegmentationViewModel* receiver = delivery_gate->receiver;
    if (!receiver) {
        return;
    }

    QMetaObject::invokeMethod(
        receiver,
        [receiver, payload] {
            receiver->handle_completion(std::move(*payload));
        },
        Qt::QueuedConnection);
}

void SegmentationViewModel::handle_completion(
    random_walker::executor::SegmentationCompletion completion)
{
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
}

void SegmentationViewModel::cancel_active_request()
{
    assert_ui_thread();

    if (!active_request_id_.has_value()) {
        return;
    }

    segmentation_executor_.cancel();
    active_request_id_.reset();
    set_busy(false);
}

void SegmentationViewModel::invalidate_result()
{
    assert_ui_thread();

    if (!result_.has_value()) {
        return;
    }

    result_.reset();
    result_image_cache_.clear();
    ++result_version_;
    emit result_changed();
}

void SegmentationViewModel::set_busy(bool value)
{
    assert_ui_thread();

    if (busy_ == value) {
        return;
    }

    const bool previous_can_run = can_run();
    busy_ = value;
    emit busy_changed();
    notify_can_run_if_changed(previous_can_run);
}

void SegmentationViewModel::set_error(QString message)
{
    assert_ui_thread();

    if (error_message_ == message) {
        return;
    }

    error_message_ = std::move(message);
    emit error_message_changed();
}

void SegmentationViewModel::notify_can_run_if_changed(bool previous_value)
{
    if (previous_value != can_run()) {
        emit can_run_changed();
    }
}

void SegmentationViewModel::assert_ui_thread() const
{
    Q_ASSERT(QThread::currentThread() == thread());
}

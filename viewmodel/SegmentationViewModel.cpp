#include "SegmentationViewModel.hpp"

#include <algorithm>
#include <span>
#include <utility>
#include <variant>
#include <vector>

#include <QImage>
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

SegmentationViewModel::SegmentationViewModel(
    const random_walker::service::SegmentationService& segmentation_service,
    BaseImageSink& base_image_sink,
    ResultImageSink& result_image_sink,
    QObject* parent)
    : QObject(parent)
    , segmentation_service_(segmentation_service)
    , base_image_sink_(base_image_sink)
    , result_image_sink_(result_image_sink)
    , seed_model_()
{
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
        && object_seed_count() > 0
        && !busy_;
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
    return seed_model_.pixel_count(
        random_walker::domain::SeedLabel::Background);
}

int SegmentationViewModel::object_seed_count() const noexcept
{
    return seed_model_.pixel_count(
        random_walker::domain::SeedLabel::Object);
}

void SegmentationViewModel::set_selected_label(int label)
{
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
    const QUrl url(path);
    const QString local_path = url.isLocalFile() ? url.toLocalFile() : path;
    const QImage loaded(local_path);

    if (loaded.isNull()) {
        set_error(QStringLiteral("Failed to load the selected image."));
        return;
    }

    const bool previous_can_run = can_run();
    const bool was_loaded = image_loaded();

    const QImage grayscale =
        random_walker::qt_adapter::to_grayscale(loaded);
    image_ = random_walker::qt_adapter::to_gray_image(grayscale);
    seed_model_.clear();
    invalidate_result();
    set_error({});

    base_image_sink_.set_image(grayscale);
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
    if (!image_loaded() && seed_model_.rowCount() == 0 && !has_result()
        && error_message_.isEmpty()) {
        return;
    }

    const bool previous_can_run = can_run();
    const bool was_loaded = image_loaded();

    image_ = {};
    seed_model_.clear();
    invalidate_result();
    set_error({});
    base_image_sink_.clear();
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
    if (seed_model_.rowCount() == 0) {
        return;
    }

    const bool previous_can_run = can_run();
    seed_model_.clear();
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

    seed_model_.append({
        .x = left,
        .y = top,
        .width = right - left,
        .height = bottom - top,
        .label = label
    });

    invalidate_result();
    set_error({});
    emit seeds_changed();
    notify_can_run_if_changed(previous_can_run);
}

void SegmentationViewModel::run_segmentation()
{
    if (busy_) {
        return;
    }

    set_busy(true);
    set_error({});

    const std::vector<random_walker::domain::Seed> seeds =
        seed_model_.expanded_seeds();
    random_walker::domain::SegmentationOutcome outcome =
        segmentation_service_.segment({
            image_,
            std::span<const random_walker::domain::Seed>(seeds)
        });

    if (auto* segmentation_result =
            std::get_if<random_walker::domain::SegmentationResult>(&outcome)) {
        result_ = std::move(*segmentation_result);
        result_image_sink_.set_image(
            random_walker::qt_adapter::render_binary_mask(result_->mask));
        ++result_version_;
        emit result_changed();
    } else {
        invalidate_result();
        set_error(::error_message(
            std::get<random_walker::domain::SegmentationError>(outcome)));
    }

    set_busy(false);
}

SegmentationViewModel::DomainSeedLabel
SegmentationViewModel::domain_seed_label() const noexcept
{
    return selected_label_ == Object
        ? DomainSeedLabel::Object
        : DomainSeedLabel::Background;
}

void SegmentationViewModel::invalidate_result()
{
    if (!result_.has_value()) {
        return;
    }

    result_.reset();
    result_image_sink_.clear();
    ++result_version_;
    emit result_changed();
}

void SegmentationViewModel::set_busy(bool value)
{
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

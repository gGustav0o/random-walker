#include "ImageState.hpp"

#include <QImage>

#include "presentation/adapter/QImageAdapter.hpp"
#include "presentation/qml/qml_names.hpp"

ImageState::ImageState(PresentationImageCache& cache) noexcept
    : cache_(cache) {
}

bool ImageState::loaded() const noexcept {
    return !image_.empty();
}

int ImageState::width() const noexcept {
    return image_.width();
}

int ImageState::height() const noexcept {
    return image_.height();
}

quint64 ImageState::version() const noexcept {
    return version_;
}

QString ImageState::source() const {
    return loaded()
        ? QStringLiteral("image://%1/processed").arg(
            qml_names::kBaseImageProvider
        )
        : QString();
}

const random_walker::domain::GrayImage& ImageState::image() const noexcept {
    return image_;
}

void ImageState::set(const QImage& image) {
    image_ = random_walker::qt_adapter::to_gray_image(image);
    cache_.store(random_walker::qt_adapter::to_qimage(image_));
    ++version_;
}

void ImageState::clear() {
    image_ = {};
    cache_.clear();
    ++version_;
}

#include "ResultImageProvider.hpp"

QImage ResultImageProvider::requestImage(
    const QString& id
    , QSize* size
    , const QSize& requested_size
) {
    Q_UNUSED(id)
    Q_UNUSED(requested_size)

    QMutexLocker locker(&mutex_);
    if (size) {
        *size = cached_image_.size();
    }
    return cached_image_;
}

void ResultImageProvider::store(const QImage& image) {
    QMutexLocker locker(&mutex_);
    cached_image_ = image;
}

void ResultImageProvider::clear() {
    QMutexLocker locker(&mutex_);
    cached_image_ = {};
}

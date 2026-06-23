#include "BaseImageProvider.hpp"

QImage BaseImageProvider::requestImage(
    const QString& id
    , QSize* size
    , const QSize& requested_size
) {
    Q_UNUSED(requested_size)

    QMutexLocker locker(&mutex_);
    QString real_id = id;
    const int query_index = real_id.indexOf("?");
    if (query_index != -1) {
        real_id = real_id.left(query_index);
    }

    if (real_id == "processed" && !cached_image_.isNull()) {
        if (size) {
            *size = cached_image_.size();
        }
        return cached_image_;
    }

    return QImage();
}

void BaseImageProvider::store(const QImage& image) {
    QMutexLocker locker(&mutex_);
    cached_image_ = image;
}

void BaseImageProvider::clear() {
    QMutexLocker locker(&mutex_);
    cached_image_ = QImage();
}

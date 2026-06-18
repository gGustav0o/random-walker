#include "BaseImageProvider.hpp"

QImage BaseImageProvider::requestImage(const QString & id, QSize * size, const QSize & requestedSize) {
    Q_UNUSED(requestedSize)
    QMutexLocker locker(&mutex_);
    QString real_id = id;
    int idx = real_id.indexOf("?");
    if (idx != -1)
        real_id = real_id.left(idx);
    if (real_id == "processed" && !image_.isNull()) {
        if (size) *size = image_.size();
        return image_;
    }
    return QImage();
}

void BaseImageProvider::set_image(const QImage& image) {
    QMutexLocker locker(&mutex_);
    image_ = image;
}

void BaseImageProvider::clear() {
    QMutexLocker locker(&mutex_);
    image_ = QImage();
}
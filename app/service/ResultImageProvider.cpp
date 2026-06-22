#include "ResultImageProvider.hpp"

QImage ResultImageProvider::requestImage(
    const QString& id,
    QSize* size,
    const QSize& requested_size)
{
    Q_UNUSED(id)
    Q_UNUSED(requested_size)

    QMutexLocker locker(&mutex_);
    if (size) {
        *size = image_.size();
    }
    return image_;
}

void ResultImageProvider::set_image(const QImage& image)
{
    QMutexLocker locker(&mutex_);
    image_ = image;
}

void ResultImageProvider::clear()
{
    QMutexLocker locker(&mutex_);
    image_ = {};
}

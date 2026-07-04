#pragma once

#include <QImage>
#include <QMutex>
#include <QQuickImageProvider>

#include "presentation/image/PresentationImageCache.hpp"

class AutoMarkerImageProvider final
    : public QQuickImageProvider
    , public PresentationImageCache
{

public:
    AutoMarkerImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Image) {}

    QImage requestImage(
        const QString& id
        , QSize* size
        , const QSize& requested_size
    ) override;

    void store(const QImage& image) override;
    void clear() override;

private:
    QImage cached_image_;
    QMutex mutex_;
};
#pragma once

#include <QQuickImageProvider>
#include <QImage>
#include <QMutex>

#include "presentation/image/PresentationImageCache.hpp"

class BaseImageProvider final
    : public QQuickImageProvider
    , public PresentationImageCache
{
public:
	BaseImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

	QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;
	void store(const QImage& image) override;
	void clear() override;
private:
	QImage cached_image_;
	QMutex mutex_;
};

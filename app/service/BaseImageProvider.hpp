#pragma once

#include <QQuickImageProvider>
#include <QImage>
#include <QMutex>

#include "app/service/BaseImageSink.hpp"

class BaseImageProvider final
    : public QQuickImageProvider
    , public BaseImageSink
{
public:
	BaseImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

	QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;
	void set_image(const QImage& image) override;
	void clear() override;
private:
	QImage image_;
	QMutex mutex_;
};

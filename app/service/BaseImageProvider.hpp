#pragma once

#include <QQuickImageProvider>
#include <QImage>
#include <QMutex>

class BaseImageProvider : public QQuickImageProvider {
public:
	BaseImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

	QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;
	void set_image(const QImage& image);
	void clear();
private:
	QImage image_;
	QMutex mutex_;
};
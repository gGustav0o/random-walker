#pragma once

#include <QImage>
#include <QMutex>
#include <QQuickImageProvider>

#include "app/service/ResultImageSink.hpp"

class ResultImageProvider final
    : public QQuickImageProvider
    , public ResultImageSink
{
public:
    ResultImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Image)
    {
    }

    QImage requestImage(
        const QString& id,
        QSize* size,
        const QSize& requested_size) override;
    void set_image(const QImage& image) override;
    void clear() override;

private:
    QImage image_;
    QMutex mutex_;
};

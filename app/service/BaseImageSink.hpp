#pragma once

#include <QImage>

class BaseImageSink
{
public:
    virtual ~BaseImageSink() = default;

    virtual void set_image(const QImage& image) = 0;
    virtual void clear() = 0;
};

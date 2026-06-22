#pragma once

#include <QImage>

class ResultImageSink
{
public:
    virtual ~ResultImageSink() = default;

    virtual void set_image(const QImage& image) = 0;
    virtual void clear() = 0;
};

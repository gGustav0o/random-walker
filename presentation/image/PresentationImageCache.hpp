#pragma once

#include <QImage>

class PresentationImageCache {
public:
    virtual ~PresentationImageCache() = default;

    virtual void store(const QImage& image) = 0;
    virtual void clear() = 0;
};

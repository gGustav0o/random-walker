#pragma once

#include <QString>
#include <QtGlobal>

#include "model/domain/GrayImage.hpp"
#include "presentation/image/PresentationImageCache.hpp"

class QImage;

class ImageState final {
public:
    explicit ImageState(PresentationImageCache& cache) noexcept;

    [[nodiscard]] bool loaded() const noexcept;
    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] quint64 version() const noexcept;
    [[nodiscard]] QString source() const;
    [[nodiscard]] const random_walker::domain::GrayImage& image()
        const noexcept;

    void set(const QImage& image);
    void clear();

private:
    PresentationImageCache& cache_;
    random_walker::domain::GrayImage image_;
    quint64 version_ = 0;
};

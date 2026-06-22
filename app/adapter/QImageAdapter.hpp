#pragma once

#include <utility>

#include <QImage>

#include "model/domain/GrayImage.hpp"

namespace random_walker::qt_adapter
{
    [[nodiscard]] inline domain::GrayImage to_gray_image(const QImage& image)
    {
        const QImage grayscale = image.convertToFormat(QImage::Format_Grayscale8);
        const int width = grayscale.width();
        const int height = grayscale.height();

        domain::GrayImageMatrix pixels(height, width);
        for (int row = 0; row < height; ++row) {
            const uchar* scan_line = grayscale.constScanLine(row);
            for (int column = 0; column < width; ++column) {
                pixels(row, column) = scan_line[column];
            }
        }

        return domain::GrayImage(std::move(pixels));
    }

    [[nodiscard]] inline QImage to_grayscale(const QImage& image)
    {
        return image.convertToFormat(QImage::Format_Grayscale8);
    }
}

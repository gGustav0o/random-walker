#pragma once

#include <utility>

#include <QImage>
#include <QtGlobal>

#include "model/domain/GrayImage.hpp"

namespace random_walker::qt_adapter {

    [[nodiscard]] inline domain::GrayImage to_gray_image(const QImage& image) {
        const QImage grayscale = image.convertToFormat(QImage::Format_Grayscale8);
        const int width = grayscale.width();
        const int height = grayscale.height();
        Q_ASSERT(width >= 0);
        Q_ASSERT(height >= 0);

        domain::GrayImageMatrix pixels(height, width);
        for (int row = 0; row < height; ++row) {
            const uchar* scan_line = grayscale.constScanLine(row);
            Q_ASSERT(scan_line != nullptr);
            for (int column = 0; column < width; ++column) {
                pixels(row, column) = scan_line[column];
            }
        }

        return domain::GrayImage(std::move(pixels));
    }

    [[nodiscard]] inline QImage to_qimage(const domain::GrayImage& image) {
        if (image.empty()) {
            return {};
        }

        QImage result(
            image.width()
            , image.height()
            , QImage::Format_Grayscale8
        );
        Q_ASSERT(result.width() == image.width());
        Q_ASSERT(result.height() == image.height());

        for (int row = 0; row < image.height(); ++row) {
            uchar* scan_line = result.scanLine(row);
            Q_ASSERT(scan_line != nullptr);
            for (int column = 0; column < image.width(); ++column) {
                scan_line[column] = image.at(row, column);
            }
        }

        return result;
    }
}

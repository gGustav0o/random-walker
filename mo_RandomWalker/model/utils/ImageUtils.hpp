#pragma once

#include <QImage>
#include <Eigen/Core>

#include "MetaAnnotations.hpp"

[[nodiscard]] inline auto image_to_matrix(const QImage& image) -> Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic>
{
    const int width = image.width();
    const int height = image.height();

    Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic> result(height, width);
    for (size_t y = 0; y < height; y++) {
        const uchar* scan_line = image.constScanLine(y);
        for (size_t x = 0; x < width; ++x) {
            result(y, x) = scan_line[x];
        }
    }
    return result;
}

_impure_(inline QImage to_grayscale(const QImage& src)) {
	return src.convertToFormat(QImage::Format_Grayscale8);
}

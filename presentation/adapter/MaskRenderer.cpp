#include "MaskRenderer.hpp"

#include <QColor>

namespace random_walker::qt_adapter {
    QImage render_binary_mask(const domain::BinaryMask& mask) {
        const QColor object_color(0, 255, 0, 100);
        const QColor background_color(255, 0, 255, 100);

        QImage result(
            static_cast<int>(mask.cols())
            , static_cast<int>(mask.rows())
            , QImage::Format_ARGB32
        );

        for (int row = 0; row < result.height(); ++row) {
            for (int column = 0; column < result.width(); ++column) {
                result.setPixelColor(
                    column
                    , row
                    , mask(row, column) == 1
                        ? object_color
                        : background_color
                );
            }
        }

        return result;
    }
}

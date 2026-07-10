#include "MaskRenderer.hpp"

#include <Eigen/Core>

#include <QColor>
#include <QtGlobal>

namespace random_walker::qt_adapter {
    QImage render_binary_mask(const domain::BinaryMask& mask) {
        Q_ASSERT(mask.rows() >= 0);
        Q_ASSERT(mask.cols() >= 0);

        const QColor object_color(0, 255, 0, 60);
        const QColor background_color(255, 0, 255, 60);

        QImage result(
            static_cast<int>(mask.cols())
            , static_cast<int>(mask.rows())
            , QImage::Format_ARGB32
        );
        Q_ASSERT(static_cast<Eigen::Index>(result.width()) == mask.cols());
        Q_ASSERT(static_cast<Eigen::Index>(result.height()) == mask.rows());

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

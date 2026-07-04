#include <QColor>
#include <QtTest>

#include "model/domain/AutoMarkers.hpp"
#include "presentation/adapter/AutoMarkerMaskRenderer.hpp"

namespace domain = random_walker::domain;
namespace qt_adapter = random_walker::qt_adapter;

class AutoMarkerMaskRendererTests final : public QObject {
    Q_OBJECT

private slots:
    void renders_transparent_background_and_colored_markers();
};

void AutoMarkerMaskRendererTests::renders_transparent_background_and_colored_markers() {
    domain::MarkerLabelMask mask(3, 1);
    mask.set(0, 1, domain::MarkerLabel::Background);
    mask.set(0, 2, domain::MarkerLabel::Object);

    const QImage image = qt_adapter::render_marker_label_mask(mask);

    QCOMPARE(image.width(), 3);
    QCOMPARE(image.height(), 1);
    QCOMPARE(QColor(image.pixelColor(0, 0)).alpha(), 0);
    QVERIFY(QColor(image.pixelColor(1, 0)).alpha() > 0);
    QVERIFY(QColor(image.pixelColor(2, 0)).alpha() > 0);
    QVERIFY(QColor(image.pixelColor(1, 0)) != QColor(image.pixelColor(2, 0)));
}

QTEST_GUILESS_MAIN(AutoMarkerMaskRendererTests)
#include "AutoMarkerMaskRendererTests.moc"

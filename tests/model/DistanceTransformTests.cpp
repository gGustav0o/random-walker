#include <cmath>
#include <cstdint>
#include <vector>

#include <QtTest>

#include "model/algorithm/DistanceTransform.hpp"

namespace algorithm = random_walker::algorithm;

class DistanceTransformTests final : public QObject {
    Q_OBJECT

private slots:
    void computes_squared_euclidean_distances_from_single_source();
    void computes_squared_euclidean_distances_from_multiple_sources();
    void returns_infinity_when_source_mask_is_empty();
    void returns_zero_when_every_pixel_is_source();
    void returns_empty_distances_for_empty_image();
};

void DistanceTransformTests::computes_squared_euclidean_distances_from_single_source() {
    const std::vector<std::uint8_t> source_mask {
        0, 0, 0,
        0, 1, 0,
        0, 0, 0
    };

    const std::vector<double> distances =
        algorithm::squared_euclidean_distance_transform(3, 3, source_mask);

    const std::vector<double> expected {
        2.0, 1.0, 2.0,
        1.0, 0.0, 1.0,
        2.0, 1.0, 2.0
    };
    QCOMPARE(distances.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        QCOMPARE(distances[index], expected[index]);
    }
}

void DistanceTransformTests::computes_squared_euclidean_distances_from_multiple_sources() {
    const std::vector<std::uint8_t> source_mask {
        1, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 1
    };

    const std::vector<double> distances =
        algorithm::squared_euclidean_distance_transform(4, 3, source_mask);

    const std::vector<double> expected {
        0.0, 1.0, 4.0, 4.0,
        1.0, 2.0, 2.0, 1.0,
        4.0, 4.0, 1.0, 0.0
    };
    QCOMPARE(distances, expected);
}

void DistanceTransformTests::returns_infinity_when_source_mask_is_empty() {
    const std::vector<std::uint8_t> source_mask(4U, 0U);

    const std::vector<double> distances =
        algorithm::squared_euclidean_distance_transform(2, 2, source_mask);

    QCOMPARE(distances.size(), std::size_t {4});
    for (const double distance : distances) {
        QVERIFY(std::isinf(distance));
    }
}

void DistanceTransformTests::returns_zero_when_every_pixel_is_source() {
    const std::vector<std::uint8_t> source_mask(4U, 1U);

    const std::vector<double> distances =
        algorithm::squared_euclidean_distance_transform(2, 2, source_mask);

    QCOMPARE(distances, std::vector<double>({0.0, 0.0, 0.0, 0.0}));
}

void DistanceTransformTests::returns_empty_distances_for_empty_image() {
    const std::vector<std::uint8_t> source_mask;

    const std::vector<double> distances =
        algorithm::squared_euclidean_distance_transform(0, 0, source_mask);

    QVERIFY(distances.empty());
}

QTEST_GUILESS_MAIN(DistanceTransformTests)
#include "DistanceTransformTests.moc"

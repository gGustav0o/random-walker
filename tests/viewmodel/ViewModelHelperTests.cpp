#include <limits>
#include <optional>
#include <vector>

#include <QCoreApplication>
#include <QEventLoop>
#include <QtTest>

#include "model/domain/AutoMarkers.hpp"
#include "model/domain/RandomWalkerParameters.hpp"
#include "model/executor/SegmentationExecutor.hpp"
#include "viewmodel/SeedRegionGeometry.hpp"
#include "viewmodel/SegmentationConstraintsBuilder.hpp"
#include "viewmodel/SegmentationViewModel.hpp"
#include "viewmodel/ViewModelCallbackGate.hpp"
#include "viewmodel/ViewModelConversions.hpp"

namespace domain = random_walker::domain;
namespace executor = random_walker::executor;
namespace viewmodel = random_walker::viewmodel;

namespace {
    class CallbackReceiver final : public QObject {
        Q_OBJECT

    public:
        void handle_value(int value) {
            last_value = value;
            ++handled_count;
        }

        int handled_count = 0;
        int last_value = 0;
    };

    void process_queued_callbacks() {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
    }

    [[nodiscard]] domain::SeedRegion seed_region(
        int x
        , int y
        , int width
        , int height
        , domain::SeedLabel label
    ) {
        return {
            .area = {
                .x = x
                , .y = y
                , .width = width
                , .height = height
            }
            , .label = label
        };
    }
}

class ViewModelHelperTests final : public QObject {
    Q_OBJECT

private slots:
    void maps_connectivity_between_domain_and_view();
    void rejects_unknown_view_connectivity();
    void maps_foreground_polarity_between_domain_and_view();
    void rejects_unknown_view_foreground_polarity();
    void maps_executor_error_to_application_error();
    void clips_seed_rectangle_to_image_bounds();
    void rejects_empty_or_outside_seed_rectangle();
    void clips_seed_rectangle_without_int_overflow();
    void builds_segmentation_constraints_from_seed_regions_and_marker_mask();
    void counts_manual_and_automatic_constraints();
    void checks_required_constraint_presence();
    void callback_gate_posts_payload_to_attached_receiver();
    void callback_gate_drops_payload_after_detach();
};

void ViewModelHelperTests::maps_connectivity_between_domain_and_view() {
    QCOMPARE(
        viewmodel::view_connectivity(domain::PixelConnectivity::Four)
        , static_cast<int>(SegmentationViewModel::FourConnectivity)
    );
    QCOMPARE(
        viewmodel::view_connectivity(domain::PixelConnectivity::Eight)
        , static_cast<int>(SegmentationViewModel::EightConnectivity)
    );

    const std::optional<domain::PixelConnectivity> four_connectivity =
        viewmodel::domain_connectivity(
            SegmentationViewModel::FourConnectivity
        );
    const std::optional<domain::PixelConnectivity> eight_connectivity =
        viewmodel::domain_connectivity(
            SegmentationViewModel::EightConnectivity
        );
    QVERIFY(four_connectivity.has_value());
    QVERIFY(eight_connectivity.has_value());
    QCOMPARE(
        static_cast<int>(*four_connectivity)
        , static_cast<int>(domain::PixelConnectivity::Four)
    );
    QCOMPARE(
        static_cast<int>(*eight_connectivity)
        , static_cast<int>(domain::PixelConnectivity::Eight)
    );
}

void ViewModelHelperTests::rejects_unknown_view_connectivity() {
    QVERIFY(!viewmodel::domain_connectivity(-1).has_value());
    QVERIFY(!viewmodel::domain_connectivity(42).has_value());
}

void ViewModelHelperTests::maps_foreground_polarity_between_domain_and_view() {
    QCOMPARE(
        viewmodel::view_foreground_polarity(
            domain::ForegroundPolarity::DarkObject
        )
        , static_cast<int>(SegmentationViewModel::DarkObjectForeground)
    );
    QCOMPARE(
        viewmodel::view_foreground_polarity(
            domain::ForegroundPolarity::BrightObject
        )
        , static_cast<int>(SegmentationViewModel::BrightObjectForeground)
    );

    const std::optional<domain::ForegroundPolarity> dark_object =
        viewmodel::domain_foreground_polarity(
            SegmentationViewModel::DarkObjectForeground
        );
    const std::optional<domain::ForegroundPolarity> bright_object =
        viewmodel::domain_foreground_polarity(
            SegmentationViewModel::BrightObjectForeground
        );
    QVERIFY(dark_object.has_value());
    QVERIFY(bright_object.has_value());
    QCOMPARE(
        static_cast<int>(*dark_object)
        , static_cast<int>(domain::ForegroundPolarity::DarkObject)
    );
    QCOMPARE(
        static_cast<int>(*bright_object)
        , static_cast<int>(domain::ForegroundPolarity::BrightObject)
    );
}

void ViewModelHelperTests::rejects_unknown_view_foreground_polarity() {
    QVERIFY(!viewmodel::domain_foreground_polarity(-1).has_value());
    QVERIFY(!viewmodel::domain_foreground_polarity(42).has_value());
}

void ViewModelHelperTests::maps_executor_error_to_application_error() {
    QCOMPARE(
        static_cast<int>(viewmodel::application_error(
            executor::ExecutionError::UnexpectedInternalFailure
        ))
        , static_cast<int>(
            random_walker::application::ApplicationError::
                UnexpectedInternalFailure
        )
    );
}

void ViewModelHelperTests::clips_seed_rectangle_to_image_bounds() {
    const std::optional<domain::PixelRectangle> rectangle =
        viewmodel::clipped_seed_rectangle(-2, 1, 5, 4, 4, 3);
    const domain::PixelRectangle expected {
        .x = 0
        , .y = 1
        , .width = 3
        , .height = 2
    };

    QVERIFY(rectangle.has_value());
    QVERIFY(*rectangle == expected);
}

void ViewModelHelperTests::rejects_empty_or_outside_seed_rectangle() {
    QVERIFY(!viewmodel::clipped_seed_rectangle(
        0
        , 0
        , 0
        , 1
        , 4
        , 3
    ).has_value());
    QVERIFY(!viewmodel::clipped_seed_rectangle(
        5
        , 0
        , 2
        , 2
        , 4
        , 3
    ).has_value());
}

void ViewModelHelperTests::clips_seed_rectangle_without_int_overflow() {
    const std::optional<domain::PixelRectangle> rectangle =
        viewmodel::clipped_seed_rectangle(
            2
            , 1
            , std::numeric_limits<int>::max()
            , std::numeric_limits<int>::max()
            , 4
            , 3
        );
    const domain::PixelRectangle expected {
        .x = 2
        , .y = 1
        , .width = 2
        , .height = 2
    };

    QVERIFY(rectangle.has_value());
    QVERIFY(*rectangle == expected);
}

void ViewModelHelperTests::
builds_segmentation_constraints_from_seed_regions_and_marker_mask() {
    const std::vector<domain::SeedRegion> manual_regions {
        seed_region(0, 0, 2, 1, domain::SeedLabel::Background)
    };
    domain::MarkerLabelMask automatic_markers(2, 1);
    automatic_markers.set(0, 1, domain::MarkerLabel::Object);

    const domain::SegmentationConstraints constraints =
        viewmodel::make_segmentation_constraints(
            manual_regions
            , &automatic_markers
        );

    QCOMPARE(constraints.manual_seed_regions.size(), std::size_t {1});
    QVERIFY(constraints.manual_seed_regions.front() == manual_regions.front());
    QCOMPARE(
        static_cast<int>(constraints.automatic_markers.at(0, 1))
        , static_cast<int>(domain::MarkerLabel::Object)
    );
}

void ViewModelHelperTests::counts_manual_and_automatic_constraints() {
    const std::vector<domain::SeedRegion> manual_regions {
        seed_region(0, 0, 2, 2, domain::SeedLabel::Background),
        seed_region(3, 0, 1, 2, domain::SeedLabel::Object)
    };
    domain::MarkerLabelMask automatic_markers(4, 1);
    automatic_markers.set(0, 0, domain::MarkerLabel::Background);
    automatic_markers.set(0, 1, domain::MarkerLabel::Object);
    automatic_markers.set(0, 2, domain::MarkerLabel::Object);

    const viewmodel::ConstraintCounts counts = viewmodel::count_constraints(
        manual_regions
        , &automatic_markers
    );

    QCOMPARE(counts.background, 5);
    QCOMPARE(counts.object, 4);
}

void ViewModelHelperTests::checks_required_constraint_presence() {
    QVERIFY(!viewmodel::has_required_constraints({
        .background = 1
        , .object = 0
    }));
    QVERIFY(!viewmodel::has_required_constraints({
        .background = 0
        , .object = 1
    }));
    QVERIFY(viewmodel::has_required_constraints({
        .background = 1
        , .object = 1
    }));
}

void ViewModelHelperTests::callback_gate_posts_payload_to_attached_receiver() {
    viewmodel::ViewModelCallbackGate gate;
    CallbackReceiver receiver;
    gate.attach(receiver);

    gate.post<CallbackReceiver>(42, &CallbackReceiver::handle_value);
    process_queued_callbacks();

    QCOMPARE(receiver.handled_count, 1);
    QCOMPARE(receiver.last_value, 42);
}

void ViewModelHelperTests::callback_gate_drops_payload_after_detach() {
    viewmodel::ViewModelCallbackGate gate;
    CallbackReceiver receiver;
    gate.attach(receiver);
    gate.detach();

    gate.post<CallbackReceiver>(42, &CallbackReceiver::handle_value);
    process_queued_callbacks();

    QCOMPARE(receiver.handled_count, 0);
}

QTEST_GUILESS_MAIN(ViewModelHelperTests)
#include "ViewModelHelperTests.moc"

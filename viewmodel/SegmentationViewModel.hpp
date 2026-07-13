#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "application/markers/AutoMarkerExecutor.hpp"
#include "application/settings/SettingsService.hpp"
#include "presentation/image/PresentationImageCache.hpp"
#include "model/domain/Segmentation.hpp"
#include "model/executor/SegmentationExecutor.hpp"
#include "viewmodel/AutomaticMarkerState.hpp"
#include "viewmodel/ErrorState.hpp"
#include "viewmodel/ImageState.hpp"
#include "viewmodel/ProgressState.hpp"
#include "viewmodel/ResultState.hpp"
#include "viewmodel/SeedListModel.hpp"
#include "viewmodel/SeedRegionState.hpp"

namespace random_walker::viewmodel {
    class ViewModelCallbackGate;
}

// QML-facing facade for segmentation workflows. Domain rules stay in the
// domain/application layers; pure presentation mappings live outside this
// class. This type coordinates UI state, Qt signals, and asynchronous request
// lifetimes.
class SegmentationViewModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString image_source           READ image_source           NOTIFY image_source_changed)
    Q_PROPERTY(bool    image_loaded           READ image_loaded           NOTIFY image_loaded_changed)
    Q_PROPERTY(int     image_height           READ image_height           NOTIFY image_dimensions_changed)
    Q_PROPERTY(int     image_width            READ image_width            NOTIFY image_dimensions_changed)
    Q_PROPERTY(quint64 image_version          READ image_version          NOTIFY image_version_changed)
    Q_PROPERTY(bool    can_run                READ can_run                NOTIFY can_run_changed)
    Q_PROPERTY(bool    busy                   READ busy                   NOTIFY busy_changed)
    Q_PROPERTY(int     progress_stage         READ progress_stage         NOTIFY progress_changed)
    Q_PROPERTY(double  progress_fraction      READ progress_fraction      NOTIFY progress_changed)
    Q_PROPERTY(bool    progress_indeterminate READ progress_indeterminate NOTIFY progress_changed)
    Q_PROPERTY(QString status_text            READ status_text            NOTIFY progress_changed)
    Q_PROPERTY(bool    has_result             READ has_result             NOTIFY result_changed)
    Q_PROPERTY(QString result_source          READ result_source          NOTIFY result_changed)
    Q_PROPERTY(quint64 result_version         READ result_version         NOTIFY result_changed)
    Q_PROPERTY(bool    has_automatic_markers  READ has_automatic_markers  NOTIFY automatic_markers_changed)
    Q_PROPERTY(QString automatic_marker_source READ automatic_marker_source NOTIFY automatic_markers_changed)
    Q_PROPERTY(quint64 automatic_marker_version READ automatic_marker_version NOTIFY automatic_markers_changed)
    Q_PROPERTY(QString error_message          READ error_message          NOTIFY error_message_changed)
    Q_PROPERTY(int     background_seed_count  READ background_seed_count  NOTIFY seeds_changed)
    Q_PROPERTY(int     object_seed_count      READ object_seed_count      NOTIFY seeds_changed)
    Q_PROPERTY(int     automatic_marker_count READ automatic_marker_count NOTIFY seeds_changed)

    Q_PROPERTY(double  beta                   READ beta                 WRITE set_beta                 NOTIFY beta_changed)
    Q_PROPERTY(double  beta_slider_position   READ beta_slider_position WRITE set_beta_slider_position NOTIFY beta_changed)
    Q_PROPERTY(int     connectivity           READ connectivity         WRITE set_connectivity         NOTIFY connectivity_changed)
    Q_PROPERTY(double  distance_power         READ distance_power       WRITE set_distance_power       NOTIFY distance_power_changed)
    Q_PROPERTY(double  auto_marker_confidence_threshold READ auto_marker_confidence_threshold WRITE set_auto_marker_confidence_threshold NOTIFY auto_marker_confidence_threshold_changed)
    Q_PROPERTY(int     auto_marker_minimum_boundary_distance READ auto_marker_minimum_boundary_distance WRITE set_auto_marker_minimum_boundary_distance NOTIFY auto_marker_minimum_boundary_distance_changed)
    Q_PROPERTY(int     auto_marker_minimum_component_area READ auto_marker_minimum_component_area WRITE set_auto_marker_minimum_component_area NOTIFY auto_marker_minimum_component_area_changed)
    Q_PROPERTY(int     auto_marker_foreground_polarity READ auto_marker_foreground_polarity WRITE set_auto_marker_foreground_polarity NOTIFY auto_marker_foreground_polarity_changed)
    Q_PROPERTY(double  minimum_auto_marker_confidence_threshold READ minimum_auto_marker_confidence_threshold CONSTANT)
    Q_PROPERTY(double  maximum_auto_marker_confidence_threshold READ maximum_auto_marker_confidence_threshold CONSTANT)
    Q_PROPERTY(int     minimum_auto_marker_boundary_distance READ minimum_auto_marker_boundary_distance CONSTANT)
    Q_PROPERTY(int     maximum_auto_marker_boundary_distance READ maximum_auto_marker_boundary_distance CONSTANT)
    Q_PROPERTY(int     minimum_auto_marker_component_area READ minimum_auto_marker_component_area CONSTANT)
    Q_PROPERTY(int     maximum_auto_marker_component_area READ maximum_auto_marker_component_area CONSTANT)
    Q_PROPERTY(int     selected_label         READ selected_label       WRITE set_selected_label       NOTIFY selected_label_changed)

    Q_PROPERTY(QAbstractItemModel* seed_model READ seed_model CONSTANT)

public:
    enum SeedLabel {
        Background = 0
        , Object   = 1
    };
    Q_ENUM(SeedLabel)


    enum GraphConnectivity {
        FourConnectivity = 0
        , EightConnectivity = 1
    };
    Q_ENUM(GraphConnectivity)

    enum AutoMarkerForegroundPolarity {
        DarkObjectForeground = 0
        , BrightObjectForeground = 1
    };
    Q_ENUM(AutoMarkerForegroundPolarity)



    enum ProgressStage {
        Idle                         = ProgressState::Idle
        , ValidatingInput            = ProgressState::ValidatingInput
        , ExpandingSeeds             = ProgressState::ExpandingSeeds
        , BuildingGraph              = ProgressState::BuildingGraph
        , BuildingBoundaryConditions = ProgressState::BuildingBoundaryConditions
        , PartitioningSystem         = ProgressState::PartitioningSystem
        , Factorizing                = ProgressState::Factorizing
        , Solving                    = ProgressState::Solving
        , AssemblingProbabilities    = ProgressState::AssemblingProbabilities
        , Thresholding               = ProgressState::Thresholding
    };
    Q_ENUM(ProgressStage)

    explicit SegmentationViewModel(
        random_walker::executor::SegmentationExecutor& segmentation_executor
        , random_walker::application::SettingsService& settings_service
        , random_walker::application::AutoMarkerExecutor& auto_marker_executor
        , PresentationImageCache& base_image_cache
        , PresentationImageCache& auto_marker_image_cache
        , PresentationImageCache& result_image_cache
        , QObject* parent = nullptr
    );
    ~SegmentationViewModel() override;

    [[nodiscard]] QString image_source() const;
    [[nodiscard]] bool image_loaded() const noexcept;
    [[nodiscard]] int image_width() const noexcept;
    [[nodiscard]] int image_height() const noexcept;
    [[nodiscard]] quint64 image_version() const noexcept;
    [[nodiscard]] bool can_run() const noexcept;
    [[nodiscard]] bool busy() const noexcept;
    [[nodiscard]] int progress_stage() const noexcept;
    [[nodiscard]] double progress_fraction() const noexcept;
    [[nodiscard]] bool progress_indeterminate() const noexcept;
    [[nodiscard]] QString status_text() const;
    [[nodiscard]] double beta() const noexcept;
    [[nodiscard]] double beta_slider_position() const noexcept;
    [[nodiscard]] int connectivity() const noexcept;
    [[nodiscard]] double distance_power() const noexcept;
    [[nodiscard]] double auto_marker_confidence_threshold() const noexcept;
    [[nodiscard]] int auto_marker_minimum_boundary_distance() const noexcept;
    [[nodiscard]] int auto_marker_minimum_component_area() const noexcept;
    [[nodiscard]] int auto_marker_foreground_polarity() const noexcept;
    [[nodiscard]] double minimum_auto_marker_confidence_threshold() const noexcept;
    [[nodiscard]] double maximum_auto_marker_confidence_threshold() const noexcept;
    [[nodiscard]] int minimum_auto_marker_boundary_distance() const noexcept;
    [[nodiscard]] int maximum_auto_marker_boundary_distance() const noexcept;
    [[nodiscard]] int minimum_auto_marker_component_area() const noexcept;
    [[nodiscard]] int maximum_auto_marker_component_area() const noexcept;
    [[nodiscard]] bool has_result() const noexcept;
    [[nodiscard]] QString result_source() const;
    [[nodiscard]] quint64 result_version() const noexcept;
    [[nodiscard]] bool has_automatic_markers() const noexcept;
    [[nodiscard]] QString automatic_marker_source() const;
    [[nodiscard]] quint64 automatic_marker_version() const noexcept;
    [[nodiscard]] QAbstractItemModel* seed_model() noexcept;
    [[nodiscard]] int selected_label() const noexcept;
    [[nodiscard]] QString error_message() const;
    [[nodiscard]] int background_seed_count() const noexcept;
    [[nodiscard]] int object_seed_count() const noexcept;
    [[nodiscard]] int automatic_marker_count() const noexcept;
    void set_selected_label(int label);
    void set_beta(double value);
    void set_beta_slider_position(double position);
    void set_connectivity(int connectivity);
    void set_distance_power(double value);
    void set_auto_marker_confidence_threshold(double value);
    void set_auto_marker_minimum_boundary_distance(int value);
    void set_auto_marker_minimum_component_area(int value);
    void set_auto_marker_foreground_polarity(int polarity);

    Q_INVOKABLE void open_image(const QString& path);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void clear_seeds();
    Q_INVOKABLE void clear_automatic_markers();
    Q_INVOKABLE void propose_markers();
    Q_INVOKABLE void add_seed_rectangle(int x, int y, int width, int height);
    Q_INVOKABLE void run_segmentation();

signals:
    void image_source_changed();
    void image_loaded_changed(bool loaded);
    void image_dimensions_changed();
    void image_version_changed();
    void can_run_changed();
    void busy_changed();
    void progress_changed();
    void beta_changed();
    void connectivity_changed();
    void distance_power_changed();
    void auto_marker_confidence_threshold_changed();
    void auto_marker_minimum_boundary_distance_changed();
    void auto_marker_minimum_component_area_changed();
    void auto_marker_foreground_polarity_changed();
    void result_changed();
    void automatic_markers_changed();
    void selected_label_changed();
    void error_message_changed();
    void seeds_changed();

private:
    using DomainSeedLabel = random_walker::domain::SeedLabel;
    using DomainConnectivity = random_walker::domain::PixelConnectivity;
    using DomainForegroundPolarity = random_walker::domain::ForegroundPolarity;

    [[nodiscard]] DomainSeedLabel domain_seed_label() const noexcept;
    [[nodiscard]] int background_constraint_count() const noexcept;
    [[nodiscard]] int object_constraint_count() const noexcept;
    [[nodiscard]] std::vector<random_walker::domain::SeedRegion>
    segmentation_seed_regions() const;

    void update_random_walker_parameters(
        random_walker::domain::RandomWalkerParameters parameters
    );
    void update_auto_marker_parameters(
        random_walker::domain::AutoMarkerParameters parameters
    );

    void handle_completion(
        random_walker::executor::SegmentationCompletion completion
    );
    void handle_auto_marker_completion(
        random_walker::application::AutoMarkerCompletion completion
    );
    void handle_progress(
        random_walker::domain::SegmentationProgress progress
    );
    void assert_ui_thread() const;
    void cancel_active_request();
    void cancel_active_auto_marker_request();
    void clear_seed_regions();
    void add_seed_region(random_walker::domain::SeedRegion region);
    void reset_progress();
    void invalidate_result();
    void set_busy(bool value);
    void set_error(random_walker::application::UserError error);
    void clear_error();
    void notify_can_run_if_changed(bool previous_value);

    random_walker::executor::SegmentationExecutor& segmentation_executor_;
    random_walker::application::SettingsService& settings_service_;
    random_walker::application::AutoMarkerExecutor& auto_marker_executor_;
    ImageState image_state_;
    ResultState result_state_;
    AutomaticMarkerState automatic_marker_state_;
    SeedRegionState seed_state_;
    SeedListModel seed_model_;
    ErrorState error_state_;
    ProgressState progress_state_;
    random_walker::domain::SegmentationRequestId next_request_id_ = 1;
    random_walker::application::AutoMarkerRequestId
        next_auto_marker_request_id_ = 1;
    std::optional<random_walker::domain::SegmentationRequestId>
        active_request_id_;
    std::optional<random_walker::application::AutoMarkerRequestId>
        active_auto_marker_request_id_;
    std::shared_ptr<random_walker::viewmodel::ViewModelCallbackGate>
        callback_gate_;
    int selected_label_ = Background;
    random_walker::application::ApplicationSettings application_settings_;
    bool busy_ = false;
};

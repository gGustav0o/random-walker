#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "app/service/PresentationImageCache.hpp"
#include "model/domain/Segmentation.hpp"
#include "model/executor/SegmentationExecutor.hpp"
#include "viewmodel/SeedListModel.hpp"

class SegmentationViewModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString image_source READ image_source NOTIFY image_source_changed)
    Q_PROPERTY(bool image_loaded READ image_loaded NOTIFY image_loaded_changed)
    Q_PROPERTY(int image_width READ image_width NOTIFY image_dimensions_changed)
    Q_PROPERTY(int image_height READ image_height NOTIFY image_dimensions_changed)
    Q_PROPERTY(quint64 image_version READ image_version NOTIFY image_version_changed)
    Q_PROPERTY(bool can_run READ can_run NOTIFY can_run_changed)
    Q_PROPERTY(bool busy READ busy NOTIFY busy_changed)
    Q_PROPERTY(bool has_result READ has_result NOTIFY result_changed)
    Q_PROPERTY(QString result_source READ result_source NOTIFY result_changed)
    Q_PROPERTY(quint64 result_version READ result_version NOTIFY result_changed)
    Q_PROPERTY(QAbstractItemModel* seed_model READ seed_model CONSTANT)
    Q_PROPERTY(int selected_label READ selected_label WRITE set_selected_label NOTIFY selected_label_changed)
    Q_PROPERTY(QString error_message READ error_message NOTIFY error_message_changed)
    Q_PROPERTY(int background_seed_count READ background_seed_count NOTIFY seeds_changed)
    Q_PROPERTY(int object_seed_count READ object_seed_count NOTIFY seeds_changed)

public:
    enum SeedLabel
    {
        Background = 0,
        Object = 1
    };
    Q_ENUM(SeedLabel)

    explicit SegmentationViewModel(
        random_walker::executor::SegmentationExecutor& segmentation_executor,
        PresentationImageCache& base_image_cache,
        PresentationImageCache& result_image_cache,
        QObject* parent = nullptr);
    ~SegmentationViewModel() override;

    [[nodiscard]] QString image_source() const;
    [[nodiscard]] bool image_loaded() const noexcept;
    [[nodiscard]] int image_width() const noexcept;
    [[nodiscard]] int image_height() const noexcept;
    [[nodiscard]] quint64 image_version() const noexcept;
    [[nodiscard]] bool can_run() const noexcept;
    [[nodiscard]] bool busy() const noexcept;
    [[nodiscard]] bool has_result() const noexcept;
    [[nodiscard]] QString result_source() const;
    [[nodiscard]] quint64 result_version() const noexcept;
    [[nodiscard]] QAbstractItemModel* seed_model() noexcept;
    [[nodiscard]] int selected_label() const noexcept;
    [[nodiscard]] QString error_message() const;
    [[nodiscard]] int background_seed_count() const noexcept;
    [[nodiscard]] int object_seed_count() const noexcept;
    void set_selected_label(int label);

    Q_INVOKABLE void open_image(const QString& path);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void clear_seeds();
    Q_INVOKABLE void add_seed_rectangle(int x, int y, int width, int height);
    Q_INVOKABLE void run_segmentation();

signals:
    void image_source_changed();
    void image_loaded_changed(bool loaded);
    void image_dimensions_changed();
    void image_version_changed();
    void can_run_changed();
    void busy_changed();
    void result_changed();
    void selected_label_changed();
    void error_message_changed();
    void seeds_changed();

private:
    using DomainSeedLabel = random_walker::domain::SeedLabel;
    struct CompletionDeliveryGate;

    [[nodiscard]] DomainSeedLabel domain_seed_label() const noexcept;
    static void dispatch_completion(
        const std::shared_ptr<CompletionDeliveryGate>& delivery_gate,
        random_walker::executor::SegmentationCompletion completion);
    void handle_completion(
        random_walker::executor::SegmentationCompletion completion);
    void assert_ui_thread() const;
    void cancel_active_request();
    void invalidate_result();
    void set_busy(bool value);
    void set_error(QString message);
    void notify_can_run_if_changed(bool previous_value);

    random_walker::executor::SegmentationExecutor& segmentation_executor_;
    PresentationImageCache& base_image_cache_;
    PresentationImageCache& result_image_cache_;
    random_walker::domain::GrayImage image_;
    std::vector<random_walker::domain::SeedRegion> seed_regions_;
    SeedListModel seed_model_;
    std::optional<random_walker::domain::SegmentationResult> result_;
    QString error_message_;
    quint64 image_version_ = 0;
    quint64 result_version_ = 0;
    random_walker::domain::SegmentationRequestId next_request_id_ = 1;
    std::optional<random_walker::domain::SegmentationRequestId>
        active_request_id_;
    std::shared_ptr<CompletionDeliveryGate> completion_delivery_;
    int selected_label_ = Background;
    bool busy_ = false;
};

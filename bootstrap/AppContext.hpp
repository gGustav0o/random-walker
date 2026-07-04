#pragma once

#include <memory>

#include <QQmlApplicationEngine>

#include "application/markers/AutoMarkerService.hpp"
#include "application/settings/SettingsService.hpp"
#include "infrastructure/settings/QtSettingsRepository.hpp"
#include "model/executor/JThreadSegmentationExecutor.hpp"
#include "viewmodel/SegmentationViewModel.hpp"

class AppContext final {
public:
    explicit AppContext(QQmlApplicationEngine& engine);
    ~AppContext() = default;

private:
    // Dependencies are declared before their consumers. Reverse destruction
    // therefore guarantees:
    // ViewModel -> SettingsService -> SettingsRepository.

    random_walker::executor::JThreadSegmentationExecutor
        segmentation_executor_;

    random_walker::infrastructure::QtSettingsRepository
        settings_repository_;

    random_walker::application::SettingsService settings_service_;

    random_walker::application::AutoMarkerService auto_marker_service_;

    std::unique_ptr<SegmentationViewModel> segmentation_view_model_;
};

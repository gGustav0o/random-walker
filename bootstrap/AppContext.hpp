#pragma once

#include <memory>

#include <QQmlApplicationEngine>

#include "application/settings/SettingsService.hpp"
#include "infrastructure/execution/JThreadAutoMarkerExecutor.hpp"
#include "infrastructure/execution/JThreadSegmentationExecutor.hpp"
#include "infrastructure/settings/QtSettingsRepository.hpp"
#include "viewmodel/SegmentationViewModel.hpp"

class AppContext final {
public:
    explicit AppContext(QQmlApplicationEngine& engine);
    ~AppContext() = default;

private:
    // Dependencies are declared before their consumers. Reverse destruction
    // therefore guarantees that ViewModel releases executor callbacks and
    // services before their backing dependencies are destroyed.

    random_walker::infrastructure::JThreadSegmentationExecutor
        segmentation_executor_;

    random_walker::infrastructure::QtSettingsRepository
        settings_repository_;

    random_walker::application::SettingsService settings_service_;

    random_walker::infrastructure::JThreadAutoMarkerExecutor
        auto_marker_executor_;

    std::unique_ptr<SegmentationViewModel> segmentation_view_model_;
};

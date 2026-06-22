#pragma once

#include <memory>

#include <QQmlApplicationEngine>

#include "model/service/SegmentationService.hpp"
#include "viewmodel/SegmentationViewModel.hpp"

class AppContext final
{
public:
    explicit AppContext(QQmlApplicationEngine& engine);

private:
    random_walker::service::SegmentationService segmentation_service_;
    std::unique_ptr<SegmentationViewModel> segmentation_view_model_;
};

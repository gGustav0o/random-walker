#pragma once

#include <memory>

#include <QQmlApplicationEngine>

#include "model/executor/JThreadSegmentationExecutor.hpp"
#include "viewmodel/SegmentationViewModel.hpp"

class AppContext final
{
public:
    explicit AppContext(QQmlApplicationEngine& engine);
    ~AppContext();

private:
    random_walker::executor::JThreadSegmentationExecutor
        segmentation_executor_;
    std::unique_ptr<SegmentationViewModel> segmentation_view_model_;
};

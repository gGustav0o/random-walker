#pragma once

#include <memory>

#include <QQmlApplicationEngine>

#include "model/executor/JThreadSegmentationExecutor.hpp"
#include "viewmodel/SegmentationViewModel.hpp"

class AppContext final
{
public:
    explicit AppContext(QQmlApplicationEngine& engine);
    ~AppContext() = default;

private:
    // Member order is intentional: destruction happens in reverse order,
    // so the ViewModel stops using the executor before the executor joins.
    random_walker::executor::JThreadSegmentationExecutor
        segmentation_executor_;
    std::unique_ptr<SegmentationViewModel> segmentation_view_model_;
};

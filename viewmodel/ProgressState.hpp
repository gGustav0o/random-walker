#pragma once

#include <QString>

#include "model/domain/ProgressReporter.hpp"

class ProgressState final {
public:
    enum Stage {
        Idle
        , ValidatingInput
        , ExpandingSeeds
        , BuildingGraph
        , BuildingBoundaryConditions
        , PartitioningSystem
        , Factorizing
        , Solving
        , AssemblingProbabilities
        , Thresholding
    };

    [[nodiscard]] int stage() const noexcept;
    [[nodiscard]] double fraction() const noexcept;
    [[nodiscard]] bool indeterminate() const noexcept;
    [[nodiscard]] QString status_text() const;

    [[nodiscard]] bool reset();
    [[nodiscard]] bool apply(
        const random_walker::domain::SegmentationProgress& progress);

private:
    [[nodiscard]] static int stage_from(
        random_walker::domain::SegmentationStage stage) noexcept;

    int stage_ = Idle;
    double fraction_ = 0.0;
    bool indeterminate_ = false;
};

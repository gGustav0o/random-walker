#include "ProgressState.hpp"

#include <QtGlobal>

int ProgressState::stage() const noexcept {
    return stage_;
}

double ProgressState::fraction() const noexcept {
    return fraction_;
}

bool ProgressState::indeterminate() const noexcept {
    return indeterminate_;
}

QString ProgressState::status_text() const {
    switch (stage_) {
    case Idle:
        return {};
    case ValidatingInput:
        return QStringLiteral("Validating input");
    case ExpandingSeeds:
        return QStringLiteral("Preparing seed regions");
    case BuildingGraph:
        return QStringLiteral("Building pixel graph");
    case BuildingBoundaryConditions:
        return QStringLiteral("Building boundary conditions");
    case PartitioningSystem:
        return QStringLiteral("Building linear system");
    case Factorizing:
        return QStringLiteral("Factorizing Laplacian");
    case Solving:
        return QStringLiteral("Solving Random Walker system");
    case AssemblingProbabilities:
        return QStringLiteral("Assembling probability map");
    case Thresholding:
        return QStringLiteral("Building segmentation mask");
    }

    return {};
}

bool ProgressState::reset() {
    if (stage_ == Idle
        && qFuzzyIsNull(fraction_)
        && !indeterminate_) {
        return false;
    }

    stage_ = Idle;
    fraction_ = 0.0;
    indeterminate_ = false;
    return true;
}

bool ProgressState::apply(
    const random_walker::domain::SegmentationProgress& progress) {
    const int next_stage = stage_from(progress.stage);
    const bool next_indeterminate = !progress.fraction.has_value();
    const double next_fraction = progress.fraction.value_or(0.0);

    if (stage_ == next_stage
        && indeterminate_ == next_indeterminate
        && qFuzzyCompare(fraction_, next_fraction)) {
        return false;
    }

    stage_ = next_stage;
    indeterminate_ = next_indeterminate;
    fraction_ = next_fraction;
    return true;
}

int ProgressState::stage_from(
    random_walker::domain::SegmentationStage stage) noexcept {
    using DomainStage = random_walker::domain::SegmentationStage;

    switch (stage) {
    case DomainStage::ValidatingInput:
        return ValidatingInput;
    case DomainStage::ExpandingSeeds:
        return ExpandingSeeds;
    case DomainStage::BuildingGraph:
        return BuildingGraph;
    case DomainStage::BuildingBoundaryConditions:
        return BuildingBoundaryConditions;
    case DomainStage::PartitioningSystem:
        return PartitioningSystem;
    case DomainStage::Factorizing:
        return Factorizing;
    case DomainStage::Solving:
        return Solving;
    case DomainStage::AssemblingProbabilities:
        return AssemblingProbabilities;
    case DomainStage::Thresholding:
        return Thresholding;
    }

    return Idle;
}

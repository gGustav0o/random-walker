#pragma once

#include <optional>

#include <QString>
#include <QtGlobal>

#include "model/domain/Segmentation.hpp"
#include "presentation/image/PresentationImageCache.hpp"

class ResultState final {
public:
    explicit ResultState(PresentationImageCache& cache) noexcept;

    [[nodiscard]] bool has_result() const noexcept;
    [[nodiscard]] QString source() const;
    [[nodiscard]] quint64 version() const noexcept;

    void set(random_walker::domain::SegmentationResult result);
    [[nodiscard]] bool clear();

private:
    PresentationImageCache& cache_;
    std::optional<random_walker::domain::SegmentationResult> result_;
    quint64 version_ = 0;
};

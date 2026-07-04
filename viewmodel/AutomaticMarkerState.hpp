#pragma once

#include <optional>
#include <vector>

#include <QString>
#include <QtGlobal>

#include "model/domain/AutoMarkers.hpp"
#include "presentation/image/PresentationImageCache.hpp"

class AutomaticMarkerState final {
public:
    explicit AutomaticMarkerState(PresentationImageCache& cache) noexcept;

    [[nodiscard]] bool has_markers() const noexcept;
    [[nodiscard]] int background_count() const noexcept;
    [[nodiscard]] int object_count() const noexcept;
    [[nodiscard]] int marker_count() const noexcept;
    [[nodiscard]] QString source() const;
    [[nodiscard]] quint64 version() const noexcept;
    [[nodiscard]] const random_walker::domain::MarkerLabelMask* mask()
        const noexcept;
    [[nodiscard]] std::vector<random_walker::domain::SeedRegion>
    seed_regions() const;

    void set(random_walker::domain::MarkerLabelMask mask);
    [[nodiscard]] bool clear();

private:
    PresentationImageCache& cache_;
    std::optional<random_walker::domain::MarkerLabelMask> mask_;
    quint64 version_ = 0;
};

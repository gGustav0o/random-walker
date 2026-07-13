#include "AutomaticMarkerState.hpp"

#include <utility>

#include "presentation/adapter/AutoMarkerMaskRenderer.hpp"
#include "presentation/qml/qml_names.hpp"

AutomaticMarkerState::AutomaticMarkerState(PresentationImageCache& cache) noexcept
    : cache_(cache) {
}

bool AutomaticMarkerState::has_markers() const noexcept {
    return mask_.has_value() && mask_->seed_count() > 0;
}

int AutomaticMarkerState::background_count() const noexcept {
    return mask_.has_value()
        ? random_walker::domain::marker_pixel_count(
            *mask_
            , random_walker::domain::SeedLabel::Background
        )
        : 0;
}

int AutomaticMarkerState::object_count() const noexcept {
    return mask_.has_value()
        ? random_walker::domain::marker_pixel_count(
            *mask_
            , random_walker::domain::SeedLabel::Object
        )
        : 0;
}

int AutomaticMarkerState::marker_count() const noexcept {
    return mask_.has_value()
        ? static_cast<int>(mask_->seed_count())
        : 0;
}


QString AutomaticMarkerState::source() const {
    return has_markers()
        ? QStringLiteral("image://%1/mask").arg(
            qml_names::kAutoMarkerImageProvider
        )
        : QString();
}

quint64 AutomaticMarkerState::version() const noexcept {
    return version_;
}

const random_walker::domain::MarkerLabelMask* AutomaticMarkerState::mask()
    const noexcept {
    return mask_.has_value() ? &*mask_ : nullptr;
}

void AutomaticMarkerState::set(random_walker::domain::MarkerLabelMask mask) {
    mask_ = std::move(mask);
    cache_.store(random_walker::qt_adapter::render_marker_label_mask(*mask_));
    ++version_;
}

bool AutomaticMarkerState::clear() {
    if (!mask_.has_value()) {
        return false;
    }

    mask_.reset();
    cache_.clear();
    ++version_;
    return true;
}

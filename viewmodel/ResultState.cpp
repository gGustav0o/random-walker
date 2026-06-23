#include "ResultState.hpp"

#include <utility>

#include "presentation/adapter/MaskRenderer.hpp"
#include "presentation/qml/qml_names.hpp"

ResultState::ResultState(PresentationImageCache& cache) noexcept
    : cache_(cache) {
}

bool ResultState::has_result() const noexcept {
    return result_.has_value();
}

QString ResultState::source() const {
    return has_result()
        ? QStringLiteral("image://%1/mask").arg(
            qml_names::kResultImageProvider
        )
        : QString();
}

quint64 ResultState::version() const noexcept {
    return version_;
}

void ResultState::set(random_walker::domain::SegmentationResult result) {
    result_ = std::move(result);
    cache_.store(random_walker::qt_adapter::render_binary_mask(result_->mask));
    ++version_;
}

bool ResultState::clear() {
    if (!result_.has_value()) {
        return false;
    }

    result_.reset();
    cache_.clear();
    ++version_;
    return true;
}

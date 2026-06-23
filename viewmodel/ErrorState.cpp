#include "ErrorState.hpp"

#include <utility>

#include "presentation/error/ErrorPresenter.hpp"

bool ErrorState::has_error() const noexcept {
    return error_.has_value();
}

QString ErrorState::message() const {
    return error_.has_value()
        ? random_walker::presentation::error_message(*error_)
        : QString();
}

bool ErrorState::set(random_walker::application::UserError error) {
    if (error_.has_value() && *error_ == error) {
        return false;
    }

    error_ = std::move(error);
    return true;
}

bool ErrorState::clear() {
    if (!error_.has_value()) {
        return false;
    }

    error_.reset();
    return true;
}

#pragma once

#include <optional>

#include <QString>

#include "application/error/UserError.hpp"

class ErrorState final {
public:
    [[nodiscard]] bool has_error() const noexcept;
    [[nodiscard]] QString message() const;

    [[nodiscard]] bool set(random_walker::application::UserError error);
    [[nodiscard]] bool clear();

private:
    std::optional<random_walker::application::UserError> error_;
};

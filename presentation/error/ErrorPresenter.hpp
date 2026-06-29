#pragma once

#include <QString>

#include "application/error/UserError.hpp"

namespace random_walker::presentation {
    [[nodiscard]] QString error_message(
        const application::UserError& error
    );
}

#pragma once

#include "ApplicationSettings.hpp"

namespace random_walker::application {
    class SettingsRepository {
    public:
        virtual ~SettingsRepository() = default;

        [[nodiscard]] virtual ApplicationSettings load() const = 0;
        virtual void save(const ApplicationSettings& settings) = 0;
    };
}

#pragma once

#include "ApplicationSettings.hpp"

namespace random_walker::application {

    struct SettingsRepositoryLoadResult {
        ApplicationSettings settings;
        bool repair_required = false;
    };

    class SettingsRepository {
    public:
        virtual ~SettingsRepository() = default;

        [[nodiscard]] virtual SettingsRepositoryLoadResult load() const = 0;
        [[nodiscard]] virtual bool save(const ApplicationSettings& settings) = 0;
    };
}

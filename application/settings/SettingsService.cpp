#include "SettingsService.hpp"

namespace random_walker::application {
    SettingsService::SettingsService(SettingsRepository& repository)
        : repository_(repository) {
    }

    SettingsLoadResult SettingsService::load() const {
        const SettingsRepositoryLoadResult stored = repository_.load();
        const ApplicationSettings normalized = normalize(stored.settings);

        return {
            .settings = normalized
            , .repair_required = stored.repair_required
                || normalized != stored.settings
        };
    }

    SettingsSaveOutcome SettingsService::save(
        const ApplicationSettings& settings
    ) {
        if (!is_valid(settings)) {
            return SettingsError::InvalidSettings;
        }

        if (!repository_.save(settings)) {
            return SettingsError::SaveFailed;
        }

        return std::nullopt;
    }

    ApplicationSettings SettingsService::defaults() {
        return {};
    }

    bool SettingsService::is_valid(
        const ApplicationSettings& settings
    ) noexcept {
        return domain::is_valid(settings.random_walker);
    }

    ApplicationSettings SettingsService::normalize(
        ApplicationSettings settings
    ) {
        const ApplicationSettings default_settings = defaults();
        if (!is_valid(settings)) {
            settings.random_walker = default_settings.random_walker;
        }
        return settings;
    }
}

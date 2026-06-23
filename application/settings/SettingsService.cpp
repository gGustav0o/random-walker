#include "SettingsService.hpp"

namespace random_walker::application {
    SettingsService::SettingsService(SettingsRepository& repository)
        : repository_(repository) {
    }

    ApplicationSettings SettingsService::load() {
        const ApplicationSettings stored = repository_.load();
        const ApplicationSettings normalized = normalize(stored);

        if (normalized != stored) {
            (void)repository_.save(normalized);
        }

        return normalized;
    }

    SettingsSaveOutcome SettingsService::save(
        const ApplicationSettings& settings) {
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
        const ApplicationSettings& settings) noexcept {
        return domain::is_valid(settings.random_walker);
    }

    ApplicationSettings SettingsService::normalize(
        ApplicationSettings settings) {
        const ApplicationSettings default_settings = defaults();
        if (!is_valid(settings)) {
            settings.random_walker = default_settings.random_walker;
        }
        return settings;
    }
}

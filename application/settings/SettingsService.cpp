#include "SettingsService.hpp"

namespace random_walker::application {
    SettingsService::SettingsService(SettingsRepository& repository)
        : repository_(repository) {
    }

    ApplicationSettings SettingsService::load() {
        const ApplicationSettings stored = repository_.load();
        const ApplicationSettings normalized = normalize(stored);

        if (normalized != stored) {
            repository_.save(normalized);
        }

        return normalized;
    }

    bool SettingsService::try_save(const ApplicationSettings& settings) {
        if (!is_valid(settings)) {
            return false;
        }

        repository_.save(settings);
        return true;
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

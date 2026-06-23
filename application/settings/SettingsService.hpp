#pragma once

#include <optional>

#include "application/error/UserError.hpp"
#include "ApplicationSettings.hpp"
#include "SettingsRepository.hpp"

namespace random_walker::application {
    using SettingsSaveOutcome = std::optional<SettingsError>;

    class SettingsService final {
    public:
        explicit SettingsService(SettingsRepository& repository);

        [[nodiscard]] ApplicationSettings load();
        [[nodiscard]] SettingsSaveOutcome save(
            const ApplicationSettings& settings);

    private:
        [[nodiscard]] static ApplicationSettings defaults();
        [[nodiscard]] static bool is_valid(
            const ApplicationSettings& settings) noexcept;
        [[nodiscard]] static ApplicationSettings normalize(
            ApplicationSettings settings);

        SettingsRepository& repository_;
    };
}

#pragma once

#include "ApplicationSettings.hpp"
#include "SettingsRepository.hpp"

namespace random_walker::application
{
    class SettingsService final
    {
    public:
        explicit SettingsService(SettingsRepository& repository);

        [[nodiscard]] ApplicationSettings load();
        [[nodiscard]] bool try_save(const ApplicationSettings& settings);

    private:
        [[nodiscard]] static ApplicationSettings defaults();
        [[nodiscard]] static bool is_valid(
            const ApplicationSettings& settings) noexcept;
        [[nodiscard]] static ApplicationSettings normalize(
            ApplicationSettings settings);

        SettingsRepository& repository_;
    };
}

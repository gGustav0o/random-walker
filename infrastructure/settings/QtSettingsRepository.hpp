#pragma once

#include <QSettings>
#include <QString>

#include "application/settings/SettingsRepository.hpp"

namespace random_walker::infrastructure {

    class QtSettingsRepository final
        : public application::SettingsRepository {

    public:
        QtSettingsRepository(
            QString organization_name
            , QString application_name
        );

        QtSettingsRepository(
            QString file_name
            , QSettings::Format format
        );

        [[nodiscard]]
        application::SettingsRepositoryLoadResult load() const override;

        [[nodiscard]]
        bool save(
            const application::ApplicationSettings& settings
        ) override;

    private:
        [[nodiscard]]
        application::SettingsRepositoryLoadResult load_current_schema() const;

        [[nodiscard]]
        application::ApplicationSettings
            migrate_from(int schema_version) const;

        void write_current_schema(
            const application::ApplicationSettings& settings
        ) const;

        mutable QSettings settings_;
    };
}

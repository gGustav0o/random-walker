#include "QSettingsRepository.hpp"

#include <limits>
#include <utility>

#include <QVariant>

namespace {
    constexpr int kCurrentSchemaVersion = 1;

    constexpr auto kSettingsGroup = "applicationSettings";
    constexpr auto kSchemaVersionKey = "schemaVersion";
    constexpr auto kRandomWalkerBetaKey = "randomWalker/beta";
    constexpr auto kLegacyBetaKey = "beta";

    [[nodiscard]] double stored_double(
        const QSettings& settings
        , const char* key
    ) {
        bool converted = false;
        const double value = settings.value(key).toDouble(&converted);
        return converted
            ? value
            : std::numeric_limits<double>::quiet_NaN();
    }
}

namespace random_walker::infrastructure {
    QSettingsRepository::QSettingsRepository(
        QString organization_name
        , QString application_name
    )
        : settings_(
            QSettings::NativeFormat
            , QSettings::UserScope
            , std::move(organization_name)
            , std::move(application_name)
        ) {
    }

    QSettingsRepository::QSettingsRepository(
        QString file_name
        , QSettings::Format format
    )
        : settings_(std::move(file_name), format) {
    }

    application::SettingsRepositoryLoadResult QSettingsRepository::load()
        const {
        settings_.beginGroup(kSettingsGroup);

        bool converted = false;
        const int schema_version =
            settings_.value(kSchemaVersionKey).toInt(&converted);

        application::SettingsRepositoryLoadResult result;
        if (converted && schema_version == kCurrentSchemaVersion) {
            result.settings = load_current_schema();
        } else if (!converted || schema_version < kCurrentSchemaVersion) {
            result.settings = migrate_from(converted ? schema_version : 0);
            result.repair_required = true;
        } else {
            // A newer schema must not be interpreted by an older binary.
            result.settings = {};
        }

        settings_.endGroup();
        return result;
    }

    bool QSettingsRepository::save(
        const application::ApplicationSettings& application_settings) {
        settings_.beginGroup(kSettingsGroup);
        write_current_schema(application_settings);
        settings_.endGroup();
        settings_.sync();
        return settings_.status() == QSettings::NoError;
    }

    application::ApplicationSettings
    QSettingsRepository::load_current_schema() const {
        application::ApplicationSettings result;
        result.random_walker.beta = stored_double(
            settings_
            , kRandomWalkerBetaKey
        );
        return result;
    }

    application::ApplicationSettings
    QSettingsRepository::migrate_from(int schema_version) const {
        application::ApplicationSettings result;

        switch (schema_version) {
        case 0:
            result.random_walker.beta =
                stored_double(settings_, kLegacyBetaKey);
            return result;
        default:
            return {};
        }
    }

    void QSettingsRepository::write_current_schema(
        const application::ApplicationSettings& application_settings) const {
        settings_.remove(QString{});
        settings_.setValue(
            kRandomWalkerBetaKey
            , application_settings.random_walker.beta
        );
        settings_.setValue(kSchemaVersionKey, kCurrentSchemaVersion);
    }
}

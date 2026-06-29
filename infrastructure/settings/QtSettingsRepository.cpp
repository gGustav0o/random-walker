#include "QtSettingsRepository.hpp"

#include <limits>
#include <utility>

#include <QDebug>
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
        const QVariant stored_value = settings.value(key);
        const double value = stored_value.toDouble(&converted);
        if (!converted) {
            qWarning()
                << "Failed to read numeric settings value"
                << key
                << "from"
                << settings.fileName()
                << "; defaults will be used if validation fails";
            return std::numeric_limits<double>::quiet_NaN();
        }

        return value;
    }
}

namespace random_walker::infrastructure {

    QtSettingsRepository::QtSettingsRepository(
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

    QtSettingsRepository::QtSettingsRepository(
        QString file_name
        , QSettings::Format format
    )
        : settings_(std::move(file_name), format) {
    }

    application::SettingsRepositoryLoadResult QtSettingsRepository::load() const {
        settings_.beginGroup(kSettingsGroup);

        bool converted = false;
        const QVariant stored_schema_version = settings_.value(kSchemaVersionKey);
        const int schema_version = stored_schema_version.toInt(&converted);

        application::SettingsRepositoryLoadResult result;

        if (converted && schema_version == kCurrentSchemaVersion) {
            result.settings = load_current_schema();
        } else if (!converted) {
            qWarning()
                << "Settings schema version is missing or invalid in"
                << settings_.fileName()
                << "; trying legacy settings migration";
            result.settings = migrate_from(0);
            result.repair_required = true;
        } else if (schema_version < kCurrentSchemaVersion) {
            qWarning()
                << "Migrating settings schema from version"
                << schema_version
                << "to"
                << kCurrentSchemaVersion;
            result.settings = migrate_from(schema_version);
            result.repair_required = true;
        } else {
            qWarning()
                << "Settings schema version"
                << schema_version
                << "is newer than supported version"
                << kCurrentSchemaVersion
                << "; settings will be ignored";
            // A newer schema must not be interpreted by an older binary.
            result.settings = {};
        }

        settings_.endGroup();
        return result;
    }

    bool QtSettingsRepository::save(
        const application::ApplicationSettings& application_settings) {
        settings_.beginGroup(kSettingsGroup);
        write_current_schema(application_settings);
        settings_.endGroup();
        settings_.sync();
        return settings_.status() == QSettings::NoError;
    }

    application::ApplicationSettings
    QtSettingsRepository::load_current_schema() const {
        application::ApplicationSettings result;
        result.random_walker.beta = stored_double(
            settings_
            , kRandomWalkerBetaKey
        );
        return result;
    }

    application::ApplicationSettings
    QtSettingsRepository::migrate_from(int schema_version) const {
        application::ApplicationSettings result;

        switch (schema_version) {
        case 0:
            result.random_walker.beta =
                stored_double(settings_, kLegacyBetaKey);
            return result;
        default:
            qWarning()
                << "Unsupported legacy settings schema version"
                << schema_version
                << "; defaults will be used";
            return {};
        }
    }

    void QtSettingsRepository::write_current_schema(
        const application::ApplicationSettings& application_settings
    ) const {
        settings_.remove(QString{});
        settings_.setValue(
            kRandomWalkerBetaKey
            , application_settings.random_walker.beta
        );
        settings_.setValue(kSchemaVersionKey, kCurrentSchemaVersion);
    }
}

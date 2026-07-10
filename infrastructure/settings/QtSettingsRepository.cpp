#include "QtSettingsRepository.hpp"

#include <limits>
#include <optional>
#include <utility>

#include <QDebug>
#include <QVariant>

namespace {

    constexpr int kCurrentSchemaVersion = 7;

    constexpr auto kSettingsGroup = "applicationSettings";
    constexpr auto kSchemaVersionKey = "schemaVersion";
    constexpr auto kRandomWalkerBetaKey = "randomWalker/beta";
    constexpr auto kRandomWalkerConnectivityKey = "randomWalker/connectivity";
    constexpr auto kRandomWalkerDistancePowerKey = "randomWalker/distancePower";
    constexpr auto kLegacyBetaKey = "beta";

    template<typename Value>
    struct StoredValue {
        Value value;
        bool repair_required = false;
    };

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

    [[nodiscard]] StoredValue<double> stored_double_or_default(
        const QSettings& settings
        , const char* key
        , double default_value
    ) {
        if (!settings.contains(key)) {
            return {
                .value = default_value
                , .repair_required = true
            };
        }

        bool converted = false;
        const QVariant stored_value = settings.value(key);
        const double value = stored_value.toDouble(&converted);
        if (!converted) {
            qWarning()
                << "Failed to read numeric settings value"
                << key
                << "from"
                << settings.fileName()
                << "; settings will be repaired";
            return {
                .value = std::numeric_limits<double>::quiet_NaN()
                , .repair_required = true
            };
        }

        return {.value = value};
    }

    [[nodiscard]] std::optional<random_walker::domain::PixelConnectivity>
    connectivity_from_storage(const QString& value) noexcept {
        if (value == QStringLiteral("four")) {
            return random_walker::domain::PixelConnectivity::Four;
        }
        if (value == QStringLiteral("eight")) {
            return random_walker::domain::PixelConnectivity::Eight;
        }

        return std::nullopt;
    }

    [[nodiscard]] QString connectivity_to_storage(
        random_walker::domain::PixelConnectivity connectivity
    ) noexcept {
        switch (connectivity) {
        case random_walker::domain::PixelConnectivity::Four:
            return QStringLiteral("four");
        case random_walker::domain::PixelConnectivity::Eight:
            return QStringLiteral("eight");
        }

        Q_ASSERT_X(false, "connectivity_to_storage", "Unhandled connectivity");
        return QStringLiteral("four");
    }

    [[nodiscard]] random_walker::domain::PixelConnectivity stored_connectivity(
        const QSettings& settings
        , const char* key
    ) {
        const QVariant stored_value = settings.value(key);
        const std::optional<random_walker::domain::PixelConnectivity>
            connectivity = connectivity_from_storage(stored_value.toString());
        if (!connectivity.has_value()) {
            qWarning()
                << "Failed to read connectivity settings value"
                << key
                << "from"
                << settings.fileName()
                << "; settings will be repaired";
            return static_cast<random_walker::domain::PixelConnectivity>(-1);
        }

        return *connectivity;
    }

    [[nodiscard]]
    StoredValue<random_walker::domain::PixelConnectivity>
    stored_connectivity_or_default(
        const QSettings& settings
        , const char* key
        , random_walker::domain::PixelConnectivity default_value
    ) {
        if (!settings.contains(key)) {
            return {
                .value = default_value
                , .repair_required = true
            };
        }

        const QVariant stored_value = settings.value(key);
        const std::optional<random_walker::domain::PixelConnectivity>
            connectivity = connectivity_from_storage(stored_value.toString());
        if (!connectivity.has_value()) {
            qWarning()
                << "Failed to read connectivity settings value"
                << key
                << "from"
                << settings.fileName()
                << "; settings will be repaired";
            return {
                .value = static_cast<random_walker::domain::PixelConnectivity>(-1)
                , .repair_required = true
            };
        }

        return {.value = *connectivity};
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
            result = load_current_schema();
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

    application::SettingsRepositoryLoadResult
    QtSettingsRepository::load_current_schema() const {
        application::SettingsRepositoryLoadResult result;
        application::ApplicationSettings settings;
        const application::ApplicationSettings default_settings;

        const auto beta = stored_double_or_default(
            settings_
            , kRandomWalkerBetaKey
            , default_settings.random_walker.beta
        );
        settings.random_walker.beta = beta.value;
        result.repair_required |= beta.repair_required;

        const auto connectivity = stored_connectivity_or_default(
            settings_
            , kRandomWalkerConnectivityKey
            , default_settings.random_walker.connectivity
        );
        settings.random_walker.connectivity = connectivity.value;
        result.repair_required |= connectivity.repair_required;

        const auto distance_power = stored_double_or_default(
            settings_
            , kRandomWalkerDistancePowerKey
            , default_settings.random_walker.distance_power
        );
        settings.random_walker.distance_power = distance_power.value;
        result.repair_required |= distance_power.repair_required;

        result.settings = settings;
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
        case 1:
            result.random_walker.beta =
                stored_double(settings_, kRandomWalkerBetaKey);
            return result;
        case 2:
            result.random_walker.beta =
                stored_double(settings_, kRandomWalkerBetaKey);
            result.random_walker.connectivity = stored_connectivity(
                settings_
                , kRandomWalkerConnectivityKey
            );
            return result;
        default:
            result.random_walker.beta =
                stored_double(settings_, kRandomWalkerBetaKey);
            result.random_walker.connectivity = stored_connectivity(
                settings_
                , kRandomWalkerConnectivityKey
            );
            result.random_walker.distance_power = stored_double(
                settings_
                , kRandomWalkerDistancePowerKey
            );
            return result;
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
        settings_.setValue(
            kRandomWalkerConnectivityKey
            , connectivity_to_storage(
                application_settings.random_walker.connectivity
            )
        );
        settings_.setValue(
            kRandomWalkerDistancePowerKey
            , application_settings.random_walker.distance_power
        );
        settings_.setValue(kSchemaVersionKey, kCurrentSchemaVersion);
    }
}

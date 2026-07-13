#include "QtSettingsRepository.hpp"

#include <limits>
#include <optional>
#include <utility>

#include <QDebug>
#include <QVariant>

namespace {

    constexpr int kCurrentSchemaVersion = 9;

    constexpr auto kSettingsGroup = "applicationSettings";
    constexpr auto kSchemaVersionKey = "schemaVersion";
    constexpr auto kRandomWalkerBetaKey = "randomWalker/beta";
    constexpr auto kRandomWalkerConnectivityKey = "randomWalker/connectivity";
    constexpr auto kRandomWalkerDistancePowerKey = "randomWalker/distancePower";
    constexpr auto kAutoMarkersConfidenceThresholdKey =
        "autoMarkers/confidenceThreshold";
    constexpr auto kAutoMarkersMinimumComponentAreaKey =
        "autoMarkers/minimumComponentArea";
    constexpr auto kAutoMarkersMinimumBoundaryDistanceKey =
        "autoMarkers/minimumBoundaryDistance";
    constexpr auto kAutoMarkersForegroundPolarityKey =
        "autoMarkers/foregroundPolarity";
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

    [[nodiscard]] StoredValue<int> stored_int_or_default(
        const QSettings& settings
        , const char* key
        , int default_value
    ) {
        if (!settings.contains(key)) {
            return {
                .value = default_value
                , .repair_required = true
            };
        }

        bool converted = false;
        const QVariant stored_value = settings.value(key);
        const int value = stored_value.toInt(&converted);
        if (!converted) {
            qWarning()
                << "Failed to read integer settings value"
                << key
                << "from"
                << settings.fileName()
                << "; settings will be repaired";
            return {
                .value = std::numeric_limits<int>::min()
                , .repair_required = true
            };
        }

        return {.value = value};
    }

    [[nodiscard]] double stored_legacy_raw_beta(
        const QSettings& settings
        , const char* key
    ) {
        return random_walker::domain::normalized_beta_from_legacy_raw_beta(
            stored_double(settings, key)
        );
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

    [[nodiscard]] std::optional<random_walker::domain::ForegroundPolarity>
    foreground_polarity_from_storage(const QString& value) noexcept {
        if (value == QStringLiteral("dark")) {
            return random_walker::domain::ForegroundPolarity::DarkObject;
        }
        if (value == QStringLiteral("bright")) {
            return random_walker::domain::ForegroundPolarity::BrightObject;
        }

        return std::nullopt;
    }

    [[nodiscard]] QString foreground_polarity_to_storage(
        random_walker::domain::ForegroundPolarity foreground_polarity
    ) noexcept {
        switch (foreground_polarity) {
        case random_walker::domain::ForegroundPolarity::DarkObject:
            return QStringLiteral("dark");
        case random_walker::domain::ForegroundPolarity::BrightObject:
            return QStringLiteral("bright");
        }

        Q_ASSERT_X(
            false
            , "foreground_polarity_to_storage"
            , "Unhandled foreground polarity"
        );
        return QStringLiteral("bright");
    }

    [[nodiscard]]
    StoredValue<random_walker::domain::ForegroundPolarity>
    stored_foreground_polarity_or_default(
        const QSettings& settings
        , const char* key
        , random_walker::domain::ForegroundPolarity default_value
    ) {
        if (!settings.contains(key)) {
            return {
                .value = default_value
                , .repair_required = true
            };
        }

        const QVariant stored_value = settings.value(key);
        const std::optional<random_walker::domain::ForegroundPolarity>
            foreground_polarity =
                foreground_polarity_from_storage(stored_value.toString());
        if (!foreground_polarity.has_value()) {
            qWarning()
                << "Failed to read foreground polarity settings value"
                << key
                << "from"
                << settings.fileName()
                << "; settings will be repaired";
            return {
                .value = static_cast<
                    random_walker::domain::ForegroundPolarity
                >(-1)
                , .repair_required = true
            };
        }

        return {.value = *foreground_polarity};
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

        const auto confidence_threshold = stored_double_or_default(
            settings_
            , kAutoMarkersConfidenceThresholdKey
            , default_settings.auto_markers.confidence_threshold
        );
        settings.auto_markers.confidence_threshold =
            confidence_threshold.value;
        result.repair_required |= confidence_threshold.repair_required;

        const auto minimum_component_area = stored_int_or_default(
            settings_
            , kAutoMarkersMinimumComponentAreaKey
            , default_settings.auto_markers.minimum_component_area
        );
        settings.auto_markers.minimum_component_area =
            minimum_component_area.value;
        result.repair_required |= minimum_component_area.repair_required;

        const auto minimum_boundary_distance = stored_int_or_default(
            settings_
            , kAutoMarkersMinimumBoundaryDistanceKey
            , default_settings.auto_markers.minimum_boundary_distance
        );
        settings.auto_markers.minimum_boundary_distance =
            minimum_boundary_distance.value;
        result.repair_required |= minimum_boundary_distance.repair_required;

        const auto foreground_polarity =
            stored_foreground_polarity_or_default(
                settings_
                , kAutoMarkersForegroundPolarityKey
                , default_settings.auto_markers.foreground_polarity
            );
        settings.auto_markers.foreground_polarity =
            foreground_polarity.value;
        result.repair_required |= foreground_polarity.repair_required;

        result.settings = settings;
        return result;
    }

    application::ApplicationSettings
    QtSettingsRepository::migrate_from(int schema_version) const {
        application::ApplicationSettings result;

        switch (schema_version) {
        case 0:
            result.random_walker.beta =
                stored_legacy_raw_beta(settings_, kLegacyBetaKey);
            return result;
        case 1:
            result.random_walker.beta =
                stored_legacy_raw_beta(settings_, kRandomWalkerBetaKey);
            return result;
        case 2:
            result.random_walker.beta =
                stored_legacy_raw_beta(settings_, kRandomWalkerBetaKey);
            result.random_walker.connectivity = stored_connectivity(
                settings_
                , kRandomWalkerConnectivityKey
            );
            return result;
        default:
            result.random_walker.beta =
                stored_legacy_raw_beta(settings_, kRandomWalkerBetaKey);
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
        settings_.setValue(
            kAutoMarkersConfidenceThresholdKey
            , application_settings.auto_markers.confidence_threshold
        );
        settings_.setValue(
            kAutoMarkersMinimumComponentAreaKey
            , application_settings.auto_markers.minimum_component_area
        );
        settings_.setValue(
            kAutoMarkersMinimumBoundaryDistanceKey
            , application_settings.auto_markers.minimum_boundary_distance
        );
        settings_.setValue(
            kAutoMarkersForegroundPolarityKey
            , foreground_polarity_to_storage(
                application_settings.auto_markers.foreground_polarity
            )
        );
        settings_.setValue(kSchemaVersionKey, kCurrentSchemaVersion);
    }
}

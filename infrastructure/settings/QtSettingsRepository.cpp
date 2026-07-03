#include "QtSettingsRepository.hpp"

#include <limits>
#include <optional>
#include <utility>

#include <QDebug>
#include <QVariant>

namespace {

    constexpr int kCurrentSchemaVersion = 4;

    constexpr auto kSettingsGroup = "applicationSettings";
    constexpr auto kSchemaVersionKey = "schemaVersion";
    constexpr auto kRandomWalkerBetaKey = "randomWalker/beta";
    constexpr auto kRandomWalkerConnectivityKey = "randomWalker/connectivity";
    constexpr auto kRandomWalkerDistancePowerKey = "randomWalker/distancePower";
    constexpr auto kRandomWalkerEdgeWeightModelKey = "randomWalker/edgeWeightModel";
    constexpr auto kRandomWalkerLocalContrastRadiusKey =
        "randomWalker/localContrast/radius";
    constexpr auto kRandomWalkerLocalContrastMinimumVarianceKey =
        "randomWalker/localContrast/minimumVariance";
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

    [[nodiscard]] int stored_int(
        const QSettings& settings
        , const char* key
    ) {
        bool converted = false;
        const QVariant stored_value = settings.value(key);
        const int value = stored_value.toInt(&converted);
        if (!converted) {
            qWarning()
                << "Failed to read integer settings value"
                << key
                << "from"
                << settings.fileName()
                << "; defaults will be used if validation fails";
            return std::numeric_limits<int>::min();
        }

        return value;
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

    [[nodiscard]] std::optional<random_walker::domain::EdgeWeightModel>
    edge_weight_model_from_storage(const QString& value) noexcept {
        if (value == QStringLiteral("globalBeta")) {
            return random_walker::domain::EdgeWeightModel::GlobalBeta;
        }
        if (value == QStringLiteral("localVarianceNormalized")) {
            return random_walker::domain::EdgeWeightModel::LocalVarianceNormalized;
        }

        return std::nullopt;
    }

    [[nodiscard]] QString edge_weight_model_to_storage(
        random_walker::domain::EdgeWeightModel model
    ) noexcept {
        switch (model) {
        case random_walker::domain::EdgeWeightModel::GlobalBeta:
            return QStringLiteral("globalBeta");
        case random_walker::domain::EdgeWeightModel::LocalVarianceNormalized:
            return QStringLiteral("localVarianceNormalized");
        }

        Q_ASSERT_X(
            false
            , "edge_weight_model_to_storage"
            , "Unhandled edge weight model"
        );
        return QStringLiteral("globalBeta");
    }

    [[nodiscard]] random_walker::domain::EdgeWeightModel stored_edge_weight_model(
        const QSettings& settings
        , const char* key
    ) {
        const QVariant stored_value = settings.value(key);
        const std::optional<random_walker::domain::EdgeWeightModel> model =
            edge_weight_model_from_storage(stored_value.toString());
        if (!model.has_value()) {
            qWarning()
                << "Failed to read edge weight model settings value"
                << key
                << "from"
                << settings.fileName()
                << "; settings will be repaired";
            return static_cast<random_walker::domain::EdgeWeightModel>(-1);
        }

        return *model;
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
        result.random_walker.connectivity = stored_connectivity(
            settings_
            , kRandomWalkerConnectivityKey
        );
        result.random_walker.distance_power = stored_double(
            settings_
            , kRandomWalkerDistancePowerKey
        );
        result.random_walker.edge_weight_model = stored_edge_weight_model(
            settings_
            , kRandomWalkerEdgeWeightModelKey
        );
        result.random_walker.local_contrast_scale.radius = stored_int(
            settings_
            , kRandomWalkerLocalContrastRadiusKey
        );
        result.random_walker.local_contrast_scale.minimum_variance =
            stored_double(
                settings_
                , kRandomWalkerLocalContrastMinimumVarianceKey
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
        case 3:
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
            kRandomWalkerEdgeWeightModelKey
            , edge_weight_model_to_storage(
                application_settings.random_walker.edge_weight_model
            )
        );
        settings_.setValue(
            kRandomWalkerLocalContrastRadiusKey
            , application_settings.random_walker.local_contrast_scale.radius
        );
        settings_.setValue(
            kRandomWalkerLocalContrastMinimumVarianceKey
            , application_settings.random_walker
                .local_contrast_scale
                .minimum_variance
        );
        settings_.setValue(kSchemaVersionKey, kCurrentSchemaVersion);
    }
}
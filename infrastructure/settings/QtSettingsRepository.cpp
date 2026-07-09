#include "QtSettingsRepository.hpp"

#include <limits>
#include <optional>
#include <utility>

#include <QDebug>
#include <QVariant>

namespace {

    constexpr int kCurrentSchemaVersion = 6;

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
    constexpr auto kRandomWalkerLocalContrastMinimumVarianceModeKey =
        "randomWalker/localContrast/minimumVarianceMode";
    constexpr auto kRandomWalkerLocalContrastAutoQuantileKey =
        "randomWalker/localContrast/autoMinimumVarianceQuantile";
    constexpr auto kRandomWalkerEdgeLocalContrastRadiusKey =
        "randomWalker/edgeLocalContrast/radius";
    constexpr auto kRandomWalkerEdgeLocalContrastQuantileKey =
        "randomWalker/edgeLocalContrast/quantile";
    constexpr auto kRandomWalkerEdgeLocalContrastMinimumScaleKey =
        "randomWalker/edgeLocalContrast/minimumScale";
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

    [[nodiscard]] std::optional<random_walker::domain::EdgeWeightModel>
    edge_weight_model_from_storage(const QString& value) noexcept {
        if (value == QStringLiteral("globalBeta")) {
            return random_walker::domain::EdgeWeightModel::GlobalBeta;
        }
        if (value == QStringLiteral("localVarianceNormalized")) {
            return random_walker::domain::EdgeWeightModel::LocalVarianceNormalized;
        }
        if (value == QStringLiteral("edgeLocalContrastNormalized")) {
            return random_walker::domain::EdgeWeightModel::EdgeLocalContrastNormalized;
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
        case random_walker::domain::EdgeWeightModel::EdgeLocalContrastNormalized:
            return QStringLiteral("edgeLocalContrastNormalized");
        }

        Q_ASSERT_X(
            false
            , "edge_weight_model_to_storage"
            , "Unhandled edge weight model"
        );
        return QStringLiteral("globalBeta");
    }

    [[nodiscard]] std::optional<random_walker::domain::MinimumVarianceMode>
    minimum_variance_mode_from_storage(const QString& value) noexcept {
        if (value == QStringLiteral("manual")) {
            return random_walker::domain::MinimumVarianceMode::Manual;
        }
        if (value == QStringLiteral("auto")) {
            return random_walker::domain::MinimumVarianceMode::Auto;
        }

        return std::nullopt;
    }

    [[nodiscard]] QString minimum_variance_mode_to_storage(
        random_walker::domain::MinimumVarianceMode mode
    ) noexcept {
        switch (mode) {
        case random_walker::domain::MinimumVarianceMode::Manual:
            return QStringLiteral("manual");
        case random_walker::domain::MinimumVarianceMode::Auto:
            return QStringLiteral("auto");
        }

        Q_ASSERT_X(
            false
            , "minimum_variance_mode_to_storage"
            , "Unhandled minimum variance mode"
        );
        return QStringLiteral("manual");
    }

    [[nodiscard]] random_walker::domain::MinimumVarianceMode
    stored_minimum_variance_mode(
        const QSettings& settings
        , const char* key
    ) {
        const QVariant stored_value = settings.value(key);
        const std::optional<random_walker::domain::MinimumVarianceMode> mode =
            minimum_variance_mode_from_storage(stored_value.toString());
        if (!mode.has_value()) {
            qWarning()
                << "Failed to read minimum variance mode settings value"
                << key
                << "from"
                << settings.fileName()
                << "; settings will be repaired";
            return static_cast<random_walker::domain::MinimumVarianceMode>(-1);
        }

        return *mode;
    }

    [[nodiscard]]
    StoredValue<random_walker::domain::MinimumVarianceMode>
    stored_minimum_variance_mode_or_default(
        const QSettings& settings
        , const char* key
        , random_walker::domain::MinimumVarianceMode default_value
    ) {
        if (!settings.contains(key)) {
            return {
                .value = default_value
                , .repair_required = true
            };
        }

        const QVariant stored_value = settings.value(key);
        const std::optional<random_walker::domain::MinimumVarianceMode> mode =
            minimum_variance_mode_from_storage(stored_value.toString());
        if (!mode.has_value()) {
            qWarning()
                << "Failed to read minimum variance mode settings value"
                << key
                << "from"
                << settings.fileName()
                << "; settings will be repaired";
            return {
                .value = static_cast<random_walker::domain::MinimumVarianceMode>(-1)
                , .repair_required = true
            };
        }

        return {.value = *mode};
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

    [[nodiscard]]
    StoredValue<random_walker::domain::EdgeWeightModel>
    stored_edge_weight_model_or_default(
        const QSettings& settings
        , const char* key
        , random_walker::domain::EdgeWeightModel default_value
    ) {
        if (!settings.contains(key)) {
            return {
                .value = default_value
                , .repair_required = true
            };
        }

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
            return {
                .value = static_cast<random_walker::domain::EdgeWeightModel>(-1)
                , .repair_required = true
            };
        }

        return {.value = *model};
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

        const auto edge_weight_model = stored_edge_weight_model_or_default(
            settings_
            , kRandomWalkerEdgeWeightModelKey
            , default_settings.random_walker.edge_weight_model
        );
        settings.random_walker.edge_weight_model = edge_weight_model.value;
        result.repair_required |= edge_weight_model.repair_required;

        const auto local_contrast_radius = stored_int_or_default(
            settings_
            , kRandomWalkerLocalContrastRadiusKey
            , default_settings.random_walker.local_contrast_scale.radius
        );
        settings.random_walker.local_contrast_scale.radius =
            local_contrast_radius.value;
        result.repair_required |= local_contrast_radius.repair_required;

        const auto local_contrast_minimum_variance = stored_double_or_default(
            settings_
            , kRandomWalkerLocalContrastMinimumVarianceKey
            , default_settings.random_walker
                .local_contrast_scale
                .minimum_variance
        );
        settings.random_walker.local_contrast_scale.minimum_variance =
            local_contrast_minimum_variance.value;
        result.repair_required |=
            local_contrast_minimum_variance.repair_required;

        const auto local_contrast_minimum_variance_mode =
            stored_minimum_variance_mode_or_default(
                settings_
                , kRandomWalkerLocalContrastMinimumVarianceModeKey
                , default_settings.random_walker
                    .local_contrast_scale
                    .minimum_variance_mode
            );
        settings.random_walker.local_contrast_scale.minimum_variance_mode =
            local_contrast_minimum_variance_mode.value;
        result.repair_required |=
            local_contrast_minimum_variance_mode.repair_required;

        const auto local_contrast_auto_quantile = stored_double_or_default(
            settings_
            , kRandomWalkerLocalContrastAutoQuantileKey
            , default_settings.random_walker
                .local_contrast_scale
                .auto_minimum_variance_quantile
        );
        settings.random_walker.local_contrast_scale
            .auto_minimum_variance_quantile =
                local_contrast_auto_quantile.value;
        result.repair_required |=
            local_contrast_auto_quantile.repair_required;

        const auto edge_local_contrast_radius = stored_int_or_default(
            settings_
            , kRandomWalkerEdgeLocalContrastRadiusKey
            , default_settings.random_walker.edge_local_contrast_scale.radius
        );
        settings.random_walker.edge_local_contrast_scale.radius =
            edge_local_contrast_radius.value;
        result.repair_required |= edge_local_contrast_radius.repair_required;

        const auto edge_local_contrast_quantile = stored_double_or_default(
            settings_
            , kRandomWalkerEdgeLocalContrastQuantileKey
            , default_settings.random_walker.edge_local_contrast_scale.quantile
        );
        settings.random_walker.edge_local_contrast_scale.quantile =
            edge_local_contrast_quantile.value;
        result.repair_required |= edge_local_contrast_quantile.repair_required;

        const auto edge_local_contrast_minimum_scale =
            stored_double_or_default(
                settings_
                , kRandomWalkerEdgeLocalContrastMinimumScaleKey
                , default_settings.random_walker
                    .edge_local_contrast_scale
                    .minimum_scale
            );
        settings.random_walker.edge_local_contrast_scale.minimum_scale =
            edge_local_contrast_minimum_scale.value;
        result.repair_required |=
            edge_local_contrast_minimum_scale.repair_required;

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
        case 4:
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
        case 5:
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
            result.random_walker.local_contrast_scale.minimum_variance_mode =
                stored_minimum_variance_mode(
                    settings_
                    , kRandomWalkerLocalContrastMinimumVarianceModeKey
                );
            result.random_walker.local_contrast_scale
                .auto_minimum_variance_quantile = stored_double(
                    settings_
                    , kRandomWalkerLocalContrastAutoQuantileKey
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
        settings_.setValue(
            kRandomWalkerLocalContrastMinimumVarianceModeKey
            , minimum_variance_mode_to_storage(
                application_settings.random_walker
                    .local_contrast_scale
                    .minimum_variance_mode
            )
        );
        settings_.setValue(
            kRandomWalkerLocalContrastAutoQuantileKey
            , application_settings.random_walker
                .local_contrast_scale
                .auto_minimum_variance_quantile
        );
        settings_.setValue(
            kRandomWalkerEdgeLocalContrastRadiusKey
            , application_settings.random_walker
                .edge_local_contrast_scale
                .radius
        );
        settings_.setValue(
            kRandomWalkerEdgeLocalContrastQuantileKey
            , application_settings.random_walker
                .edge_local_contrast_scale
                .quantile
        );
        settings_.setValue(
            kRandomWalkerEdgeLocalContrastMinimumScaleKey
            , application_settings.random_walker
                .edge_local_contrast_scale
                .minimum_scale
        );
        settings_.setValue(kSchemaVersionKey, kCurrentSchemaVersion);
    }
}

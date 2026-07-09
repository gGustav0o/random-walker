#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "application/settings/SettingsService.hpp"
#include "infrastructure/settings/QtSettingsRepository.hpp"

namespace {
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
    constexpr int kCurrentSchemaVersion = 6;

    class InMemorySettingsRepository final
        : public random_walker::application::SettingsRepository {
    public:
        [[nodiscard]] random_walker::application::SettingsRepositoryLoadResult
            load() const override {
            return {
                .settings = stored
                , .repair_required = repair_required
            };
        }

        [[nodiscard]] bool save(
            const random_walker::application::ApplicationSettings& settings)
            override {
            stored = settings;
            ++save_count;
            return save_succeeds;
        }

        mutable random_walker::application::ApplicationSettings stored;
        bool save_succeeds = true;
        bool repair_required = false;
        int save_count = 0;
    };

    [[nodiscard]] QString settings_path(const QTemporaryDir& directory) {
        return directory.filePath(QStringLiteral("settings.ini"));
    }

    void write_value(
        const QString& path
        , const QString& key
        , const QVariant& value
    ) {
        QSettings settings(path, QSettings::IniFormat);
        settings.beginGroup(kSettingsGroup);
        settings.setValue(key, value);
        settings.endGroup();
        settings.sync();
    }
}

class SettingsTests final : public QObject {
    Q_OBJECT

private slots:
    void loads_default_values();
    void repairs_corrupted_values();
    void repairs_missing_edge_local_contrast_values();
    void loads_persisted_parameters();
    void ignores_unknown_newer_schema();
    void migrates_legacy_schema();
    void migrates_schema_one_with_default_connectivity();
    void migrates_schema_two_with_default_distance_power();
    void migrates_schema_three_with_default_edge_weight_settings();
    void migrates_schema_four_with_default_minimum_variance_mode();
    void migrates_schema_five_with_default_edge_local_contrast_settings();
    void rejects_invalid_settings_on_save();
    void reports_settings_save_failure();
};

void SettingsTests::loads_default_values() {
    InMemorySettingsRepository repository;
    random_walker::application::SettingsService service(repository);

    const auto load_result = service.load();
    const auto settings = load_result.settings;

    QCOMPARE(
        settings.random_walker.beta
        , random_walker::domain::kDefaultRandomWalkerBeta
    );
    QCOMPARE(
        static_cast<int>(settings.random_walker.connectivity)
        , static_cast<int>(random_walker::domain::kDefaultPixelConnectivity)
    );
    QCOMPARE(
        settings.random_walker.distance_power
        , random_walker::domain::kDefaultRandomWalkerDistancePower
    );
    QCOMPARE(
        static_cast<int>(settings.random_walker.edge_weight_model)
        , static_cast<int>(random_walker::domain::kDefaultEdgeWeightModel)
    );
    QCOMPARE(
        settings.random_walker.local_contrast_scale.radius
        , random_walker::domain::kDefaultLocalContrastRadius
    );
    QCOMPARE(
        settings.random_walker.local_contrast_scale.minimum_variance
        , random_walker::domain::kDefaultLocalContrastMinimumVariance
    );
    QCOMPARE(
        static_cast<int>(
            settings.random_walker.local_contrast_scale.minimum_variance_mode
        )
        , static_cast<int>(random_walker::domain::kDefaultMinimumVarianceMode)
    );
    QCOMPARE(
        settings.random_walker.local_contrast_scale
            .auto_minimum_variance_quantile
        , random_walker::domain::kDefaultLocalContrastAutoQuantile
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.radius
        , random_walker::domain::kDefaultEdgeLocalContrastRadius
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.quantile
        , random_walker::domain::kDefaultEdgeLocalContrastQuantile
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.minimum_scale
        , random_walker::domain::kDefaultEdgeLocalContrastMinimumScale
    );
    QVERIFY(repository.stored == settings);
    QVERIFY(!load_result.repair_required);
    QCOMPARE(repository.save_count, 0);
}

void SettingsTests::repairs_corrupted_values() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = settings_path(directory);
    write_value(path, kSchemaVersionKey, kCurrentSchemaVersion);
    write_value(path, kRandomWalkerBetaKey, QStringLiteral("invalid"));
    write_value(path, kRandomWalkerConnectivityKey, QStringLiteral("eight"));
    write_value(path, kRandomWalkerDistancePowerKey, QStringLiteral("invalid"));
    write_value(path, kRandomWalkerEdgeWeightModelKey, QStringLiteral("invalid"));
    write_value(path, kRandomWalkerLocalContrastRadiusKey, QStringLiteral("invalid"));
    write_value(
        path
        , kRandomWalkerLocalContrastMinimumVarianceKey
        , QStringLiteral("invalid")
    );
    write_value(
        path
        , kRandomWalkerLocalContrastMinimumVarianceModeKey
        , QStringLiteral("invalid")
    );
    write_value(
        path
        , kRandomWalkerLocalContrastAutoQuantileKey
        , QStringLiteral("invalid")
    );
    write_value(
        path
        , kRandomWalkerEdgeLocalContrastRadiusKey
        , QStringLiteral("invalid")
    );
    write_value(
        path
        , kRandomWalkerEdgeLocalContrastQuantileKey
        , QStringLiteral("invalid")
    );
    write_value(
        path
        , kRandomWalkerEdgeLocalContrastMinimumScaleKey
        , QStringLiteral("invalid")
    );

    random_walker::infrastructure::QtSettingsRepository repository(
        path
        , QSettings::IniFormat
    );
    random_walker::application::SettingsService service(repository);

    const auto load_result = service.load();
    const auto settings = load_result.settings;

    QCOMPARE(
        settings.random_walker.beta
        , random_walker::domain::kDefaultRandomWalkerBeta
    );
    QCOMPARE(
        static_cast<int>(settings.random_walker.connectivity)
        , static_cast<int>(random_walker::domain::kDefaultPixelConnectivity)
    );
    QCOMPARE(
        settings.random_walker.distance_power
        , random_walker::domain::kDefaultRandomWalkerDistancePower
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.radius
        , random_walker::domain::kDefaultEdgeLocalContrastRadius
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.quantile
        , random_walker::domain::kDefaultEdgeLocalContrastQuantile
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.minimum_scale
        , random_walker::domain::kDefaultEdgeLocalContrastMinimumScale
    );
    QVERIFY(load_result.repair_required);

    {
        QSettings persisted(path, QSettings::IniFormat);
        persisted.beginGroup(kSettingsGroup);
        QCOMPARE(
            persisted.value(kRandomWalkerBetaKey).toString()
            , QStringLiteral("invalid")
        );
    }

    const auto repair_outcome = service.save(settings);
    QVERIFY(!repair_outcome.has_value());

    QSettings repaired(path, QSettings::IniFormat);
    repaired.beginGroup(kSettingsGroup);
    QCOMPARE(
        repaired.value(kRandomWalkerBetaKey).toDouble()
        , random_walker::domain::kDefaultRandomWalkerBeta
    );
    QCOMPARE(
        repaired.value(kRandomWalkerConnectivityKey).toString()
        , QStringLiteral("four")
    );
    QCOMPARE(
        repaired.value(kRandomWalkerDistancePowerKey).toDouble()
        , random_walker::domain::kDefaultRandomWalkerDistancePower
    );
    QCOMPARE(
        repaired.value(kRandomWalkerEdgeWeightModelKey).toString()
        , QStringLiteral("globalBeta")
    );
    QCOMPARE(
        repaired.value(kRandomWalkerLocalContrastRadiusKey).toInt()
        , random_walker::domain::kDefaultLocalContrastRadius
    );
    QCOMPARE(
        repaired.value(kRandomWalkerLocalContrastMinimumVarianceKey).toDouble()
        , random_walker::domain::kDefaultLocalContrastMinimumVariance
    );
    QCOMPARE(
        repaired.value(kRandomWalkerLocalContrastMinimumVarianceModeKey).toString()
        , QStringLiteral("manual")
    );
    QCOMPARE(
        repaired.value(kRandomWalkerLocalContrastAutoQuantileKey).toDouble()
        , random_walker::domain::kDefaultLocalContrastAutoQuantile
    );
    QCOMPARE(
        repaired.value(kRandomWalkerEdgeLocalContrastRadiusKey).toInt()
        , random_walker::domain::kDefaultEdgeLocalContrastRadius
    );
    QCOMPARE(
        repaired.value(kRandomWalkerEdgeLocalContrastQuantileKey).toDouble()
        , random_walker::domain::kDefaultEdgeLocalContrastQuantile
    );
    QCOMPARE(
        repaired.value(kRandomWalkerEdgeLocalContrastMinimumScaleKey).toDouble()
        , random_walker::domain::kDefaultEdgeLocalContrastMinimumScale
    );
}

void SettingsTests::repairs_missing_edge_local_contrast_values() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = settings_path(directory);
    write_value(path, kSchemaVersionKey, kCurrentSchemaVersion);
    write_value(path, kRandomWalkerBetaKey, 0.005);
    write_value(path, kRandomWalkerConnectivityKey, QStringLiteral("eight"));
    write_value(path, kRandomWalkerDistancePowerKey, 1.5);
    write_value(
        path
        , kRandomWalkerEdgeWeightModelKey
        , QStringLiteral("localVarianceNormalized")
    );
    write_value(path, kRandomWalkerLocalContrastRadiusKey, 3);
    write_value(path, kRandomWalkerLocalContrastMinimumVarianceKey, 4.5);
    write_value(
        path
        , kRandomWalkerLocalContrastMinimumVarianceModeKey
        , QStringLiteral("auto")
    );
    write_value(path, kRandomWalkerLocalContrastAutoQuantileKey, 0.25);

    random_walker::infrastructure::QtSettingsRepository repository(
        path
        , QSettings::IniFormat
    );

    const auto load_result = repository.load();
    const auto settings = load_result.settings;

    QCOMPARE(settings.random_walker.beta, 0.005);
    QCOMPARE(
        static_cast<int>(settings.random_walker.connectivity)
        , static_cast<int>(random_walker::domain::PixelConnectivity::Eight)
    );
    QCOMPARE(settings.random_walker.distance_power, 1.5);
    QCOMPARE(
        static_cast<int>(settings.random_walker.edge_weight_model)
        , static_cast<int>(
            random_walker::domain::EdgeWeightModel::LocalVarianceNormalized
        )
    );
    QCOMPARE(settings.random_walker.local_contrast_scale.radius, 3);
    QCOMPARE(settings.random_walker.local_contrast_scale.minimum_variance, 4.5);
    QCOMPARE(
        static_cast<int>(
            settings.random_walker.local_contrast_scale.minimum_variance_mode
        )
        , static_cast<int>(random_walker::domain::MinimumVarianceMode::Auto)
    );
    QCOMPARE(
        settings.random_walker.local_contrast_scale
            .auto_minimum_variance_quantile
        , 0.25
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.radius
        , random_walker::domain::kDefaultEdgeLocalContrastRadius
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.quantile
        , random_walker::domain::kDefaultEdgeLocalContrastQuantile
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.minimum_scale
        , random_walker::domain::kDefaultEdgeLocalContrastMinimumScale
    );
    QVERIFY(load_result.repair_required);
}

void SettingsTests::loads_persisted_parameters() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = settings_path(directory);
    write_value(path, kSchemaVersionKey, kCurrentSchemaVersion);
    write_value(path, kRandomWalkerBetaKey, 0.005);
    write_value(path, kRandomWalkerConnectivityKey, QStringLiteral("eight"));
    write_value(path, kRandomWalkerDistancePowerKey, 1.5);
    write_value(
        path
        , kRandomWalkerEdgeWeightModelKey
        , QStringLiteral("edgeLocalContrastNormalized")
    );
    write_value(path, kRandomWalkerLocalContrastRadiusKey, 3);
    write_value(path, kRandomWalkerLocalContrastMinimumVarianceKey, 4.5);
    write_value(
        path
        , kRandomWalkerLocalContrastMinimumVarianceModeKey
        , QStringLiteral("auto")
    );
    write_value(path, kRandomWalkerLocalContrastAutoQuantileKey, 0.25);
    write_value(path, kRandomWalkerEdgeLocalContrastRadiusKey, 4);
    write_value(path, kRandomWalkerEdgeLocalContrastQuantileKey, 0.35);
    write_value(path, kRandomWalkerEdgeLocalContrastMinimumScaleKey, 2.5);

    random_walker::infrastructure::QtSettingsRepository repository(
        path
        , QSettings::IniFormat
    );

    const auto load_result = repository.load();
    const auto settings = load_result.settings;

    QCOMPARE(settings.random_walker.beta, 0.005);
    QCOMPARE(
        static_cast<int>(settings.random_walker.connectivity)
        , static_cast<int>(random_walker::domain::PixelConnectivity::Eight)
    );
    QCOMPARE(settings.random_walker.distance_power, 1.5);
    QCOMPARE(
        static_cast<int>(settings.random_walker.edge_weight_model)
        , static_cast<int>(
            random_walker::domain::EdgeWeightModel::EdgeLocalContrastNormalized
        )
    );
    QCOMPARE(settings.random_walker.local_contrast_scale.radius, 3);
    QCOMPARE(settings.random_walker.local_contrast_scale.minimum_variance, 4.5);
    QCOMPARE(
        static_cast<int>(
            settings.random_walker.local_contrast_scale.minimum_variance_mode
        )
        , static_cast<int>(random_walker::domain::MinimumVarianceMode::Auto)
    );
    QCOMPARE(
        settings.random_walker.local_contrast_scale
            .auto_minimum_variance_quantile
        , 0.25
    );
    QCOMPARE(settings.random_walker.edge_local_contrast_scale.radius, 4);
    QCOMPARE(settings.random_walker.edge_local_contrast_scale.quantile, 0.35);
    QCOMPARE(settings.random_walker.edge_local_contrast_scale.minimum_scale, 2.5);
    QVERIFY(!load_result.repair_required);
}

void SettingsTests::ignores_unknown_newer_schema() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = settings_path(directory);
    write_value(path, kSchemaVersionKey, kCurrentSchemaVersion + 1);
    write_value(path, kRandomWalkerBetaKey, 0.005);
    write_value(path, kRandomWalkerConnectivityKey, QStringLiteral("eight"));
    write_value(path, kRandomWalkerDistancePowerKey, 1.5);
    write_value(
        path
        , kRandomWalkerEdgeWeightModelKey
        , QStringLiteral("edgeLocalContrastNormalized")
    );
    write_value(path, kRandomWalkerLocalContrastRadiusKey, 3);
    write_value(path, kRandomWalkerLocalContrastMinimumVarianceKey, 4.5);
    write_value(
        path
        , kRandomWalkerLocalContrastMinimumVarianceModeKey
        , QStringLiteral("auto")
    );
    write_value(path, kRandomWalkerLocalContrastAutoQuantileKey, 0.25);
    write_value(path, kRandomWalkerEdgeLocalContrastRadiusKey, 4);
    write_value(path, kRandomWalkerEdgeLocalContrastQuantileKey, 0.35);
    write_value(path, kRandomWalkerEdgeLocalContrastMinimumScaleKey, 2.5);

    random_walker::infrastructure::QtSettingsRepository repository(
        path
        , QSettings::IniFormat
    );

    const auto load_result = repository.load();
    const auto settings = load_result.settings;

    QCOMPARE(
        settings.random_walker.beta
        , random_walker::domain::kDefaultRandomWalkerBeta
    );
    QCOMPARE(
        static_cast<int>(settings.random_walker.connectivity)
        , static_cast<int>(random_walker::domain::kDefaultPixelConnectivity)
    );
    QCOMPARE(
        settings.random_walker.distance_power
        , random_walker::domain::kDefaultRandomWalkerDistancePower
    );
    QVERIFY(!load_result.repair_required);

    QSettings persisted(path, QSettings::IniFormat);
    persisted.beginGroup(kSettingsGroup);
    QCOMPARE(
        persisted.value(kSchemaVersionKey).toInt()
        , kCurrentSchemaVersion + 1
    );
}

void SettingsTests::migrates_legacy_schema() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = settings_path(directory);
    constexpr double legacy_beta = 0.004;
    write_value(path, kSchemaVersionKey, 0);
    write_value(path, kLegacyBetaKey, legacy_beta);

    random_walker::infrastructure::QtSettingsRepository repository(
        path
        , QSettings::IniFormat
    );

    const auto load_result = repository.load();
    const auto settings = load_result.settings;

    QCOMPARE(settings.random_walker.beta, legacy_beta);
    QCOMPARE(
        static_cast<int>(settings.random_walker.connectivity)
        , static_cast<int>(random_walker::domain::kDefaultPixelConnectivity)
    );
    QCOMPARE(
        settings.random_walker.distance_power
        , random_walker::domain::kDefaultRandomWalkerDistancePower
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.radius
        , random_walker::domain::kDefaultEdgeLocalContrastRadius
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.quantile
        , random_walker::domain::kDefaultEdgeLocalContrastQuantile
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.minimum_scale
        , random_walker::domain::kDefaultEdgeLocalContrastMinimumScale
    );
    QVERIFY(load_result.repair_required);

    {
        QSettings persisted(path, QSettings::IniFormat);
        persisted.beginGroup(kSettingsGroup);
        QCOMPARE(
            persisted.value(kSchemaVersionKey).toInt()
            , 0
        );
        QVERIFY(!persisted.contains(kRandomWalkerBetaKey));
        QVERIFY(!persisted.contains(kRandomWalkerConnectivityKey));
        QVERIFY(!persisted.contains(kRandomWalkerDistancePowerKey));
        QVERIFY(persisted.contains(kLegacyBetaKey));
    }

    QVERIFY(repository.save(settings));

    QSettings migrated(path, QSettings::IniFormat);
    migrated.beginGroup(kSettingsGroup);
    QCOMPARE(
        migrated.value(kSchemaVersionKey).toInt()
        , kCurrentSchemaVersion
    );
    QCOMPARE(
        migrated.value(kRandomWalkerBetaKey).toDouble()
        , legacy_beta
    );
    QCOMPARE(
        migrated.value(kRandomWalkerConnectivityKey).toString()
        , QStringLiteral("four")
    );
    QCOMPARE(
        migrated.value(kRandomWalkerDistancePowerKey).toDouble()
        , random_walker::domain::kDefaultRandomWalkerDistancePower
    );
    QVERIFY(!migrated.contains(kLegacyBetaKey));
}

void SettingsTests::migrates_schema_one_with_default_connectivity() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = settings_path(directory);
    constexpr double stored_beta = 0.006;
    write_value(path, kSchemaVersionKey, 1);
    write_value(path, kRandomWalkerBetaKey, stored_beta);

    random_walker::infrastructure::QtSettingsRepository repository(
        path
        , QSettings::IniFormat
    );

    const auto load_result = repository.load();
    const auto settings = load_result.settings;

    QCOMPARE(settings.random_walker.beta, stored_beta);
    QCOMPARE(
        static_cast<int>(settings.random_walker.connectivity)
        , static_cast<int>(random_walker::domain::PixelConnectivity::Four)
    );
    QCOMPARE(
        settings.random_walker.distance_power
        , random_walker::domain::kDefaultRandomWalkerDistancePower
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.radius
        , random_walker::domain::kDefaultEdgeLocalContrastRadius
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.quantile
        , random_walker::domain::kDefaultEdgeLocalContrastQuantile
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.minimum_scale
        , random_walker::domain::kDefaultEdgeLocalContrastMinimumScale
    );
    QVERIFY(load_result.repair_required);
}

void SettingsTests::migrates_schema_two_with_default_distance_power() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = settings_path(directory);
    constexpr double stored_beta = 0.006;
    write_value(path, kSchemaVersionKey, 2);
    write_value(path, kRandomWalkerBetaKey, stored_beta);
    write_value(path, kRandomWalkerConnectivityKey, QStringLiteral("eight"));

    random_walker::infrastructure::QtSettingsRepository repository(
        path
        , QSettings::IniFormat
    );

    const auto load_result = repository.load();
    const auto settings = load_result.settings;

    QCOMPARE(settings.random_walker.beta, stored_beta);
    QCOMPARE(
        static_cast<int>(settings.random_walker.connectivity)
        , static_cast<int>(random_walker::domain::PixelConnectivity::Eight)
    );
    QCOMPARE(
        settings.random_walker.distance_power
        , random_walker::domain::kDefaultRandomWalkerDistancePower
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.radius
        , random_walker::domain::kDefaultEdgeLocalContrastRadius
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.quantile
        , random_walker::domain::kDefaultEdgeLocalContrastQuantile
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.minimum_scale
        , random_walker::domain::kDefaultEdgeLocalContrastMinimumScale
    );
    QVERIFY(load_result.repair_required);
}


void SettingsTests::migrates_schema_three_with_default_edge_weight_settings() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = settings_path(directory);
    constexpr double stored_beta = 0.006;
    write_value(path, kSchemaVersionKey, 3);
    write_value(path, kRandomWalkerBetaKey, stored_beta);
    write_value(path, kRandomWalkerConnectivityKey, QStringLiteral("eight"));
    write_value(path, kRandomWalkerDistancePowerKey, 1.5);

    random_walker::infrastructure::QtSettingsRepository repository(
        path
        , QSettings::IniFormat
    );

    const auto load_result = repository.load();
    const auto settings = load_result.settings;

    QCOMPARE(settings.random_walker.beta, stored_beta);
    QCOMPARE(
        static_cast<int>(settings.random_walker.connectivity)
        , static_cast<int>(random_walker::domain::PixelConnectivity::Eight)
    );
    QCOMPARE(settings.random_walker.distance_power, 1.5);
    QCOMPARE(
        static_cast<int>(settings.random_walker.edge_weight_model)
        , static_cast<int>(random_walker::domain::kDefaultEdgeWeightModel)
    );
    QCOMPARE(
        settings.random_walker.local_contrast_scale.radius
        , random_walker::domain::kDefaultLocalContrastRadius
    );
    QCOMPARE(
        settings.random_walker.local_contrast_scale.minimum_variance
        , random_walker::domain::kDefaultLocalContrastMinimumVariance
    );
    QCOMPARE(
        static_cast<int>(
            settings.random_walker.local_contrast_scale.minimum_variance_mode
        )
        , static_cast<int>(random_walker::domain::kDefaultMinimumVarianceMode)
    );
    QCOMPARE(
        settings.random_walker.local_contrast_scale
            .auto_minimum_variance_quantile
        , random_walker::domain::kDefaultLocalContrastAutoQuantile
    );
    QVERIFY(load_result.repair_required);
}

void SettingsTests::migrates_schema_four_with_default_minimum_variance_mode() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = settings_path(directory);
    constexpr double stored_beta = 0.006;
    write_value(path, kSchemaVersionKey, 4);
    write_value(path, kRandomWalkerBetaKey, stored_beta);
    write_value(path, kRandomWalkerConnectivityKey, QStringLiteral("eight"));
    write_value(path, kRandomWalkerDistancePowerKey, 1.5);
    write_value(
        path
        , kRandomWalkerEdgeWeightModelKey
        , QStringLiteral("localVarianceNormalized")
    );
    write_value(path, kRandomWalkerLocalContrastRadiusKey, 3);
    write_value(path, kRandomWalkerLocalContrastMinimumVarianceKey, 4.5);

    random_walker::infrastructure::QtSettingsRepository repository(
        path
        , QSettings::IniFormat
    );

    const auto load_result = repository.load();
    const auto settings = load_result.settings;

    QCOMPARE(settings.random_walker.beta, stored_beta);
    QCOMPARE(
        static_cast<int>(settings.random_walker.connectivity)
        , static_cast<int>(random_walker::domain::PixelConnectivity::Eight)
    );
    QCOMPARE(settings.random_walker.distance_power, 1.5);
    QCOMPARE(
        static_cast<int>(settings.random_walker.edge_weight_model)
        , static_cast<int>(
            random_walker::domain::EdgeWeightModel::LocalVarianceNormalized
        )
    );
    QCOMPARE(settings.random_walker.local_contrast_scale.radius, 3);
    QCOMPARE(settings.random_walker.local_contrast_scale.minimum_variance, 4.5);
    QCOMPARE(
        static_cast<int>(
            settings.random_walker.local_contrast_scale.minimum_variance_mode
        )
        , static_cast<int>(random_walker::domain::kDefaultMinimumVarianceMode)
    );
    QCOMPARE(
        settings.random_walker.local_contrast_scale
            .auto_minimum_variance_quantile
        , random_walker::domain::kDefaultLocalContrastAutoQuantile
    );
    QVERIFY(load_result.repair_required);
}

void SettingsTests::migrates_schema_five_with_default_edge_local_contrast_settings() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = settings_path(directory);
    write_value(path, kSchemaVersionKey, 5);
    write_value(path, kRandomWalkerBetaKey, 0.006);
    write_value(path, kRandomWalkerConnectivityKey, QStringLiteral("eight"));
    write_value(path, kRandomWalkerDistancePowerKey, 1.5);
    write_value(
        path
        , kRandomWalkerEdgeWeightModelKey
        , QStringLiteral("localVarianceNormalized")
    );
    write_value(path, kRandomWalkerLocalContrastRadiusKey, 3);
    write_value(path, kRandomWalkerLocalContrastMinimumVarianceKey, 4.5);
    write_value(
        path
        , kRandomWalkerLocalContrastMinimumVarianceModeKey
        , QStringLiteral("auto")
    );
    write_value(path, kRandomWalkerLocalContrastAutoQuantileKey, 0.25);

    random_walker::infrastructure::QtSettingsRepository repository(
        path
        , QSettings::IniFormat
    );

    const auto load_result = repository.load();
    const auto settings = load_result.settings;

    QCOMPARE(settings.random_walker.beta, 0.006);
    QCOMPARE(
        static_cast<int>(settings.random_walker.connectivity)
        , static_cast<int>(random_walker::domain::PixelConnectivity::Eight)
    );
    QCOMPARE(settings.random_walker.distance_power, 1.5);
    QCOMPARE(
        static_cast<int>(settings.random_walker.edge_weight_model)
        , static_cast<int>(
            random_walker::domain::EdgeWeightModel::LocalVarianceNormalized
        )
    );
    QCOMPARE(settings.random_walker.local_contrast_scale.radius, 3);
    QCOMPARE(settings.random_walker.local_contrast_scale.minimum_variance, 4.5);
    QCOMPARE(
        static_cast<int>(
            settings.random_walker.local_contrast_scale.minimum_variance_mode
        )
        , static_cast<int>(random_walker::domain::MinimumVarianceMode::Auto)
    );
    QCOMPARE(
        settings.random_walker.local_contrast_scale
            .auto_minimum_variance_quantile
        , 0.25
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.radius
        , random_walker::domain::kDefaultEdgeLocalContrastRadius
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.quantile
        , random_walker::domain::kDefaultEdgeLocalContrastQuantile
    );
    QCOMPARE(
        settings.random_walker.edge_local_contrast_scale.minimum_scale
        , random_walker::domain::kDefaultEdgeLocalContrastMinimumScale
    );
    QVERIFY(load_result.repair_required);
}

void SettingsTests::rejects_invalid_settings_on_save() {
    InMemorySettingsRepository repository;
    random_walker::application::SettingsService service(repository);

    random_walker::application::ApplicationSettings settings;
    settings.random_walker.beta =
        random_walker::domain::kMaximumRandomWalkerBeta * 2.0;

    const auto outcome = service.save(settings);

    QVERIFY(outcome.has_value());
    QVERIFY(
        *outcome == random_walker::application::SettingsError::InvalidSettings
    );
    QCOMPARE(repository.save_count, 0);
}

void SettingsTests::reports_settings_save_failure() {
    InMemorySettingsRepository repository;
    repository.save_succeeds = false;
    random_walker::application::SettingsService service(repository);

    random_walker::application::ApplicationSettings settings;

    const auto outcome = service.save(settings);

    QVERIFY(outcome.has_value());
    QVERIFY(
        *outcome == random_walker::application::SettingsError::SaveFailed
    );
    QCOMPARE(repository.save_count, 1);
}

QTEST_GUILESS_MAIN(SettingsTests)

#include "SettingsTests.moc"

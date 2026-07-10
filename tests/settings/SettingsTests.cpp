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
    constexpr auto kAutoMarkersConfidenceThresholdKey =
        "autoMarkers/confidenceThreshold";
    constexpr auto kAutoMarkersMinimumComponentAreaKey =
        "autoMarkers/minimumComponentArea";
    constexpr auto kAutoMarkersMinimumBoundaryDistanceKey =
        "autoMarkers/minimumBoundaryDistance";
    constexpr auto kAutoMarkersForegroundPolarityKey =
        "autoMarkers/foregroundPolarity";
    constexpr auto kLegacyBetaKey = "beta";
    constexpr auto kRemovedEdgeWeightModelKey = "randomWalker/edgeWeightModel";
    constexpr auto kRemovedLocalContrastRadiusKey =
        "randomWalker/localContrast/radius";
    constexpr auto kRemovedEdgeLocalContrastRadiusKey =
        "randomWalker/edgeLocalContrast/radius";
    constexpr int kCurrentSchemaVersion = 8;

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
            const random_walker::application::ApplicationSettings& settings
        ) override {
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

    void compare_default_auto_markers(
        const random_walker::application::ApplicationSettings& settings
    ) {
        const random_walker::domain::AutoMarkerParameters default_parameters;

        QCOMPARE(
            settings.auto_markers.confidence_threshold
            , random_walker::domain::kDefaultAutoMarkerConfidenceThreshold
        );
        QCOMPARE(
            settings.auto_markers.minimum_component_area
            , random_walker::domain::kDefaultAutoMarkerComponentArea
        );
        QCOMPARE(
            settings.auto_markers.minimum_boundary_distance
            , random_walker::domain::kDefaultAutoMarkerBoundaryDistance
        );
        QCOMPARE(
            static_cast<int>(settings.auto_markers.foreground_polarity)
            , static_cast<int>(default_parameters.foreground_polarity)
        );
    }
}

class SettingsTests final : public QObject {
    Q_OBJECT

private slots:
    void loads_default_values();
    void repairs_corrupted_values();
    void loads_persisted_parameters();
    void ignores_unknown_newer_schema();
    void migrates_legacy_schema();
    void migrates_schema_one_with_default_connectivity();
    void migrates_schema_two_with_default_distance_power();
    void drops_removed_weight_settings_on_save();
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
    compare_default_auto_markers(settings);
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
    write_value(path, kRandomWalkerConnectivityKey, QStringLiteral("invalid"));
    write_value(path, kRandomWalkerDistancePowerKey, QStringLiteral("invalid"));
    write_value(
        path
        , kAutoMarkersConfidenceThresholdKey
        , QStringLiteral("invalid")
    );
    write_value(
        path
        , kAutoMarkersMinimumComponentAreaKey
        , QStringLiteral("invalid")
    );
    write_value(
        path
        , kAutoMarkersMinimumBoundaryDistanceKey
        , QStringLiteral("invalid")
    );
    write_value(
        path
        , kAutoMarkersForegroundPolarityKey
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
    compare_default_auto_markers(settings);
    QVERIFY(load_result.repair_required);

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
        repaired.value(kAutoMarkersConfidenceThresholdKey).toDouble()
        , random_walker::domain::kDefaultAutoMarkerConfidenceThreshold
    );
    QCOMPARE(
        repaired.value(kAutoMarkersMinimumComponentAreaKey).toInt()
        , random_walker::domain::kDefaultAutoMarkerComponentArea
    );
    QCOMPARE(
        repaired.value(kAutoMarkersMinimumBoundaryDistanceKey).toInt()
        , random_walker::domain::kDefaultAutoMarkerBoundaryDistance
    );
    QCOMPARE(
        repaired.value(kAutoMarkersForegroundPolarityKey).toString()
        , QStringLiteral("bright")
    );
    QCOMPARE(repaired.value(kSchemaVersionKey).toInt(), kCurrentSchemaVersion);
}

void SettingsTests::loads_persisted_parameters() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = settings_path(directory);
    write_value(path, kSchemaVersionKey, kCurrentSchemaVersion);
    write_value(path, kRandomWalkerBetaKey, 0.005);
    write_value(path, kRandomWalkerConnectivityKey, QStringLiteral("eight"));
    write_value(path, kRandomWalkerDistancePowerKey, 1.5);
    write_value(path, kAutoMarkersConfidenceThresholdKey, 0.75);
    write_value(path, kAutoMarkersMinimumComponentAreaKey, 32);
    write_value(path, kAutoMarkersMinimumBoundaryDistanceKey, 5);
    write_value(path, kAutoMarkersForegroundPolarityKey, QStringLiteral("dark"));

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
    QCOMPARE(settings.auto_markers.confidence_threshold, 0.75);
    QCOMPARE(settings.auto_markers.minimum_component_area, 32);
    QCOMPARE(settings.auto_markers.minimum_boundary_distance, 5);
    QCOMPARE(
        static_cast<int>(settings.auto_markers.foreground_polarity)
        , static_cast<int>(random_walker::domain::ForegroundPolarity::DarkObject)
    );
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
    write_value(path, kAutoMarkersConfidenceThresholdKey, 0.75);
    write_value(path, kAutoMarkersMinimumComponentAreaKey, 32);
    write_value(path, kAutoMarkersMinimumBoundaryDistanceKey, 5);
    write_value(path, kAutoMarkersForegroundPolarityKey, QStringLiteral("dark"));

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
    compare_default_auto_markers(settings);
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
    compare_default_auto_markers(settings);
    QVERIFY(load_result.repair_required);

    QVERIFY(repository.save(settings));

    QSettings migrated(path, QSettings::IniFormat);
    migrated.beginGroup(kSettingsGroup);
    QCOMPARE(migrated.value(kSchemaVersionKey).toInt(), kCurrentSchemaVersion);
    QCOMPARE(migrated.value(kRandomWalkerBetaKey).toDouble(), legacy_beta);
    QCOMPARE(
        migrated.value(kRandomWalkerConnectivityKey).toString()
        , QStringLiteral("four")
    );
    QCOMPARE(
        migrated.value(kRandomWalkerDistancePowerKey).toDouble()
        , random_walker::domain::kDefaultRandomWalkerDistancePower
    );
    QCOMPARE(
        migrated.value(kAutoMarkersConfidenceThresholdKey).toDouble()
        , random_walker::domain::kDefaultAutoMarkerConfidenceThreshold
    );
    QCOMPARE(
        migrated.value(kAutoMarkersMinimumComponentAreaKey).toInt()
        , random_walker::domain::kDefaultAutoMarkerComponentArea
    );
    QCOMPARE(
        migrated.value(kAutoMarkersMinimumBoundaryDistanceKey).toInt()
        , random_walker::domain::kDefaultAutoMarkerBoundaryDistance
    );
    QCOMPARE(
        migrated.value(kAutoMarkersForegroundPolarityKey).toString()
        , QStringLiteral("bright")
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
    compare_default_auto_markers(settings);
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
    compare_default_auto_markers(settings);
    QVERIFY(load_result.repair_required);
}

void SettingsTests::drops_removed_weight_settings_on_save() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = settings_path(directory);
    write_value(path, kSchemaVersionKey, 6);
    write_value(path, kRandomWalkerBetaKey, 0.006);
    write_value(path, kRandomWalkerConnectivityKey, QStringLiteral("eight"));
    write_value(path, kRandomWalkerDistancePowerKey, 1.5);
    write_value(
        path
        , kRemovedEdgeWeightModelKey
        , QStringLiteral("edgeLocalContrastNormalized")
    );
    write_value(path, kRemovedLocalContrastRadiusKey, 3);
    write_value(path, kRemovedEdgeLocalContrastRadiusKey, 4);

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
    compare_default_auto_markers(settings);
    QVERIFY(load_result.repair_required);

    QVERIFY(repository.save(settings));

    QSettings migrated(path, QSettings::IniFormat);
    migrated.beginGroup(kSettingsGroup);
    QVERIFY(!migrated.contains(kRemovedEdgeWeightModelKey));
    QVERIFY(!migrated.contains(kRemovedLocalContrastRadiusKey));
    QVERIFY(!migrated.contains(kRemovedEdgeLocalContrastRadiusKey));
    QCOMPARE(
        migrated.value(kAutoMarkersConfidenceThresholdKey).toDouble()
        , random_walker::domain::kDefaultAutoMarkerConfidenceThreshold
    );
    QCOMPARE(
        migrated.value(kAutoMarkersMinimumComponentAreaKey).toInt()
        , random_walker::domain::kDefaultAutoMarkerComponentArea
    );
    QCOMPARE(
        migrated.value(kAutoMarkersMinimumBoundaryDistanceKey).toInt()
        , random_walker::domain::kDefaultAutoMarkerBoundaryDistance
    );
    QCOMPARE(
        migrated.value(kAutoMarkersForegroundPolarityKey).toString()
        , QStringLiteral("bright")
    );
    QCOMPARE(migrated.value(kSchemaVersionKey).toInt(), kCurrentSchemaVersion);
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

    settings = {};
    settings.auto_markers.confidence_threshold =
        random_walker::domain::kMaximumAutoMarkerConfidenceThreshold + 1.0;

    const auto auto_marker_outcome = service.save(settings);

    QVERIFY(auto_marker_outcome.has_value());
    QVERIFY(
        *auto_marker_outcome
        == random_walker::application::SettingsError::InvalidSettings
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

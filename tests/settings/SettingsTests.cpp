#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "application/settings/SettingsService.hpp"
#include "infrastructure/settings/QSettingsRepository.hpp"

namespace {
    constexpr auto kSettingsGroup = "applicationSettings";
    constexpr auto kSchemaVersionKey = "schemaVersion";
    constexpr auto kRandomWalkerBetaKey = "randomWalker/beta";
    constexpr auto kLegacyBetaKey = "beta";
    constexpr int kCurrentSchemaVersion = 1;

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
    void ignores_unknown_newer_schema();
    void migrates_legacy_schema();
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

    random_walker::infrastructure::QSettingsRepository repository(
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
}

void SettingsTests::ignores_unknown_newer_schema() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = settings_path(directory);
    write_value(path, kSchemaVersionKey, kCurrentSchemaVersion + 1);
    write_value(path, kRandomWalkerBetaKey, 0.005);

    random_walker::infrastructure::QSettingsRepository repository(
        path
        , QSettings::IniFormat
    );

    const auto load_result = repository.load();
    const auto settings = load_result.settings;

    QCOMPARE(
        settings.random_walker.beta
        , random_walker::domain::kDefaultRandomWalkerBeta
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

    random_walker::infrastructure::QSettingsRepository repository(
        path
        , QSettings::IniFormat
    );

    const auto load_result = repository.load();
    const auto settings = load_result.settings;

    QCOMPARE(settings.random_walker.beta, legacy_beta);
    QVERIFY(load_result.repair_required);

    {
        QSettings persisted(path, QSettings::IniFormat);
        persisted.beginGroup(kSettingsGroup);
        QCOMPARE(
            persisted.value(kSchemaVersionKey).toInt()
            , 0
        );
        QVERIFY(!persisted.contains(kRandomWalkerBetaKey));
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
    QVERIFY(!migrated.contains(kLegacyBetaKey));
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

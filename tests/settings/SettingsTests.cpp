#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "application/settings/SettingsService.hpp"
#include "infrastructure/settings/QSettingsRepository.hpp"

namespace
{
    constexpr auto kSettingsGroup = "applicationSettings";
    constexpr auto kSchemaVersionKey = "schemaVersion";
    constexpr auto kRandomWalkerBetaKey = "randomWalker/beta";
    constexpr auto kLegacyBetaKey = "beta";
    constexpr int kCurrentSchemaVersion = 1;

    class InMemorySettingsRepository final
        : public random_walker::application::SettingsRepository
    {
    public:
        [[nodiscard]] random_walker::application::ApplicationSettings
            load() const override
        {
            return stored;
        }

        void save(
            const random_walker::application::ApplicationSettings& settings)
            override
        {
            stored = settings;
            ++save_count;
        }

        mutable random_walker::application::ApplicationSettings stored;
        int save_count = 0;
    };

    [[nodiscard]] QString settings_path(const QTemporaryDir& directory)
    {
        return directory.filePath(QStringLiteral("settings.ini"));
    }

    void write_value(
        const QString& path,
        const QString& key,
        const QVariant& value)
    {
        QSettings settings(path, QSettings::IniFormat);
        settings.beginGroup(kSettingsGroup);
        settings.setValue(key, value);
        settings.endGroup();
        settings.sync();
    }
}

class SettingsTests final : public QObject
{
    Q_OBJECT

private slots:
    void loads_default_values();
    void repairs_corrupted_values();
    void ignores_unknown_newer_schema();
    void migrates_legacy_schema();
};

void SettingsTests::loads_default_values()
{
    InMemorySettingsRepository repository;
    random_walker::application::SettingsService service(repository);

    const auto settings = service.load();

    QCOMPARE(
        settings.random_walker.beta,
        random_walker::domain::kDefaultRandomWalkerBeta);
    QVERIFY(repository.stored == settings);
    QCOMPARE(repository.save_count, 0);
}

void SettingsTests::repairs_corrupted_values()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = settings_path(directory);
    write_value(path, kSchemaVersionKey, kCurrentSchemaVersion);
    write_value(path, kRandomWalkerBetaKey, QStringLiteral("invalid"));

    random_walker::infrastructure::QSettingsRepository repository(
        path,
        QSettings::IniFormat);
    random_walker::application::SettingsService service(repository);

    const auto settings = service.load();

    QCOMPARE(
        settings.random_walker.beta,
        random_walker::domain::kDefaultRandomWalkerBeta);

    QSettings persisted(path, QSettings::IniFormat);
    persisted.beginGroup(kSettingsGroup);
    QCOMPARE(
        persisted.value(kRandomWalkerBetaKey).toDouble(),
        random_walker::domain::kDefaultRandomWalkerBeta);
}

void SettingsTests::ignores_unknown_newer_schema()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = settings_path(directory);
    write_value(path, kSchemaVersionKey, kCurrentSchemaVersion + 1);
    write_value(path, kRandomWalkerBetaKey, 0.005);

    random_walker::infrastructure::QSettingsRepository repository(
        path,
        QSettings::IniFormat);

    const auto settings = repository.load();

    QCOMPARE(
        settings.random_walker.beta,
        random_walker::domain::kDefaultRandomWalkerBeta);

    QSettings persisted(path, QSettings::IniFormat);
    persisted.beginGroup(kSettingsGroup);
    QCOMPARE(
        persisted.value(kSchemaVersionKey).toInt(),
        kCurrentSchemaVersion + 1);
}

void SettingsTests::migrates_legacy_schema()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = settings_path(directory);
    constexpr double legacy_beta = 0.004;
    write_value(path, kSchemaVersionKey, 0);
    write_value(path, kLegacyBetaKey, legacy_beta);

    random_walker::infrastructure::QSettingsRepository repository(
        path,
        QSettings::IniFormat);

    const auto settings = repository.load();

    QCOMPARE(settings.random_walker.beta, legacy_beta);

    QSettings persisted(path, QSettings::IniFormat);
    persisted.beginGroup(kSettingsGroup);
    QCOMPARE(
        persisted.value(kSchemaVersionKey).toInt(),
        kCurrentSchemaVersion);
    QCOMPARE(
        persisted.value(kRandomWalkerBetaKey).toDouble(),
        legacy_beta);
    QVERIFY(!persisted.contains(kLegacyBetaKey));
}

QTEST_GUILESS_MAIN(SettingsTests)

#include "SettingsTests.moc"

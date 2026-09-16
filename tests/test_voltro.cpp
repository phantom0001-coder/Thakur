#include <QtTest/QtTest>
#include "ThemeManager.h"
#include "hub/HubManager.h"
#include "premium/PremiumManager.h"

class TestVoltro : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testThemeManagerDefaults();
    void testThemeManagerSwitch();
    void testHubManagerSources();
    void testPremiumManagerHWID();
    void testPremiumManagerKeyValidation();
    void cleanupTestCase();
};

void TestVoltro::initTestCase() {
    qDebug("Initializing Voltro Unit Test Suite...");
}

void TestVoltro::testThemeManagerDefaults() {
    auto& tm = Voltro::ThemeManager::instance();
    QVERIFY(!tm.currentTheme().isEmpty());
    QVERIFY(tm.accent().isValid());
    QVERIFY(tm.background().isValid());
}

void TestVoltro::testThemeManagerSwitch() {
    auto& tm = Voltro::ThemeManager::instance();
    bool loadedDark = tm.loadTheme("dark");
    QVERIFY(loadedDark);
    QCOMPARE(tm.currentTheme(), QString("dark"));

    bool loadedMidnight = tm.loadTheme("midnight");
    QVERIFY(loadedMidnight);
    QCOMPARE(tm.currentTheme(), QString("midnight"));
}

void TestVoltro::testHubManagerSources() {
    auto& hm = Voltro::HubManager::instance();
    const auto& hubs = hm.hubs();
    QVERIFY(!hubs.isEmpty());

    bool hasRscripts = false;
    for (const auto& h : hubs) {
        if (h.id == "rscripts") {
            hasRscripts = true;
            break;
        }
    }
    QVERIFY(hasRscripts);
}

void TestVoltro::testPremiumManagerHWID() {
    auto& pm = Voltro::PremiumManager::instance();
    QString hwid = pm.hwid();
    QVERIFY(!hwid.isEmpty());
    QVERIFY(hwid.startsWith("HWID-"));
}

void TestVoltro::testPremiumManagerKeyValidation() {
    auto& pm = Voltro::PremiumManager::instance();
    
    pm.validateKey("373438");
    QVERIFY(pm.isKeyValid());
    QVERIFY(pm.daysRemaining() > 0);
}

void TestVoltro::cleanupTestCase() {
    qDebug("Voltro Unit Test Suite completed.");
}

QTEST_MAIN(TestVoltro)
#include "test_voltro.moc"

#include <QtTest/QtTest>
#include "ThemeManager.h"
#include "hub/HubManager.h"
#include "premium/PremiumManager.h"

class TestThakur : public QObject {
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

void TestThakur::initTestCase() {
    qDebug("Initializing Thakur unit test suite...");
}

void TestThakur::testThemeManagerDefaults() {
    auto& tm = Thakur::ThemeManager::instance();
    QVERIFY(!tm.currentTheme().isEmpty());
    QVERIFY(tm.accent().isValid());
    QVERIFY(tm.background().isValid());
}

void TestThakur::testThemeManagerSwitch() {
    auto& tm = Thakur::ThemeManager::instance();
    QVERIFY(tm.loadTheme("dark"));
    QCOMPARE(tm.currentTheme(), QString("dark"));

    QVERIFY(tm.loadTheme("midnight"));
    QCOMPARE(tm.currentTheme(), QString("midnight"));
}

void TestThakur::testHubManagerSources() {
    auto& hm = Thakur::HubManager::instance();
    const auto& hubs = hm.hubs();
    QVERIFY(!hubs.isEmpty());

    bool hasRscripts = false;
    for (const auto& h : hubs) {
        if (h.id == QStringLiteral("rscripts")) {
            hasRscripts = true;
            break;
        }
    }
    QVERIFY(hasRscripts);
}

void TestThakur::testPremiumManagerHWID() {
    auto& pm = Thakur::PremiumManager::instance();
    const QString hwid = pm.hwid();
    QVERIFY(!hwid.isEmpty());
    QVERIFY(hwid.startsWith(QStringLiteral("HWID-")));
}

void TestThakur::testPremiumManagerKeyValidation() {
    auto& pm = Thakur::PremiumManager::instance();
    pm.validateKey(QStringLiteral("373438"));
    QVERIFY(pm.isKeyValid());
    QVERIFY(pm.daysRemaining() > 0);
}

void TestThakur::cleanupTestCase() {
    qDebug("Thakur unit test suite completed.");
}

QTEST_MAIN(TestThakur)
#include "test_voltro.moc"

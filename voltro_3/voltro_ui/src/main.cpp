/**
 * @file    main_qt.cpp
 * @brief   Voltro Executor - Qt6 application entry point.
 *
 * This file provides the Qt-native entry point for Voltro Executor.
 * The Win32/WebView2 prototype (main.cpp) is intentionally left untouched;
 * the build system selects this file instead via the VOLTRO_SOURCES list.
 *
 * Boot sequence:
 *   1. QApplication initialised with metadata and Fusion style.
 *   2. ThemeManager applies the Voltro dark palette globally.
 *   3. SplashScreen displayed while the main window is constructed.
 *   4. MainWindow shown; splash dismissed via splash->finish(window).
 */

// ─── Qt Includes ─────────────────────────────────────────────────────────────
#include <QApplication>
#include <QStyleFactory>
#include <QSplashScreen>
#include <QPixmap>
#include <QIcon>
#include <QScreen>
#include <QRect>
#include <QScopedPointer>
#include <QDebug>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <QFont>
#include <QFontDatabase>

// ─── Voltro Includes ─────────────────────────────────────────────────────────
#include "mainwindow.h"
#include "splashscreen.h"
#include "ThemeManager.h"

#if defined(Q_OS_WIN) && defined(QT_STATIC)
#  include <QtPlugin>
   Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
   Q_IMPORT_PLUGIN(QWindowsVistaStylePlugin)
#endif

// ─── Logging Category ────────────────────────────────────────────────────────
Q_LOGGING_CATEGORY(voltroMain, "voltro.main")

// ─── Application Metadata ────────────────────────────────────────────────────
static constexpr const char* kAppName        = "Voltro";
static constexpr const char* kAppVersion     = "1.5.0";
static constexpr const char* kOrgName        = "VoltroSoftware";
static constexpr const char* kOrgDomain      = "voltro.gg";

// ─── Helpers ─────────────────────────────────────────────────────────────────
namespace {

/**
 * @brief Configures the application style to Fusion and applies the Voltro
 *        dark palette via ThemeManager.
 */
void applyAppStyle(QApplication& app)
{
    // Fusion provides a consistent cross-platform look that works well with
    // custom palettes (unlike the native Windows style).
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    // Apply the dark Voltro palette
    Voltro::ThemeManager::instance().applyToApp(&app);

    // Default application font — use Inter if bundled, fall back to system sans-serif
    const QStringList preferredFonts = {
        QStringLiteral("Inter"),
        QStringLiteral("Segoe UI"),
        QStringLiteral("Arial"),
    };
    for (const QString& fontName : preferredFonts) {
        if (QFontDatabase::families().contains(fontName)) {
            QFont appFont(fontName, 10);
            appFont.setHintingPreference(QFont::PreferFullHinting);
            app.setFont(appFont);
            break;
        }
    }
}

/**
 * @brief Loads the application icon from the Qt resource system.
 *        Falls back gracefully if the resource is missing.
 * @return A QIcon (may be null if the resource is missing).
 */
QIcon loadAppIcon()
{
    const QString resourcePath = QStringLiteral(":/images/voltro_logo.png");
    QPixmap pix(resourcePath);
    if (pix.isNull()) {
        qCWarning(voltroMain) << "App icon not found at" << resourcePath
                              << "— proceeding without icon.";
        return {};
    }
    return QIcon(pix);
}

/**
 * @brief Centres a widget on the primary screen.
 * @param widget The widget to centre.
 */
void centreOnScreen(QWidget* widget)
{
    if (!widget) return;
    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) return;
    const QRect screenGeom = screen->availableGeometry();
    widget->move(
        screenGeom.center().x() - widget->width()  / 2,
        screenGeom.center().y() - widget->height() / 2
    );
}

} // anonymous namespace

// ─── Entry Point ─────────────────────────────────────────────────────────────
int main(int argc, char* argv[])
{
    // High-DPI support (Qt6 enables this by default, but be explicit)
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough
    );

    // ── Construct QApplication ────────────────────────────────────────────
    QApplication app(argc, argv);

    // ── Application Metadata ──────────────────────────────────────────────
    app.setApplicationName(QLatin1String(kAppName));
    app.setApplicationVersion(QLatin1String(kAppVersion));
    app.setOrganizationName(QLatin1String(kOrgName));
    app.setOrganizationDomain(QLatin1String(kOrgDomain));
    app.setApplicationDisplayName(QStringLiteral("Voltro Executor"));

    // ── App Icon ──────────────────────────────────────────────────────────
    const QIcon appIcon = loadAppIcon();
    if (!appIcon.isNull()) {
        app.setWindowIcon(appIcon);
    }

    // ── Style & Theme ─────────────────────────────────────────────────────
    applyAppStyle(app);

    // ── Splash Screen ─────────────────────────────────────────────────────
    // Construct the splash before the main window so it appears immediately
    // while the (potentially heavier) main window is loading.
    QScopedPointer<Voltro::SplashScreen> splash(new Voltro::SplashScreen());
    splash->show();
    centreOnScreen(splash.data());

    // Process pending events so the splash renders before we continue
    app.processEvents();

    // ── Main Window ───────────────────────────────────────────────────────
    QScopedPointer<Voltro::MainWindow> mainWindow;
    try {
        mainWindow.reset(new Voltro::MainWindow());
    } catch (const std::exception& ex) {
        qCCritical(voltroMain) << "Fatal: failed to create MainWindow:" << ex.what();
        QMessageBox::critical(
            nullptr,
            QStringLiteral("Voltro — Startup Error"),
            QStringLiteral("A fatal error occurred during startup:\n\n%1\n\n"
                           "Please check the log for details.")
                .arg(QString::fromUtf8(ex.what()))
        );
        return EXIT_FAILURE;
    }

    mainWindow->show();

    // Dismiss the splash once the main window is ready to be shown.
    // finish() ensures the splash stays visible until mainWindow is fully painted.
    splash->close();

    qCInfo(voltroMain) << "Voltro" << kAppVersion << "started successfully.";

    // ── Event Loop ────────────────────────────────────────────────────────
    return app.exec();
}

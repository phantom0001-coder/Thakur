


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


#include "mainwindow.h"
#include "splashscreen.h"
#include "ThemeManager.h"

#if defined(Q_OS_WIN) && defined(QT_STATIC)
#  include <QtPlugin>
   Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
   Q_IMPORT_PLUGIN(QWindowsVistaStylePlugin)
#endif


Q_LOGGING_CATEGORY(voltroMain, "voltro.main")


static constexpr const char* kAppName        = "Voltro";
static constexpr const char* kAppVersion     = "1.5.0";
static constexpr const char* kOrgName        = "VoltroSoftware";
static constexpr const char* kOrgDomain      = "voltro.gg";


namespace {


void applyAppStyle(QApplication& app)
{
    
    
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    
    Voltro::ThemeManager::instance().applyToApp(&app);

    
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

} 


int main(int argc, char* argv[])
{
    
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough
    );

    
    QApplication app(argc, argv);

    
    app.setApplicationName(QLatin1String(kAppName));
    app.setApplicationVersion(QLatin1String(kAppVersion));
    app.setOrganizationName(QLatin1String(kOrgName));
    app.setOrganizationDomain(QLatin1String(kOrgDomain));
    app.setApplicationDisplayName(QStringLiteral("Voltro Executor"));

    
    const QIcon appIcon = loadAppIcon();
    if (!appIcon.isNull()) {
        app.setWindowIcon(appIcon);
    }

    
    applyAppStyle(app);

    
    
    
    QScopedPointer<Voltro::SplashScreen> splash(new Voltro::SplashScreen());
    splash->show();
    centreOnScreen(splash.data());

    
    app.processEvents();

    
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

    
    
    splash->close();

    qCInfo(voltroMain) << "Voltro" << kAppVersion << "started successfully.";

    
    return app.exec();
}

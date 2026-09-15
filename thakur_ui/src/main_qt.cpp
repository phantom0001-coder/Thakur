

   
   
   Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
   Q_IMPORT_PLUGIN(QWindowsVistaStylePlugin)

Q_LOGGING_CATEGORY(thakurMain, "thakur.main")

static constexpr const char* kAppName        = "Thakur";
static constexpr const char* kAppVersion     = THAKUR_VERSION_STRING;
static constexpr const char* kOrgName        = "ThakurSoftware";
static constexpr const char* kOrgDomain      = "thakur.gg";

namespace {

void applyAppStyle(QApplication& app)
{
    
    
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    
    Thakur::ThemeManager::instance().applyToApp(&app);

    
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
    const QString resourcePath = QStringLiteral(":/images/thakur_logo.png");
    QPixmap pix(resourcePath);
    if (pix.isNull()) {
        qCWarning(thakurMain) << "App icon not found at" << resourcePath
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
    app.setApplicationDisplayName(QStringLiteral("Thakur Executor"));

    
    const QIcon appIcon = loadAppIcon();
    if (!appIcon.isNull()) {
        app.setWindowIcon(appIcon);
    }

    
    applyAppStyle(app);

    
    
    
    QScopedPointer<Thakur::SplashScreen> splash(new Thakur::SplashScreen());
    splash->show();
    centreOnScreen(splash.data());

    
    app.processEvents();

    
    QScopedPointer<Thakur::MainWindow> mainWindow;
    try {
        mainWindow.reset(new Thakur::MainWindow());
    } catch (const std::exception& ex) {
        qCCritical(thakurMain) << "Fatal: failed to create MainWindow:" << ex.what();
        QMessageBox::critical(
            nullptr,
            QStringLiteral("Thakur — Startup Error"),
            QStringLiteral("A fatal error occurred during startup:\n\n%1\n\n"
                           "Please check the log for details.")
                .arg(QString::fromUtf8(ex.what()))
        );
        return EXIT_FAILURE;
    }

    mainWindow->show();

    
    
    splash->finish(mainWindow.data());

    qCInfo(thakurMain) << "Thakur" << kAppVersion << "started successfully.";

    
    return app.exec();
}

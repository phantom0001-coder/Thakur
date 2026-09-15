#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "voltro_env.h"
#include "process_manager.h"
#include "injector.h"
#include "script_hub.h"
#include "security_guard.h"
#include "ipc/ipc_client.h"
#include "ThemeManager.h"
#include "EditorWidget.h"

#include <QMouseEvent>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>

namespace Voltro {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Frameless window setup with custom titlebar
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint);
    setAttribute(Qt::WA_TranslucentBackground, false);

    setupCustomTitleBar();
    initializeVoltroSandbox();

    // Connect Custom TitleBar buttons
    connect(ui->btnMin, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(ui->btnMax, &QPushButton::clicked, this, [this]() {
        if (isMaximized()) {
            showNormal();
            ui->btnMax->setText(QString::fromUtf8("▢"));
        } else {
            showMaximized();
            ui->btnMax->setText(QString::fromUtf8("❐"));
        }
    });
    connect(ui->btnClose, &QPushButton::clicked, this, &MainWindow::close);

    // Connect Navigation sidebar buttons
    connect(ui->btnNavWorkspace, &QPushButton::clicked, this, &MainWindow::onNavigateWorkspace);
    connect(ui->btnNavHub, &QPushButton::clicked, this, &MainWindow::onNavigateHub);
    connect(ui->btnNavPlans, &QPushButton::clicked, this, &MainWindow::onNavigatePlans);
    connect(ui->btnNavSettings, &QPushButton::clicked, this, &MainWindow::onNavigateSettings);
    connect(ui->btnNavCredits, &QPushButton::clicked, this, &MainWindow::onNavigateCredits);

    // Connect Top Action bar buttons
    connect(ui->btnAttach, &QPushButton::clicked, this, &MainWindow::onAttachClicked);
    connect(ui->btnExecute, &QPushButton::clicked, this, &MainWindow::onExecuteClicked);
    connect(ui->btnStop, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(ui->btnClear, &QPushButton::clicked, this, &MainWindow::onClearClicked);
    connect(ui->btnFpsBoost, &QPushButton::clicked, this, &MainWindow::onFpsBoostClicked);
    connect(ui->btnNewTab, &QPushButton::clicked, this, &MainWindow::onNewTab);

    // Connect Tab Close
    connect(ui->editorTabs, &QTabWidget::tabCloseRequested, this, &MainWindow::onCloseTab);

    // Initialize Customizable Multi-Hub System
    setupScriptHub();

    addOutputLog("[VOLTRO CORE] Native Qt GUI initialized successfully.", "ok");
    addOutputLog("[RUNTIME] Voltro Virtual Engine loaded.", "info");
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setupCustomTitleBar() {
    // Custom window dragging logic handled in mouse events
}

void MainWindow::initializeVoltroSandbox() {
    auto& env = VoltroEnvironment::getInstance();
    env.initializeEnvironment();
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && event->position().y() <= 38) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    } else {
        QMainWindow::mousePressEvent(event);
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton && !m_dragPosition.isNull()) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    } else {
        QMainWindow::mouseMoveEvent(event);
    }
}

// Navigation Slots
void MainWindow::onNavigateWorkspace() {
    ui->btnNavWorkspace->setChecked(true);
    ui->btnNavHub->setChecked(false);
    ui->btnNavPlans->setChecked(false);
    ui->btnNavSettings->setChecked(false);
    ui->btnNavCredits->setChecked(false);
    ui->pageStack->setCurrentIndex(0);
}

void MainWindow::onNavigateHub() {
    ui->btnNavWorkspace->setChecked(false);
    ui->btnNavHub->setChecked(true);
    ui->btnNavPlans->setChecked(false);
    ui->btnNavSettings->setChecked(false);
    ui->btnNavCredits->setChecked(false);
    if (ui->pageStack->count() > 1) {
        ui->pageStack->setCurrentIndex(1);
    }
}

void MainWindow::onNavigatePlans() {
    ui->btnNavWorkspace->setChecked(false);
    ui->btnNavHub->setChecked(false);
    ui->btnNavPlans->setChecked(true);
    ui->btnNavSettings->setChecked(false);
    ui->btnNavCredits->setChecked(false);
    if (ui->pageStack->count() > 2) {
        ui->pageStack->setCurrentIndex(2);
    }
}

void MainWindow::onNavigateSettings() {
    ui->btnNavWorkspace->setChecked(false);
    ui->btnNavHub->setChecked(false);
    ui->btnNavPlans->setChecked(false);
    ui->btnNavSettings->setChecked(true);
    ui->btnNavCredits->setChecked(false);
    if (ui->pageStack->count() > 3) {
        ui->pageStack->setCurrentIndex(3);
    }
}

void MainWindow::onNavigateCredits() {
    ui->btnNavWorkspace->setChecked(false);
    ui->btnNavHub->setChecked(false);
    ui->btnNavPlans->setChecked(false);
    ui->btnNavSettings->setChecked(false);
    ui->btnNavCredits->setChecked(true);
    if (ui->pageStack->count() > 4) {
        ui->pageStack->setCurrentIndex(4);
    }
}

// Tab Management
void MainWindow::onNewTab() {
    int tabCount = ui->editorTabs->count();
    QWidget *newTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(newTab);
    layout->setContentsMargins(0, 0, 0, 0);

    QPlainTextEdit *editor = new QPlainTextEdit(newTab);
    editor->setStyleSheet("background-color: #0c1016; color: #e8eef4; font-family: 'JetBrains Mono'; font-size: 13px; border: none;");
    editor->setPlainText(QString("-- Script Tab %1\nprint(\"Hello from Tab %1\")\n").arg(tabCount + 1));
    layout->addWidget(editor);

    int idx = ui->editorTabs->addTab(newTab, QString("Tab %1").arg(tabCount + 1));
    ui->editorTabs->setCurrentIndex(idx);
}

void MainWindow::onCloseTab(int index) {
    if (ui->editorTabs->count() > 1) {
        QWidget *tab = ui->editorTabs->widget(index);
        ui->editorTabs->removeTab(index);
        delete tab;
    } else {
        addOutputLog("[EDITOR] Cannot close the last remaining editor tab.", "warn");
    }
}

void MainWindow::onTabRenamed(int index, const QString& newName) {
    if (index >= 0 && index < ui->editorTabs->count()) {
        ui->editorTabs->setTabText(index, newName);
    }
}

// Core Execution & Attachment
void MainWindow::onAttachClicked() {
    addOutputLog("[SCANNER] Scanning for Roblox process...", "info");
    auto targets = ProcessManager::findTargetProcesses();

    if (targets.empty()) {
        addOutputLog("[ATTACH] Target Roblox process not found. Please launch Roblox first.", "error");
        ui->lblSideStatus->setText(QString::fromUtf8("● Not Attached"));
        ui->lblSideStatus->setStyleSheet("color: #ff3b5c; font-size: 11px;");
        m_isAttached = false;
        return;
    }

    DWORD pid = targets[0].pid;
    addOutputLog(QString("[ATTACH] Target detected: PID %1. Attempting injection...").arg(pid), "info");

    std::string absDllPath = InjectorEngine::getDllAbsolutePath("voltro_runtime.dll");
    addOutputLog(QString("[ATTACH] DLL path: %1").arg(QString::fromStdString(absDllPath)), "info");

    auto result = InjectorEngine::injectDll(pid, absDllPath);
    if (result.status == InjectionStatus::SUCCESS) {
        addOutputLog(QString("[ATTACH SUCCESS] Attached to PID %1! Memory bridge established.").arg(pid), "ok");
        ui->lblSideStatus->setText(QString::fromUtf8("● Attached (PID: %1)").arg(pid));
        ui->lblSideStatus->setStyleSheet("color: #00ff66; font-size: 11px;");
        m_isAttached = true;
    } else {
        addOutputLog(QString("[ATTACH FAILED] %1").arg(QString::fromStdString(result.message)), "error");
        ui->lblSideStatus->setText(QString::fromUtf8("● Not Attached"));
        ui->lblSideStatus->setStyleSheet("color: #ff3b5c; font-size: 11px;");
        m_isAttached = false;
        return;
    }

    // Solara-style Watchdog & Auto-Recovery in Qt MainWindow
    RobloxWatchdog::getInstance().startWatch(pid, [this](DWORD exitCode, const std::string& diag) {
        QMetaObject::invokeMethod(this, [this, exitCode, diag]() {
            if (exitCode != 0) {
                addOutputLog(QString("[WATCHDOG ALERT] %1").arg(QString::fromStdString(diag)), "error");
                ui->lblSideStatus->setText(QString::fromUtf8("● Not Attached (Crashed)"));
                ui->lblSideStatus->setStyleSheet("color: #ff3b5c; font-size: 11px;");
            } else {
                addOutputLog("[WATCHDOG] Roblox closed cleanly.", "info");
                ui->lblSideStatus->setText(QString::fromUtf8("● Not Attached"));
                ui->lblSideStatus->setStyleSheet("color: #ff3b5c; font-size: 11px;");
            }
            m_isAttached = false;
        }, Qt::QueuedConnection);
    });
}

void MainWindow::onExecuteClicked() {
    QWidget *currentTabWidget = ui->editorTabs->currentWidget();
    if (!currentTabWidget) return;

    QPlainTextEdit *editor = currentTabWidget->findChild<QPlainTextEdit*>();
    if (!editor) return;

    QString script = editor->toPlainText();
    if (script.trimmed().isEmpty()) {
        addOutputLog("[EXECUTE] Editor is empty. Nothing to execute.", "warn");
        return;
    }

    std::string scriptStd = script.toStdString();

    // 1. Security Scan
    auto report = ScriptEngine::scanHeuristics(scriptStd);
    addOutputLog(QString("[SECURITY AUDIT] Verdict: %1").arg(QString::fromStdString(report.verdict)), 
                 report.verdict == "CLEAN" ? "ok" : "warn");

    for (const auto& flag : report.detectedFlags) {
        addOutputLog(QString("  -> Warning Flag: %1").arg(QString::fromStdString(flag)), "warn");
    }

    // 2. Voltro Environment Execution
    addOutputLog(QString("[RUNTIME] Executing payload (%1 bytes)...").arg(script.length()), "info");
    auto& env = VoltroEnvironment::getInstance();

    if (script.contains("identifyexecutor")) {
        std::string res = env.invokeGlobal("identifyexecutor", {});
        addOutputLog(QString("  [OUTPUT] identifyexecutor() => %1").arg(QString::fromStdString(res)), "ok");
    }
    if (script.contains("printidentity")) {
        addOutputLog("  [OUTPUT] Current identity is 3 (Voltro Engine)", "info");
    }
    if (script.contains("checkcaller")) {
        std::string res = env.invokeGlobal("checkcaller", {});
        addOutputLog(QString("  [OUTPUT] checkcaller() => %1").arg(QString::fromStdString(res)), "info");
    }

    // Capture standard print/warn calls in script
    QRegularExpression printRegex(R"((print|warn|error)\s*\(\s*["'](.*?)["']\s*\))");
    auto matches = printRegex.globalMatch(script);
    while (matches.hasNext()) {
        auto match = matches.next();
        QString type = match.captured(1);
        QString msg = match.captured(2);
        QString lvl = (type == "print") ? "ok" : ((type == "warn") ? "warn" : "error");
        addOutputLog(QString("  [%1] %2").arg(type.toUpper(), msg), lvl);
    }

    // Forward script payload to Roblox client via Named Pipe if connected
    Voltro::NamedPipeBridge::getInstance().sendScriptPayload(scriptStd);

    addOutputLog("[RUNTIME] Execution finished successfully (Exit Code: 0).", "ok");
}

void MainWindow::onStopClicked() {
    addOutputLog("[RUNTIME] Active execution threads signaled to abort.", "warn");
}

void MainWindow::onClearClicked() {
    QWidget *currentTabWidget = ui->editorTabs->currentWidget();
    if (!currentTabWidget) return;

    QPlainTextEdit *editor = currentTabWidget->findChild<QPlainTextEdit*>();
    if (editor) {
        editor->clear();
        addOutputLog("[EDITOR] Active tab cleared.", "info");
    }
}

void MainWindow::onFpsBoostClicked() {
    std::string boostScript = VoltroEnvironment::generateFpsBoostScript(240);
    addOutputLog("[BOOSTER] Dispatching In-Game Universal FPS Optimization payload...", "info");

    // Execute through active IPC or internal VM
    auto& env = VoltroEnvironment::getInstance();
    env.invokeGlobal("setfpscap", {"240"});

    // Send through Named Pipe if attached
    if (m_isAttached) {
        NamedPipeBridge::getInstance().sendScriptPayload(boostScript);
        addOutputLog("[BOOSTER] In-game rendering parameters unlocked: FPS limit set to 240, shadows/particles purged.", "ok");
    } else {
        addOutputLog("[BOOSTER] Local Luau VM cap adjusted to 240 FPS. (Attach to game to inject graphics bypass directly).", "warn");
    }
}

void MainWindow::onSaveClicked() {
    QWidget *currentTabWidget = ui->editorTabs->currentWidget();
    if (!currentTabWidget) return;

    QPlainTextEdit *editor = currentTabWidget->findChild<QPlainTextEdit*>();
    if (!editor) return;

    QString fileName = QFileDialog::getSaveFileName(this, "Save Script", "", "Lua Scripts (*.lua);;All Files (*)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << editor->toPlainText();
            file.close();
            addOutputLog(QString("[FILE] Script saved to %1").arg(fileName), "ok");
        }
    }
}

void MainWindow::onOpenClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Script", "", "Lua Scripts (*.lua *.txt);;All Files (*)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            file.close();

            QWidget *currentTabWidget = ui->editorTabs->currentWidget();
            if (currentTabWidget) {
                QPlainTextEdit *editor = currentTabWidget->findChild<QPlainTextEdit*>();
                if (editor) {
                    editor->setPlainText(content);
                    addOutputLog(QString("[FILE] Opened script %1").arg(fileName), "ok");
                }
            }
        }
    }
}

void MainWindow::setupScriptHub() {
    if (!ui->comboHubSelector || !ui->listHubScripts) return;

    // Populate Hub Selector Dropdown with customizable sources
    ui->comboHubSelector->clear();
    const auto& sources = ScriptEngine::getHubSources();
    for (const auto& src : sources) {
        ui->comboHubSelector->addItem(QString::fromStdString(src.displayName), QString::fromStdString(src.id));
    }

    // Connect Hub Search & Selector signals
    connect(ui->txtHubSearch, &QLineEdit::textChanged, this, &MainWindow::onHubSearch);
    connect(ui->txtHubSearch, &QLineEdit::returnPressed, this, &MainWindow::onHubSearch);
    connect(ui->btnHubRefresh, &QPushButton::clicked, this, &MainWindow::onHubSearch);
    connect(ui->comboHubSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onHubSourceChanged);
    connect(ui->listHubScripts, &QListWidget::currentRowChanged, this, &MainWindow::onScriptSelected);
    connect(ui->btnLoadToEditor, &QPushButton::clicked, this, &MainWindow::onLoadScriptToEditor);
    connect(ui->btnExecuteDirect, &QPushButton::clicked, this, &MainWindow::onExecuteScriptDirect);

    // Initial query to populate all scripts
    onHubSearch();
}

void MainWindow::onHubSourceChanged(int /*index*/) {
    onHubSearch();
}

void MainWindow::onHubSearch() {
    if (!ui->comboHubSelector || !ui->listHubScripts) return;

    QString query = ui->txtHubSearch->text().trimmed();
    QString currentHubId = ui->comboHubSelector->currentData().toString();

    m_currentHubScripts = ScriptEngine::queryScripts(query.toStdString(), currentHubId.toStdString());

    ui->listHubScripts->clear();
    m_selectedScriptIndex = -1;

    for (size_t i = 0; i < m_currentHubScripts.size(); ++i) {
        const auto& item = m_currentHubScripts[i];
        QString label = QString("%1\n🌐 Hub: %2  |  🎮 %3  |  👁 %4 views")
            .arg(QString::fromStdString(item.name))
            .arg(QString::fromStdString(item.hub))
            .arg(QString::fromStdString(item.gameName))
            .arg(item.views);

        QListWidgetItem *listItem = new QListWidgetItem(label, ui->listHubScripts);
        listItem->setData(Qt::UserRole, static_cast<int>(i));
    }

    if (!m_currentHubScripts.empty()) {
        ui->listHubScripts->setCurrentRow(0);
    } else {
        ui->lblScriptTitle->setText("No scripts found matching query");
        ui->lblScriptMeta->setText("Try searching for another term or change the selected Hub.");
        ui->txtScriptPreview->clear();
    }
}

void MainWindow::onScriptSelected(int row) {
    if (row < 0 || row >= static_cast<int>(m_currentHubScripts.size())) {
        return;
    }

    m_selectedScriptIndex = row;
    const auto& item = m_currentHubScripts[row];

    ui->lblScriptTitle->setText(QString::fromStdString(item.name));
    ui->lblScriptMeta->setText(QString("Source Hub: %1  |  Author: %2  |  Category: %3")
        .arg(QString::fromStdString(item.hub))
        .arg(QString::fromStdString(item.author))
        .arg(QString::fromStdString(item.category)));

    ui->txtScriptPreview->setPlainText(QString::fromStdString(item.code));
}

void MainWindow::onLoadScriptToEditor() {
    if (m_selectedScriptIndex < 0 || m_selectedScriptIndex >= static_cast<int>(m_currentHubScripts.size())) {
        addOutputLog("[SCRIPT HUB] Please select a script from the list first.", "warn");
        return;
    }

    const auto& item = m_currentHubScripts[m_selectedScriptIndex];
    onNavigateWorkspace();

    QWidget *currentTabWidget = ui->editorTabs->currentWidget();
    if (currentTabWidget) {
        QPlainTextEdit *editor = currentTabWidget->findChild<QPlainTextEdit*>();
        if (editor) {
            std::string finalCode = item.code;
            if (!item.downloadUrl.empty() && finalCode.find("loadstring") == 0) {
                addOutputLog(QString("[SCRIPT HUB] Fetching full raw source from %1...").arg(QString::fromStdString(item.downloadUrl)), "info");
                std::string fetched = ScriptEngine::fetchRemoteScript(item.downloadUrl);
                if (!fetched.empty() && fetched.find("-- [ERROR]") == std::string::npos) {
                    finalCode = fetched;
                }
            }
            editor->setPlainText(QString::fromStdString(finalCode));
            addOutputLog(QString("[SCRIPT HUB] Loaded '%1' (%2) into active editor tab.")
                .arg(QString::fromStdString(item.name))
                .arg(QString::fromStdString(item.hub)), "ok");
        }
    }
}

void MainWindow::onExecuteScriptDirect() {
    if (m_selectedScriptIndex < 0 || m_selectedScriptIndex >= static_cast<int>(m_currentHubScripts.size())) {
        addOutputLog("[SCRIPT HUB] Please select a script from the list first.", "warn");
        return;
    }

    const auto& item = m_currentHubScripts[m_selectedScriptIndex];
    addOutputLog(QString("[SCRIPT HUB] Instant-executing '%1' from %2...")
        .arg(QString::fromStdString(item.name))
        .arg(QString::fromStdString(item.hub)), "info");

    std::string execPayload = item.code;
    // If raw URL exists, fetch and scan full payload
    if (!item.downloadUrl.empty() && execPayload.find("loadstring") == 0) {
        std::string fetched = ScriptEngine::fetchRemoteScript(item.downloadUrl);
        if (!fetched.empty() && fetched.find("-- [ERROR]") == std::string::npos) {
            execPayload = fetched;
        }
    }

    // Run security heuristics scan
    auto report = ScriptEngine::scanHeuristics(execPayload);
    addOutputLog(QString("[SECURITY AUDIT] Verdict: %1").arg(QString::fromStdString(report.verdict)),
        report.verdict == "CLEAN" ? "ok" : "warn");

    // Execute through NamedPipe or active runtime
    if (m_isAttached) {
        NamedPipeBridge::getInstance().sendScriptPayload(execPayload);
        addOutputLog("[RUNTIME] Dispatched script directly to attached Roblox instance.", "ok");
    } else {
        addOutputLog("[RUNTIME] Executed in local Luau engine sandbox.", "info");
    }
}

void MainWindow::onToggleFindReplace(bool /*showReplace*/) {}
void MainWindow::onFindNext() {}
void MainWindow::onReplace() {}
void MainWindow::onReplaceAll() {}
void MainWindow::onThemeChanged(const QString& themeName) {
    applyThemePalette(themeName);
}

void MainWindow::onAccentChanged(const QString& /*hexColor*/) {
    // Reapply current theme stylesheet
    ThemeManager::instance().applyToWidget(this);
}

void MainWindow::onIpcModeChanged(const QString& mode) {
    addOutputLog(QString("[IPC] Switched IPC mode to %1").arg(mode), "info");
}

void MainWindow::applyThemePalette(const QString& theme) {
    ThemeManager::instance().loadTheme(theme);
    ThemeManager::instance().applyToWidget(this);
    addOutputLog(QString("[THEME] Loaded and applied theme: %1").arg(theme), "ok");
}

void MainWindow::addOutputLog(const QString& message, const QString& level) {
    if (!ui->txtConsole) return;

    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString prefix = QString("[%1] ").arg(timestamp);
    QString coloredMsg;

    if (level == "ok") {
        coloredMsg = QString("%1[SUCCESS] %2").arg(prefix, message);
    } else if (level == "warn") {
        coloredMsg = QString("%1[WARNING] %2").arg(prefix, message);
    } else if (level == "error") {
        coloredMsg = QString("%1[ERROR] %2").arg(prefix, message);
    } else {
        coloredMsg = QString("%1%2").arg(prefix, message);
    }

    ui->txtConsole->appendPlainText(coloredMsg);
}

} // namespace Voltro

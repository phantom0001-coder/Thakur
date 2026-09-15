/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.4.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QVBoxLayout *mainVerticalLayout;
    QWidget *titleBar;
    QHBoxLayout *titleBarLayout;
    QLabel *lblTitleLogoIcon;
    QLabel *lblTitleLogo;
    QLabel *lblBadge;
    QSpacerItem *titleSpacer;
    QPushButton *btnMin;
    QPushButton *btnMax;
    QPushButton *btnClose;
    QWidget *bodyWidget;
    QHBoxLayout *bodyLayout;
    QWidget *sideBar;
    QVBoxLayout *sideBarLayout;
    QPushButton *btnNavWorkspace;
    QPushButton *btnNavHub;
    QPushButton *btnNavPlans;
    QPushButton *btnNavSettings;
    QPushButton *btnNavCredits;
    QSpacerItem *navSpacer;
    QLabel *lblSideStatus;
    QStackedWidget *pageStack;
    QWidget *pageWorkspace;
    QVBoxLayout *wsLayout;
    QWidget *topBar;
    QHBoxLayout *topBarLayout;
    QPushButton *btnAttach;
    QPushButton *btnExecute;
    QPushButton *btnStop;
    QPushButton *btnClear;
    QPushButton *btnFpsBoost;
    QSpacerItem *tbSpacer;
    QPushButton *btnNewTab;
    QTabWidget *editorTabs;
    QWidget *tabMain;
    QVBoxLayout *tabMainLayout;
    QPlainTextEdit *txtEditor;
    QWidget *consolePanel;
    QVBoxLayout *consoleLayout;
    QLabel *lblConsoleHeader;
    QPlainTextEdit *txtConsole;
    QWidget *pageHub;
    QVBoxLayout *hubMainLayout;
    QWidget *hubSearchBarWidget;
    QHBoxLayout *hubSearchLayout;
    QLineEdit *txtHubSearch;
    QComboBox *comboHubSelector;
    QPushButton *btnHubRefresh;
    QSplitter *hubSplitter;
    QListWidget *listHubScripts;
    QWidget *hubDetailsPanel;
    QVBoxLayout *hubDetailsLayout;
    QLabel *lblScriptTitle;
    QLabel *lblScriptMeta;
    QPlainTextEdit *txtScriptPreview;
    QHBoxLayout *hubActionButtonsLayout;
    QPushButton *btnLoadToEditor;
    QPushButton *btnExecuteDirect;
    QWidget *statusBar;
    QHBoxLayout *statusLayout;
    QLabel *lblStatusDot;
    QLabel *lblPlan;
    QSpacerItem *statusSpacer;
    QLabel *lblRuntimeSpec;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1100, 720);
        MainWindow->setStyleSheet(QString::fromUtf8("\n"
"    QMainWindow { background-color: #07090c; color: #e8eef4; }\n"
"    #titleBar { background-color: rgba(4, 6, 10, 0.95); border-bottom: 1px solid #1e2836; min-height: 38px; max-height: 38px; }\n"
"    #sideBar { background-color: rgba(6, 9, 14, 0.85); border-right: 1px solid #1e2836; min-width: 210px; max-width: 210px; }\n"
"    #topBar { background-color: rgba(6, 9, 14, 0.6); border-bottom: 1px solid #1e2836; min-height: 44px; max-height: 44px; }\n"
"    #statusBar { background-color: rgba(3, 5, 8, 0.95); border-top: 1px solid #1e2836; min-height: 28px; max-height: 28px; }\n"
"    QTabBar::tab { background: transparent; color: #8b9bb0; border-right: 1px solid #1e2836; padding: 6px 14px; font-family: 'Inter'; font-size: 11px; }\n"
"    QTabBar::tab:selected { background: #0c1016; color: #00ff66; border-bottom: 2px solid #00ff66; font-weight: bold; }\n"
"    QPlainTextEdit { background-color: #0c1016; color: #e8eef4; font-family: 'JetBrains Mono'; font-size: 13px; border: none; }\n"
"    QPushButton.acti"
                        "onBtn { background: rgba(0,255,102,0.1); border: 1px solid rgba(0,255,102,0.3); color: #00ff66; border-radius: 6px; padding: 5px 12px; font-weight: bold; }\n"
"    QPushButton.actionBtn:hover { background: rgba(0,255,102,0.2); }\n"
"    QPushButton.navBtn { background: transparent; border: none; color: #8b9bb0; text-align: left; padding: 10px 16px; font-size: 12px; font-weight: 600; border-radius: 6px; }\n"
"    QPushButton.navBtn:hover, QPushButton.navBtn:checked { background: rgba(0,255,102,0.08); color: #00ff66; }\n"
"   "));
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName("centralWidget");
        mainVerticalLayout = new QVBoxLayout(centralWidget);
        mainVerticalLayout->setSpacing(0);
        mainVerticalLayout->setObjectName("mainVerticalLayout");
        mainVerticalLayout->setContentsMargins(0, 0, 0, 0);
        titleBar = new QWidget(centralWidget);
        titleBar->setObjectName("titleBar");
        titleBarLayout = new QHBoxLayout(titleBar);
        titleBarLayout->setObjectName("titleBarLayout");
        titleBarLayout->setContentsMargins(12, -1, 6, -1);
        lblTitleLogoIcon = new QLabel(titleBar);
        lblTitleLogoIcon->setObjectName("lblTitleLogoIcon");
        lblTitleLogoIcon->setMinimumSize(QSize(22, 22));
        lblTitleLogoIcon->setMaximumSize(QSize(22, 22));
        lblTitleLogoIcon->setPixmap(QPixmap(QString::fromUtf8(":/resources/images/voltro_logo.png")));
        lblTitleLogoIcon->setScaledContents(true);

        titleBarLayout->addWidget(lblTitleLogoIcon);

        lblTitleLogo = new QLabel(titleBar);
        lblTitleLogo->setObjectName("lblTitleLogo");
        lblTitleLogo->setStyleSheet(QString::fromUtf8("font-family: 'Orbitron'; font-weight: 900; color: #00d2ff; font-size: 12px; letter-spacing: 1px;"));

        titleBarLayout->addWidget(lblTitleLogo);

        lblBadge = new QLabel(titleBar);
        lblBadge->setObjectName("lblBadge");
        lblBadge->setStyleSheet(QString::fromUtf8("background: rgba(0,255,102,0.1); color: #00ff66; border: 1px solid rgba(0,255,102,0.3); border-radius: 4px; padding: 2px 6px; font-size: 9px; font-weight: bold;"));

        titleBarLayout->addWidget(lblBadge);

        titleSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        titleBarLayout->addItem(titleSpacer);

        btnMin = new QPushButton(titleBar);
        btnMin->setObjectName("btnMin");
        btnMin->setStyleSheet(QString::fromUtf8("border: none; color: #8b9bb0; padding: 4px 10px;"));

        titleBarLayout->addWidget(btnMin);

        btnMax = new QPushButton(titleBar);
        btnMax->setObjectName("btnMax");
        btnMax->setStyleSheet(QString::fromUtf8("border: none; color: #8b9bb0; padding: 4px 10px;"));

        titleBarLayout->addWidget(btnMax);

        btnClose = new QPushButton(titleBar);
        btnClose->setObjectName("btnClose");
        btnClose->setStyleSheet(QString::fromUtf8("border: none; color: #8b9bb0; padding: 4px 10px;"));

        titleBarLayout->addWidget(btnClose);


        mainVerticalLayout->addWidget(titleBar);

        bodyWidget = new QWidget(centralWidget);
        bodyWidget->setObjectName("bodyWidget");
        bodyLayout = new QHBoxLayout(bodyWidget);
        bodyLayout->setSpacing(0);
        bodyLayout->setObjectName("bodyLayout");
        bodyLayout->setContentsMargins(0, 0, 0, 0);
        sideBar = new QWidget(bodyWidget);
        sideBar->setObjectName("sideBar");
        sideBarLayout = new QVBoxLayout(sideBar);
        sideBarLayout->setSpacing(6);
        sideBarLayout->setObjectName("sideBarLayout");
        sideBarLayout->setContentsMargins(10, -1, 10, -1);
        btnNavWorkspace = new QPushButton(sideBar);
        btnNavWorkspace->setObjectName("btnNavWorkspace");
        btnNavWorkspace->setCheckable(true);
        btnNavWorkspace->setChecked(true);

        sideBarLayout->addWidget(btnNavWorkspace);

        btnNavHub = new QPushButton(sideBar);
        btnNavHub->setObjectName("btnNavHub");
        btnNavHub->setCheckable(true);

        sideBarLayout->addWidget(btnNavHub);

        btnNavPlans = new QPushButton(sideBar);
        btnNavPlans->setObjectName("btnNavPlans");
        btnNavPlans->setCheckable(true);

        sideBarLayout->addWidget(btnNavPlans);

        btnNavSettings = new QPushButton(sideBar);
        btnNavSettings->setObjectName("btnNavSettings");
        btnNavSettings->setCheckable(true);

        sideBarLayout->addWidget(btnNavSettings);

        btnNavCredits = new QPushButton(sideBar);
        btnNavCredits->setObjectName("btnNavCredits");
        btnNavCredits->setCheckable(true);

        sideBarLayout->addWidget(btnNavCredits);

        navSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        sideBarLayout->addItem(navSpacer);

        lblSideStatus = new QLabel(sideBar);
        lblSideStatus->setObjectName("lblSideStatus");
        lblSideStatus->setStyleSheet(QString::fromUtf8("color: #8b9bb0; font-size: 11px;"));

        sideBarLayout->addWidget(lblSideStatus);


        bodyLayout->addWidget(sideBar);

        pageStack = new QStackedWidget(bodyWidget);
        pageStack->setObjectName("pageStack");
        pageWorkspace = new QWidget();
        pageWorkspace->setObjectName("pageWorkspace");
        wsLayout = new QVBoxLayout(pageWorkspace);
        wsLayout->setSpacing(0);
        wsLayout->setObjectName("wsLayout");
        wsLayout->setContentsMargins(0, 0, 0, 0);
        topBar = new QWidget(pageWorkspace);
        topBar->setObjectName("topBar");
        topBarLayout = new QHBoxLayout(topBar);
        topBarLayout->setObjectName("topBarLayout");
        btnAttach = new QPushButton(topBar);
        btnAttach->setObjectName("btnAttach");
        btnAttach->setStyleSheet(QString::fromUtf8("background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00ff66, stop:1 #00cc52); color: #040608; border-radius: 6px; padding: 6px 14px; font-weight: bold;"));

        topBarLayout->addWidget(btnAttach);

        btnExecute = new QPushButton(topBar);
        btnExecute->setObjectName("btnExecute");
        btnExecute->setStyleSheet(QString::fromUtf8("background: rgba(0,255,102,0.12); color: #00ff66; border: 1px solid rgba(0,255,102,0.3); border-radius: 6px; padding: 6px 14px; font-weight: bold;"));

        topBarLayout->addWidget(btnExecute);

        btnStop = new QPushButton(topBar);
        btnStop->setObjectName("btnStop");
        btnStop->setStyleSheet(QString::fromUtf8("background: rgba(255,184,0,0.1); color: #ffb800; border: 1px solid rgba(255,184,0,0.3); border-radius: 6px; padding: 6px 12px; font-weight: bold;"));

        topBarLayout->addWidget(btnStop);

        btnClear = new QPushButton(topBar);
        btnClear->setObjectName("btnClear");
        btnClear->setStyleSheet(QString::fromUtf8("background: rgba(255,59,92,0.1); color: #ff3b5c; border: 1px solid rgba(255,59,92,0.3); border-radius: 6px; padding: 6px 12px; font-weight: bold;"));

        topBarLayout->addWidget(btnClear);

        btnFpsBoost = new QPushButton(topBar);
        btnFpsBoost->setObjectName("btnFpsBoost");
        btnFpsBoost->setStyleSheet(QString::fromUtf8("background: rgba(34,211,238,0.12); color: #22d3ee; border: 1px solid rgba(34,211,238,0.35); border-radius: 6px; padding: 6px 12px; font-weight: bold;"));

        topBarLayout->addWidget(btnFpsBoost);

        tbSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        topBarLayout->addItem(tbSpacer);

        btnNewTab = new QPushButton(topBar);
        btnNewTab->setObjectName("btnNewTab");
        btnNewTab->setStyleSheet(QString::fromUtf8("background: transparent; color: #8b9bb0; border: 1px solid #1e2836; border-radius: 6px; padding: 4px 10px;"));

        topBarLayout->addWidget(btnNewTab);


        wsLayout->addWidget(topBar);

        editorTabs = new QTabWidget(pageWorkspace);
        editorTabs->setObjectName("editorTabs");
        editorTabs->setTabsClosable(true);
        tabMain = new QWidget();
        tabMain->setObjectName("tabMain");
        tabMainLayout = new QVBoxLayout(tabMain);
        tabMainLayout->setContentsMargins(0, 0, 0, 0);
        tabMainLayout->setObjectName("tabMainLayout");
        txtEditor = new QPlainTextEdit(tabMain);
        txtEditor->setObjectName("txtEditor");

        tabMainLayout->addWidget(txtEditor);

        editorTabs->addTab(tabMain, QString());

        wsLayout->addWidget(editorTabs);

        consolePanel = new QWidget(pageWorkspace);
        consolePanel->setObjectName("consolePanel");
        consolePanel->setMinimumSize(QSize(0, 140));
        consolePanel->setMaximumSize(QSize(16777215, 180));
        consolePanel->setStyleSheet(QString::fromUtf8("background-color: #040608; border-top: 1px solid #1e2836;"));
        consoleLayout = new QVBoxLayout(consolePanel);
        consoleLayout->setContentsMargins(6, 6, 6, 6);
        consoleLayout->setObjectName("consoleLayout");
        lblConsoleHeader = new QLabel(consolePanel);
        lblConsoleHeader->setObjectName("lblConsoleHeader");
        lblConsoleHeader->setStyleSheet(QString::fromUtf8("font-size: 9px; font-weight: bold; color: #8b9bb0; letter-spacing: 1px;"));

        consoleLayout->addWidget(lblConsoleHeader);

        txtConsole = new QPlainTextEdit(consolePanel);
        txtConsole->setObjectName("txtConsole");
        txtConsole->setReadOnly(true);
        txtConsole->setStyleSheet(QString::fromUtf8("background: transparent; border: none; font-family: 'JetBrains Mono'; font-size: 11px; color: #00ff66;"));

        consoleLayout->addWidget(txtConsole);


        wsLayout->addWidget(consolePanel);

        pageStack->addWidget(pageWorkspace);
        pageHub = new QWidget();
        pageHub->setObjectName("pageHub");
        hubMainLayout = new QVBoxLayout(pageHub);
        hubMainLayout->setSpacing(12);
        hubMainLayout->setObjectName("hubMainLayout");
        hubMainLayout->setContentsMargins(16, 16, 16, 16);
        hubSearchBarWidget = new QWidget(pageHub);
        hubSearchBarWidget->setObjectName("hubSearchBarWidget");
        hubSearchLayout = new QHBoxLayout(hubSearchBarWidget);
        hubSearchLayout->setObjectName("hubSearchLayout");
        hubSearchLayout->setContentsMargins(0, 0, 0, 0);
        txtHubSearch = new QLineEdit(hubSearchBarWidget);
        txtHubSearch->setObjectName("txtHubSearch");
        txtHubSearch->setStyleSheet(QString::fromUtf8("background-color: #0c1016; border: 1px solid #1e2836; border-radius: 6px; padding: 8px 14px; font-family: 'Inter'; font-size: 13px; color: #e8eef4;"));

        hubSearchLayout->addWidget(txtHubSearch);

        comboHubSelector = new QComboBox(hubSearchBarWidget);
        comboHubSelector->setObjectName("comboHubSelector");
        comboHubSelector->setMinimumSize(QSize(190, 36));
        comboHubSelector->setStyleSheet(QString::fromUtf8("QComboBox { background-color: #0c1016; border: 1px solid #00ff66; border-radius: 6px; padding: 6px 12px; color: #00ff66; font-weight: bold; font-size: 12px; } QComboBox::drop-down { border: none; } QComboBox QAbstractItemView { background-color: #07090c; color: #e8eef4; selection-background-color: rgba(0,255,102,0.2); selection-color: #00ff66; }"));

        hubSearchLayout->addWidget(comboHubSelector);

        btnHubRefresh = new QPushButton(hubSearchBarWidget);
        btnHubRefresh->setObjectName("btnHubRefresh");
        btnHubRefresh->setStyleSheet(QString::fromUtf8("background: rgba(0,255,102,0.15); color: #00ff66; border: 1px solid rgba(0,255,102,0.4); border-radius: 6px; padding: 8px 16px; font-weight: bold;"));

        hubSearchLayout->addWidget(btnHubRefresh);


        hubMainLayout->addWidget(hubSearchBarWidget);

        hubSplitter = new QSplitter(pageHub);
        hubSplitter->setObjectName("hubSplitter");
        hubSplitter->setOrientation(Qt::Horizontal);
        listHubScripts = new QListWidget(hubSplitter);
        listHubScripts->setObjectName("listHubScripts");
        listHubScripts->setStyleSheet(QString::fromUtf8("QListWidget { background-color: #0c1016; border: 1px solid #1e2836; border-radius: 6px; padding: 6px; color: #e8eef4; } QListWidget::item { padding: 10px; border-bottom: 1px solid #141b24; border-radius: 4px; } QListWidget::item:hover { background-color: rgba(0,255,102,0.06); } QListWidget::item:selected { background-color: rgba(0,255,102,0.15); color: #00ff66; font-weight: bold; }"));
        hubSplitter->addWidget(listHubScripts);
        hubDetailsPanel = new QWidget(hubSplitter);
        hubDetailsPanel->setObjectName("hubDetailsPanel");
        hubDetailsPanel->setStyleSheet(QString::fromUtf8("background-color: #07090c; border: 1px solid #1e2836; border-radius: 6px; padding: 12px;"));
        hubDetailsLayout = new QVBoxLayout(hubDetailsPanel);
        hubDetailsLayout->setObjectName("hubDetailsLayout");
        lblScriptTitle = new QLabel(hubDetailsPanel);
        lblScriptTitle->setObjectName("lblScriptTitle");
        lblScriptTitle->setStyleSheet(QString::fromUtf8("font-size: 15px; font-weight: bold; color: #00ff66;"));

        hubDetailsLayout->addWidget(lblScriptTitle);

        lblScriptMeta = new QLabel(hubDetailsPanel);
        lblScriptMeta->setObjectName("lblScriptMeta");
        lblScriptMeta->setStyleSheet(QString::fromUtf8("color: #8b9bb0; font-size: 11px;"));

        hubDetailsLayout->addWidget(lblScriptMeta);

        txtScriptPreview = new QPlainTextEdit(hubDetailsPanel);
        txtScriptPreview->setObjectName("txtScriptPreview");
        txtScriptPreview->setReadOnly(true);
        txtScriptPreview->setStyleSheet(QString::fromUtf8("background-color: #0c1016; border: 1px solid #1e2836; color: #e8eef4; font-family: 'JetBrains Mono'; font-size: 12px;"));

        hubDetailsLayout->addWidget(txtScriptPreview);

        hubActionButtonsLayout = new QHBoxLayout();
        hubActionButtonsLayout->setObjectName("hubActionButtonsLayout");
        btnLoadToEditor = new QPushButton(hubDetailsPanel);
        btnLoadToEditor->setObjectName("btnLoadToEditor");
        btnLoadToEditor->setStyleSheet(QString::fromUtf8("background: rgba(0,255,102,0.15); color: #00ff66; border: 1px solid rgba(0,255,102,0.4); border-radius: 6px; padding: 8px 16px; font-weight: bold;"));

        hubActionButtonsLayout->addWidget(btnLoadToEditor);

        btnExecuteDirect = new QPushButton(hubDetailsPanel);
        btnExecuteDirect->setObjectName("btnExecuteDirect");
        btnExecuteDirect->setStyleSheet(QString::fromUtf8("background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00ff66, stop:1 #00cc52); color: #040608; border-radius: 6px; padding: 8px 16px; font-weight: bold;"));

        hubActionButtonsLayout->addWidget(btnExecuteDirect);


        hubDetailsLayout->addLayout(hubActionButtonsLayout);

        hubSplitter->addWidget(hubDetailsPanel);

        hubMainLayout->addWidget(hubSplitter);

        pageStack->addWidget(pageHub);

        bodyLayout->addWidget(pageStack);


        mainVerticalLayout->addWidget(bodyWidget);

        statusBar = new QWidget(centralWidget);
        statusBar->setObjectName("statusBar");
        statusLayout = new QHBoxLayout(statusBar);
        statusLayout->setObjectName("statusLayout");
        statusLayout->setContentsMargins(12, -1, 12, -1);
        lblStatusDot = new QLabel(statusBar);
        lblStatusDot->setObjectName("lblStatusDot");
        lblStatusDot->setStyleSheet(QString::fromUtf8("color: #00ff66; font-weight: bold;"));

        statusLayout->addWidget(lblStatusDot);

        lblPlan = new QLabel(statusBar);
        lblPlan->setObjectName("lblPlan");
        lblPlan->setStyleSheet(QString::fromUtf8("color: #8b9bb0;"));

        statusLayout->addWidget(lblPlan);

        statusSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        statusLayout->addItem(statusSpacer);

        lblRuntimeSpec = new QLabel(statusBar);
        lblRuntimeSpec->setObjectName("lblRuntimeSpec");
        lblRuntimeSpec->setStyleSheet(QString::fromUtf8("color: #8b9bb0; font-family: 'JetBrains Mono';"));

        statusLayout->addWidget(lblRuntimeSpec);


        mainVerticalLayout->addWidget(statusBar);

        MainWindow->setCentralWidget(centralWidget);

        retranslateUi(MainWindow);

        pageStack->setCurrentIndex(0);
        editorTabs->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "VOLTRO EXECUTOR v1.5.0", nullptr));
        lblTitleLogoIcon->setText(QString());
        lblTitleLogo->setText(QCoreApplication::translate("MainWindow", "VOLTRO EXECUTOR", nullptr));
        lblBadge->setText(QCoreApplication::translate("MainWindow", "v1.5.0 PRE-RELEASE", nullptr));
        btnMin->setText(QCoreApplication::translate("MainWindow", "\342\200\224", nullptr));
        btnMax->setText(QCoreApplication::translate("MainWindow", "\342\226\242", nullptr));
        btnClose->setText(QCoreApplication::translate("MainWindow", "\342\234\225", nullptr));
        btnNavWorkspace->setText(QCoreApplication::translate("MainWindow", "\342\214\250 Workspace", nullptr));
        btnNavHub->setText(QCoreApplication::translate("MainWindow", "\360\237\214\220 Script Hub", nullptr));
        btnNavPlans->setText(QCoreApplication::translate("MainWindow", "\360\237\221\221 Store & Keys", nullptr));
        btnNavSettings->setText(QCoreApplication::translate("MainWindow", "\342\232\231 Settings", nullptr));
        btnNavCredits->setText(QCoreApplication::translate("MainWindow", "\360\237\221\245 Credits", nullptr));
        lblSideStatus->setText(QCoreApplication::translate("MainWindow", "\342\227\217 Not Attached", nullptr));
        btnAttach->setText(QCoreApplication::translate("MainWindow", "\342\232\241 Attach", nullptr));
        btnExecute->setText(QCoreApplication::translate("MainWindow", "\342\226\266 Execute", nullptr));
        btnStop->setText(QCoreApplication::translate("MainWindow", "\342\217\271 Stop", nullptr));
        btnClear->setText(QCoreApplication::translate("MainWindow", "\360\237\227\221 Clear", nullptr));
        btnFpsBoost->setText(QCoreApplication::translate("MainWindow", "\342\232\241 FPS Boost [240]", nullptr));
        btnNewTab->setText(QCoreApplication::translate("MainWindow", "+ New Tab", nullptr));
        txtEditor->setPlainText(QCoreApplication::translate("MainWindow", "-- [[ Voltro Executor v1.5.0 ]]\n"
"printidentity()\n"
"local name, ver = identifyexecutor()\n"
"print(\"[VOLTRO] Injected runtime: \" .. tostring(name) .. \" \" .. tostring(ver))\n"
"print(\"[VOLTRO] Engine v1.5.0 Active\")\n"
"", nullptr));
        editorTabs->setTabText(editorTabs->indexOf(tabMain), QCoreApplication::translate("MainWindow", "main.lua", nullptr));
        lblConsoleHeader->setText(QCoreApplication::translate("MainWindow", "TERMINAL OUTPUT STREAM (IPC: \\\\.\\pipe\\VoltroIPC)", nullptr));
        txtConsole->setPlainText(QCoreApplication::translate("MainWindow", "[VOLTRO] System initialized. Voltro Virtual Sandbox ready.\n"
"[IPC] Listening on \\\\.\\pipe\\VoltroIPC.\n"
"", nullptr));
        txtHubSearch->setPlaceholderText(QCoreApplication::translate("MainWindow", "\360\237\224\215 Search exploit scripts across hubs (e.g. Infinite Yield, Aimbot, Blox Fruits)...", nullptr));
        btnHubRefresh->setText(QCoreApplication::translate("MainWindow", "\360\237\224\204 Search", nullptr));
        lblScriptTitle->setText(QCoreApplication::translate("MainWindow", "Select a script from the list", nullptr));
        lblScriptMeta->setText(QCoreApplication::translate("MainWindow", "Hub Source: \342\200\224 | Author: \342\200\224", nullptr));
        btnLoadToEditor->setText(QCoreApplication::translate("MainWindow", "\360\237\223\245 Load into Editor", nullptr));
        btnExecuteDirect->setText(QCoreApplication::translate("MainWindow", "\342\232\241 Execute Instantly", nullptr));
        lblStatusDot->setText(QCoreApplication::translate("MainWindow", "\342\227\217 [Ready]", nullptr));
        lblPlan->setText(QCoreApplication::translate("MainWindow", "Free Plan", nullptr));
        lblRuntimeSpec->setText(QCoreApplication::translate("MainWindow", "Voltro Luau Runtime (v1.5.0)", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H

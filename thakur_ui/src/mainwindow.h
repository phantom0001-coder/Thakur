

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

namespace Thakur {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    
    void onNavigateWorkspace();
    void onNavigateHub();
    void onNavigatePlans();
    void onNavigateSettings();
    void onNavigateCredits();

    
    void onNewTab();
    void onCloseTab(int index);
    void onTabRenamed(int index, const QString& newName);
    void onAttachClicked();
    void onExecuteClicked();
    void onStopClicked();
    void onClearClicked();
    void onFpsBoostClicked();
    void onSaveClicked();
    void onOpenClicked();

    
    void onToggleFindReplace(bool showReplace);
    void onFindNext();
    void onReplace();
    void onReplaceAll();

    
    void onThemeChanged(const QString& themeName);
    void onAccentChanged(const QString& hexColor);
    void onIpcModeChanged(const QString& mode);

    
    void onHubSearch();
    void onHubSourceChanged(int index);
    void onScriptSelected(int row);
    void onLoadScriptToEditor();
    void onExecuteScriptDirect();

private:
    void setupCustomTitleBar();
    void setupScriptHub();
    void applyThemePalette(const QString& theme);
    void initializeThakurSandbox();
    void addOutputLog(const QString& message, const QString& level = "info");

    Ui::MainWindow *ui;
    QPoint m_dragPosition;
    bool m_isAttached = false;
    bool m_hasKey = false;
    QString m_currentPlan = "Free";
    std::vector<ScriptItem> m_currentHubScripts;
    int m_selectedScriptIndex = -1;
};

} 

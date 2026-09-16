




#include "ThemeManager.h"

#include <QApplication>
#include <QWidget>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>
#include <QDebug>

namespace Voltro {



ThemeManager& ThemeManager::instance()
{
    static ThemeManager s_instance;
    return s_instance;
}

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
    loadBuiltinThemes();
    
    if (m_themes.contains("dark"))
        loadTheme("dark");
}



void ThemeManager::loadBuiltinThemes()
{
    
    const QStringList builtins = {
        {":/themes/dark.json"},
        {":/themes/midnight.json"},
        {":/themes/glitched.json"}
    };
    const QStringList ids = {"dark", "midnight", "glitched"};

    for (int i = 0; i < builtins.size(); ++i) {
        if (!loadThemeFromResource(builtins[i], ids[i])) {
            qWarning() << "[ThemeManager] Resource not found:" << builtins[i]
                       << "– using hardcoded fallback";
        }
    }

    
    if (!m_themes.contains("dark")) {
        QJsonObject colors;
        colors["background"]  = "#07090c";
        colors["background2"] = "#0c1016";
        colors["card"]        = "#10151c";
        colors["border"]      = "#1e2836";
        colors["foreground"]  = "#e8eef4";
        colors["foreground2"] = "#c8d0dc";
        colors["muted"]       = "#8b9bb0";
        colors["accent"]      = "#00ff66";
        colors["accent2"]     = "#00cc52";
        colors["accent3"]     = "#39ff14";
        colors["ok"]          = "#00ff66";
        colors["error"]       = "#ff3b5c";
        colors["warning"]     = "#ffb800";
        colors["info"]        = "#00e5ff";

        QJsonObject obj;
        obj["name"]     = "Dark";
        obj["id"]       = "dark";
        obj["colors"]   = colors;
        obj["glitched"] = false;
        obj["animated"] = false;

        m_themes["dark"] = obj;
    }

    
    if (!m_themes.contains("midnight")) {
        QJsonObject colors;
        colors["background"]  = "#020208";
        colors["background2"] = "#06040f";
        colors["card"]        = "#0a0818";
        colors["border"]      = "#1a1230";
        colors["foreground"]  = "#e2dff8";
        colors["foreground2"] = "#c4bfee";
        colors["muted"]       = "#7a72a8";
        colors["accent"]      = "#8b5cf6";
        colors["accent2"]     = "#7c3aed";
        colors["accent3"]     = "#a78bfa";
        colors["ok"]          = "#34d399";
        colors["error"]       = "#f87171";
        colors["warning"]     = "#fbbf24";
        colors["info"]        = "#38bdf8";

        QJsonObject obj;
        obj["name"]     = "Midnight";
        obj["id"]       = "midnight";
        obj["colors"]   = colors;
        obj["glitched"] = false;
        obj["animated"] = false;

        m_themes["midnight"] = obj;
    }

    
    if (!m_themes.contains("glitched")) {
        QJsonObject base = m_themes.value("dark");
        base["name"]             = "Glitched";
        base["id"]               = "glitched";
        base["glitched"]         = true;
        base["animated"]         = true;
        base["glitchIntensity"]  = 70;
        base["scanLines"]        = true;
        base["titleFlicker"]     = true;
        base["animatedBorders"]  = true;
        base["glitchIntervalMs"] = 3000;
        base["glitchDurationMs"] = 150;
        m_themes["glitched"] = base;
    }
}

bool ThemeManager::loadThemeFromResource(const QString& resourcePath, const QString& id)
{
    QFile f(resourcePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();

    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[ThemeManager] Parse error in" << resourcePath << ":" << err.errorString();
        return false;
    }

    m_themes[id] = doc.object();
    return true;
}



bool ThemeManager::loadTheme(const QString& name)
{
    if (!m_themes.contains(name)) {
        qWarning() << "[ThemeManager] Unknown theme:" << name;
        return false;
    }

    m_current      = m_themes[name];
    m_currentTheme = name;

    emit themeChanged(name);
    return true;
}

void ThemeManager::applyToApp(QApplication* app)
{
    if (!app) return;
    app->setStyle(QStyleFactory::create("Fusion"));
    app->setPalette(buildPalette());
    app->setStyleSheet(buildStyleSheet());
}

void ThemeManager::applyToWidget(QWidget* widget)
{
    if (!widget) return;
    widget->setStyleSheet(buildStyleSheet());
}

QString ThemeManager::generateStyleSheet() const
{
    return buildStyleSheet();
}

bool ThemeManager::addCustomTheme(const QString& name, const QJsonObject& data)
{
    if (name.isEmpty() || !data.contains("colors"))
        return false;
    m_themes[name] = data;
    return true;
}

QStringList ThemeManager::availableThemes() const
{
    return m_themes.keys();
}

QString ThemeManager::currentTheme() const
{
    return m_currentTheme;
}



QColor ThemeManager::color(const QString& key, const QString& fallback) const
{
    const QJsonObject cols = m_current["colors"].toObject();
    const QString hex = cols.value(key).toString(fallback);
    return QColor(hex);
}

QColor ThemeManager::accent()     const { return color("accent",     "#00ff66"); }
QColor ThemeManager::background() const { return color("background", "#07090c"); }
QColor ThemeManager::surface()    const { return color("card",       "#10151c"); }
QColor ThemeManager::text()       const { return color("foreground", "#e8eef4"); }
QColor ThemeManager::muted()      const { return color("muted",      "#8b9bb0"); }
QColor ThemeManager::error()      const { return color("error",      "#ff3b5c"); }
QColor ThemeManager::warning()    const { return color("warning",    "#ffb800"); }
QColor ThemeManager::info()       const { return color("info",       "#00e5ff"); }



bool ThemeManager::isGlitched()         const { return m_current["glitched"].toBool(false); }
bool ThemeManager::isAnimated()         const { return m_current["animated"].toBool(false); }
int  ThemeManager::glitchIntensity()    const { return m_current["glitchIntensity"].toInt(50); }
bool ThemeManager::hasScanLines()       const { return m_current["scanLines"].toBool(false); }
bool ThemeManager::hasTitleFlicker()    const { return m_current["titleFlicker"].toBool(false); }
bool ThemeManager::hasAnimatedBorders() const { return m_current["animatedBorders"].toBool(false); }
int  ThemeManager::glitchIntervalMs()   const { return m_current["glitchIntervalMs"].toInt(4000); }
int  ThemeManager::glitchDurationMs()   const { return m_current["glitchDurationMs"].toInt(120); }



QPalette ThemeManager::buildPalette() const
{
    QPalette p;

    const QColor bg     = background();
    const QColor bg2    = color("background2", "#0c1016");
    const QColor card   = surface();
    const QColor border = color("border",      "#1e2836");
    const QColor fg     = text();
    const QColor fg2    = color("foreground2", "#c8d0dc");
    const QColor mut    = muted();
    const QColor acc    = accent();
    const QColor err    = error();

    
    p.setColor(QPalette::Window,          bg);
    p.setColor(QPalette::WindowText,      fg);
    p.setColor(QPalette::Base,            bg2);
    p.setColor(QPalette::AlternateBase,   card);
    p.setColor(QPalette::ToolTipBase,     card);
    p.setColor(QPalette::ToolTipText,     fg);
    p.setColor(QPalette::Text,            fg);
    p.setColor(QPalette::Button,          card);
    p.setColor(QPalette::ButtonText,      fg);
    p.setColor(QPalette::BrightText,      fg);

    
    p.setColor(QPalette::Highlight,       acc);
    p.setColor(QPalette::HighlightedText, bg);

    
    p.setColor(QPalette::Link,            acc);
    p.setColor(QPalette::LinkVisited,     color("accent2", "#00cc52"));

    
    p.setColor(QPalette::Disabled, QPalette::WindowText, mut);
    p.setColor(QPalette::Disabled, QPalette::Text,       mut);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, mut);
    p.setColor(QPalette::Disabled, QPalette::Base,       bg);
    p.setColor(QPalette::Disabled, QPalette::Button,     bg);

    
    p.setColor(QPalette::Shadow,   border);
    p.setColor(QPalette::Mid,      border);
    p.setColor(QPalette::Dark,     bg2);
    p.setColor(QPalette::Midlight, card);
    p.setColor(QPalette::Light,    color("accent3", "#39ff14"));

    Q_UNUSED(fg2)
    Q_UNUSED(err)

    return p;
}



QString ThemeManager::buildStyleSheet() const
{
    
    const QString bg      = color("background",  "#07090c").name();
    const QString bg2     = color("background2", "#0c1016").name();
    const QString card    = color("card",        "#10151c").name();
    const QString border  = color("border",      "#1e2836").name();
    const QString fg      = color("foreground",  "#e8eef4").name();
    const QString fg2     = color("foreground2", "#c8d0dc").name();
    const QString mut     = color("muted",       "#8b9bb0").name();
    const QString acc     = color("accent",      "#00ff66").name();
    const QString acc2    = color("accent2",     "#00cc52").name();
    const QString acc3    = color("accent3",     "#39ff14").name();
    const QString err     = color("error",       "#ff3b5c").name();
    const QString warn    = color("warning",     "#ffb800").name();
    const QString info    = color("info",        "#00e5ff").name();

    
    
    auto accentAlpha = [&](int alpha) -> QString {
        QColor c(acc);
        c.setAlpha(alpha);
        
        return QString("#%1%2%3%4")
               .arg(c.alpha(),  2, 16, QChar('0'))
               .arg(c.red(),    2, 16, QChar('0'))
               .arg(c.green(),  2, 16, QChar('0'))
               .arg(c.blue(),   2, 16, QChar('0'));
    };
    const QString accGlow  = accentAlpha(40);   
    const QString accSel   = accentAlpha(60);   
    const QString accBorder= accentAlpha(180);  

    QString ss;
    ss.reserve(16384);

    
    
    
    ss += QString(R"(

QWidget {
    background-color: %1;
    color: %2;
    font-family: "Segoe UI", "Inter", sans-serif;
    font-size: 13px;
    selection-background-color: %3;
    selection-color: %4;
}
QMainWindow {
    background-color: %1;
}
QFrame {
    background-color: transparent;
    border: none;
}
)").arg(bg, fg, acc, bg);

    
    
    
    ss += QString(R"(

QPushButton {
    background-color: %1;
    color: %2;
    border: 1px solid %3;
    border-radius: 6px;
    padding: 6px 18px;
    font-weight: 600;
    min-height: 28px;
}
QPushButton:hover {
    background-color: %4;
    border-color: %5;
    color: %6;
}
QPushButton:pressed {
    background-color: %7;
    border-color: %8;
    color: %9;
}
QPushButton:disabled {
    background-color: %10;
    color: %11;
    border-color: %12;
}
QPushButton[class="primary"] {
    background-color: %5;
    color: %9;
    border-color: %5;
}
QPushButton[class="primary"]:hover {
    background-color: %8;
    border-color: %8;
}
QPushButton[class="danger"] {
    background-color: transparent;
    color: %13;
    border-color: %13;
}
QPushButton[class="danger"]:hover {
    background-color: %13;
    color: %9;
}
)").arg(
        card,        
        fg,          
        border,      
        accGlow,     
        acc,         
        acc,         
        acc,         
        acc2,        
        bg,          
        bg2,         
        mut,         
        border,      
        err          
    );

    
    
    
    ss += QString(R"(

QTabWidget::pane {
    background-color: %1;
    border: 1px solid %2;
    border-top: none;
    border-bottom-left-radius: 6px;
    border-bottom-right-radius: 6px;
}
QTabWidget::tab-bar {
    alignment: left;
}
QTabBar {
    background-color: %3;
}
QTabBar::tab {
    background-color: %3;
    color: %4;
    padding: 8px 20px;
    border: none;
    border-bottom: 2px solid transparent;
    margin-right: 2px;
    min-width: 80px;
}
QTabBar::tab:hover {
    color: %5;
    background-color: %1;
}
QTabBar::tab:selected {
    color: %5;
    background-color: %1;
    border-bottom: 2px solid %5;
    font-weight: 600;
}
QTabBar::tab:!selected:hover {
    border-bottom: 2px solid %6;
}
)").arg(
        card,    
        border,  
        bg2,     
        mut,     
        acc,     
        accGlow  
    );

    
    
    
    ss += QString(R"(

QPlainTextEdit, QTextEdit {
    background-color: %1;
    color: %2;
    border: 1px solid %3;
    border-radius: 4px;
    padding: 4px;
    font-family: "JetBrains Mono", "Consolas", "Courier New", monospace;
    font-size: 13px;
    selection-background-color: %4;
    selection-color: %5;
}
QPlainTextEdit:focus, QTextEdit:focus {
    border-color: %6;
}
)").arg(
        bg2,        
        fg,         
        border,     
        accSel,     
        fg,         
        acc         
    );

    
    
    
    ss += QString(R"(

QListWidget, QTreeWidget, QListView, QTreeView {
    background-color: %1;
    color: %2;
    border: 1px solid %3;
    border-radius: 4px;
    outline: none;
    alternate-background-color: %4;
}
QListWidget::item, QTreeWidget::item,
QListView::item,  QTreeView::item {
    padding: 4px 8px;
    border-radius: 3px;
}
QListWidget::item:hover, QTreeWidget::item:hover,
QListView::item:hover,   QTreeView::item:hover {
    background-color: %5;
    color: %6;
}
QListWidget::item:selected, QTreeWidget::item:selected,
QListView::item:selected,   QTreeView::item:selected {
    background-color: %7;
    color: %8;
    border-left: 2px solid %6;
}
QTreeWidget::branch {
    background-color: %1;
}
QTreeWidget::branch:has-siblings:!adjoins-item {
    border-image: none;
}
QTreeWidget::branch:open:has-children {
    image: none;
    border: none;
}
QHeaderView::section {
    background-color: %4;
    color: %9;
    padding: 4px 8px;
    border: none;
    border-bottom: 1px solid %3;
    font-weight: 600;
}
)").arg(
        bg2,      
        fg,       
        border,   
        card,     
        accGlow,  
        acc,      
        accSel,   
        fg,       
        mut       
    );

    
    
    
    ss += QString(R"(

QScrollBar:vertical {
    background-color: %1;
    width: 8px;
    border: none;
    border-radius: 4px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background-color: %2;
    border-radius: 4px;
    min-height: 24px;
}
QScrollBar::handle:vertical:hover {
    background-color: %3;
}
QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical {
    height: 0;
    background: none;
}
QScrollBar:horizontal {
    background-color: %1;
    height: 8px;
    border: none;
    border-radius: 4px;
    margin: 0;
}
QScrollBar::handle:horizontal {
    background-color: %2;
    border-radius: 4px;
    min-width: 24px;
}
QScrollBar::handle:horizontal:hover {
    background-color: %3;
}
QScrollBar::add-line:horizontal,
QScrollBar::sub-line:horizontal {
    width: 0;
    background: none;
}
QScrollBar::corner {
    background-color: %1;
}
)").arg(
        bg2,     
        border,  
        acc      
    );

    
    
    
    ss += QString(R"(

QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {
    background-color: %1;
    color: %2;
    border: 1px solid %3;
    border-radius: 5px;
    padding: 5px 8px;
    min-height: 24px;
}
QLineEdit:focus, QComboBox:focus,
QSpinBox:focus, QDoubleSpinBox:focus {
    border-color: %4;
    background-color: %5;
}
QLineEdit:disabled, QComboBox:disabled,
QSpinBox:disabled, QDoubleSpinBox:disabled {
    color: %6;
    border-color: %3;
    background-color: %7;
}
QLineEdit[readOnly="true"] {
    color: %6;
    background-color: %7;
}
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 22px;
    border-left: 1px solid %3;
    border-radius: 0 5px 5px 0;
}
QComboBox::down-arrow {
    image: none;
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 5px solid %2;
    width: 0;
    height: 0;
}
QComboBox QAbstractItemView {
    background-color: %1;
    color: %2;
    border: 1px solid %4;
    selection-background-color: %8;
    selection-color: %2;
    border-radius: 4px;
    padding: 2px;
}
QSpinBox::up-button, QSpinBox::down-button,
QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {
    background-color: %3;
    border: none;
    width: 16px;
}
QSpinBox::up-button:hover, QSpinBox::down-button:hover,
QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover {
    background-color: %4;
}
)").arg(
        bg2,      
        fg,       
        border,   
        acc,      
        card,     
        mut,      
        bg,       
        accSel    
    );

    
    
    
    ss += QString(R"(

QLabel {
    background-color: transparent;
    color: %1;
    border: none;
    padding: 0;
}
QLabel[class="title"] {
    font-size: 18px;
    font-weight: 700;
    color: %2;
}
QLabel[class="subtitle"] {
    font-size: 14px;
    color: %3;
}
QLabel[class="muted"] {
    color: %3;
    font-size: 12px;
}
QLabel[class="accent"] {
    color: %2;
    font-weight: 600;
}
QLabel[class="error"] {
    color: %4;
}
QLabel[class="warning"] {
    color: %5;
}
QLabel[class="info"] {
    color: %6;
}
)").arg(fg, acc, mut, err, warn, info);

    
    
    
    ss += QString(R"(

QSplitter::handle {
    background-color: %1;
}
QSplitter::handle:horizontal {
    width: 2px;
}
QSplitter::handle:vertical {
    height: 2px;
}
QSplitter::handle:hover {
    background-color: %2;
}
)").arg(border, acc);

    
    
    
    ss += QString(R"(

QMenuBar {
    background-color: %1;
    color: %2;
    border-bottom: 1px solid %3;
    padding: 2px 4px;
}
QMenuBar::item {
    background-color: transparent;
    padding: 4px 10px;
    border-radius: 4px;
}
QMenuBar::item:selected, QMenuBar::item:pressed {
    background-color: %4;
    color: %5;
}
QMenu {
    background-color: %6;
    color: %2;
    border: 1px solid %3;
    border-radius: 6px;
    padding: 4px;
}
QMenu::item {
    padding: 6px 24px 6px 12px;
    border-radius: 4px;
}
QMenu::item:selected {
    background-color: %4;
    color: %5;
}
QMenu::item:disabled {
    color: %7;
}
QMenu::separator {
    height: 1px;
    background-color: %3;
    margin: 4px 8px;
}
QMenu::indicator {
    width: 14px;
    height: 14px;
}
)").arg(
        bg2,     
        fg,      
        border,  
        accGlow, 
        acc,     
        card,    
        mut      
    );

    
    
    
    ss += QString(R"(

QToolTip {
    background-color: %1;
    color: %2;
    border: 1px solid %3;
    border-radius: 4px;
    padding: 4px 8px;
    font-size: 12px;
    opacity: 230;
}
)").arg(card, fg, acc);

    
    
    
    ss += QString(R"(

QCheckBox, QRadioButton {
    background-color: transparent;
    color: %1;
    spacing: 6px;
}
QCheckBox::indicator, QRadioButton::indicator {
    width: 16px;
    height: 16px;
    border: 1px solid %2;
    border-radius: 3px;
    background-color: %3;
}
QRadioButton::indicator {
    border-radius: 8px;
}
QCheckBox::indicator:hover, QRadioButton::indicator:hover {
    border-color: %4;
}
QCheckBox::indicator:checked, QRadioButton::indicator:checked {
    background-color: %4;
    border-color: %4;
}
QCheckBox::indicator:checked:hover, QRadioButton::indicator:checked:hover {
    background-color: %5;
    border-color: %5;
}
QCheckBox:disabled, QRadioButton:disabled {
    color: %6;
}
QCheckBox::indicator:disabled, QRadioButton::indicator:disabled {
    border-color: %2;
    background-color: %7;
}
)").arg(fg, border, bg2, acc, acc2, mut, bg);

    
    
    
    ss += QString(R"(

QGroupBox {
    background-color: %1;
    border: 1px solid %2;
    border-radius: 6px;
    margin-top: 20px;
    padding: 12px 8px 8px 8px;
    font-weight: 600;
    color: %3;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    padding: 0 6px;
    left: 10px;
    color: %4;
}
)").arg(card, border, fg, acc);

    
    
    
    ss += QString(R"(

QProgressBar {
    background-color: %1;
    border: 1px solid %2;
    border-radius: 4px;
    text-align: center;
    color: %3;
    height: 10px;
}
QProgressBar::chunk {
    background-color: %4;
    border-radius: 3px;
}
)").arg(bg2, border, fg, acc);

    
    
    
    ss += QString(R"(

QSlider::groove:horizontal {
    height: 4px;
    background-color: %1;
    border-radius: 2px;
}
QSlider::handle:horizontal {
    background-color: %2;
    border: 2px solid %3;
    width: 14px;
    height: 14px;
    margin: -5px 0;
    border-radius: 7px;
}
QSlider::handle:horizontal:hover {
    background-color: %4;
    border-color: %4;
}
QSlider::sub-page:horizontal {
    background-color: %2;
    border-radius: 2px;
}
)").arg(border, acc, bg, acc2);

    
    
    
    ss += QString(R"(

QStatusBar {
    background-color: %1;
    color: %2;
    border-top: 1px solid %3;
}
QStatusBar::item {
    border: none;
}
)").arg(bg2, mut, border);

    
    
    
    ss += QString(R"(

QToolBar {
    background-color: %1;
    border-bottom: 1px solid %2;
    spacing: 4px;
    padding: 2px 4px;
}
QToolBar::separator {
    width: 1px;
    background-color: %2;
    margin: 4px 2px;
}
QToolButton {
    background-color: transparent;
    color: %3;
    border: 1px solid transparent;
    border-radius: 4px;
    padding: 4px 8px;
}
QToolButton:hover {
    background-color: %4;
    border-color: %5;
    color: %5;
}
QToolButton:pressed, QToolButton:checked {
    background-color: %6;
    color: %7;
}
)").arg(bg2, border, fg, accGlow, acc, acc, bg);

    
    
    
    ss += QString(R"(

QDialog {
    background-color: %1;
    border: 1px solid %2;
}
QDialogButtonBox QPushButton {
    min-width: 80px;
}
)").arg(card, border);

    return ss;
}

} 

#pragma once
// ============================================================
//  ThemeManager.h  –  Voltro Executor  (Phase 2)
//  Singleton that loads, stores, and applies UI themes.
// ============================================================

#ifndef VOLTRO_THEMEMANAGER_H
#define VOLTRO_THEMEMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QColor>
#include <QMap>
#include <QPalette>
#include <QJsonObject>

class QApplication;
class QWidget;

namespace Voltro {

// ============================================================
//  ThemeManager
//  Usage:
//      ThemeManager::instance().loadTheme("dark");
//      ThemeManager::instance().applyToApp(qApp);
// ============================================================
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    // ── Singleton access ─────────────────────────────────────
    static ThemeManager& instance();

    // Prevent copy / move
    ThemeManager(const ThemeManager&)            = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    // ── Theme management ─────────────────────────────────────

    /// Load a theme by id (e.g. "dark", "midnight", "glitched").
    /// Returns true on success.
    bool loadTheme(const QString& name);

    /// Apply the current theme palette + stylesheet to the whole app.
    void applyToApp(QApplication* app);

    /// Apply the current theme stylesheet to a single widget subtree.
    void applyToWidget(QWidget* widget);

    /// Generate (and return) the complete Qt stylesheet for the active theme.
    QString generateStyleSheet() const;

    /// Register a custom theme from a JSON object.
    /// The object must contain the same structure as the JSON files.
    bool addCustomTheme(const QString& name, const QJsonObject& data);

    // ── Queries ──────────────────────────────────────────────
    QStringList availableThemes() const;
    QString     currentTheme()    const;

    // ── Color helpers ────────────────────────────────────────
    QColor accent()     const;
    QColor background() const;
    QColor surface()    const;   ///< "card" surface color
    QColor text()       const;
    QColor muted()      const;
    QColor error()      const;
    QColor warning()    const;
    QColor info()       const;

    // ── Effect queries (for glitched theme) ──────────────────
    bool isGlitched()        const;
    bool isAnimated()        const;
    int  glitchIntensity()   const;
    bool hasScanLines()      const;
    bool hasTitleFlicker()   const;
    bool hasAnimatedBorders()const;
    int  glitchIntervalMs()  const;
    int  glitchDurationMs()  const;

signals:
    /// Emitted after a successful loadTheme() call.
    void themeChanged(const QString& name);

private:
    // Private constructor – use instance()
    explicit ThemeManager(QObject* parent = nullptr);
    ~ThemeManager() override = default;

    // ── Internal helpers ─────────────────────────────────────
    void    loadBuiltinThemes();
    bool    loadThemeFromResource(const QString& resourcePath, const QString& id);
    QColor  color(const QString& key, const QString& fallback = "#ffffff") const;

    QString  buildStyleSheet() const;
    QPalette buildPalette()    const;

    // ── State ─────────────────────────────────────────────────
    QMap<QString, QJsonObject> m_themes;       ///< id → JSON object
    QJsonObject                m_current;      ///< active theme JSON
    QString                    m_currentTheme; ///< active theme id
};

} // namespace Voltro

#endif // VOLTRO_THEMEMANAGER_H

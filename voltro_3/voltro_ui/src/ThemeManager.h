#pragma once





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







class ThemeManager : public QObject
{
    Q_OBJECT

public:
    
    static ThemeManager& instance();

    
    ThemeManager(const ThemeManager&)            = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    

    
    
    bool loadTheme(const QString& name);

    
    void applyToApp(QApplication* app);

    
    void applyToWidget(QWidget* widget);

    
    QString generateStyleSheet() const;

    
    
    bool addCustomTheme(const QString& name, const QJsonObject& data);

    
    QStringList availableThemes() const;
    QString     currentTheme()    const;

    
    QColor accent()     const;
    QColor background() const;
    QColor surface()    const;   
    QColor text()       const;
    QColor muted()      const;
    QColor error()      const;
    QColor warning()    const;
    QColor info()       const;

    
    bool isGlitched()        const;
    bool isAnimated()        const;
    int  glitchIntensity()   const;
    bool hasScanLines()      const;
    bool hasTitleFlicker()   const;
    bool hasAnimatedBorders()const;
    int  glitchIntervalMs()  const;
    int  glitchDurationMs()  const;

signals:
    
    void themeChanged(const QString& name);

private:
    
    explicit ThemeManager(QObject* parent = nullptr);
    ~ThemeManager() override = default;

    
    void    loadBuiltinThemes();
    bool    loadThemeFromResource(const QString& resourcePath, const QString& id);
    QColor  color(const QString& key, const QString& fallback = "#ffffff") const;

    QString  buildStyleSheet() const;
    QPalette buildPalette()    const;

    
    QMap<QString, QJsonObject> m_themes;       
    QJsonObject                m_current;      
    QString                    m_currentTheme; 
};

} 

#endif 

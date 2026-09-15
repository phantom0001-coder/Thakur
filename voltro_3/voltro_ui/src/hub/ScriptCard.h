#pragma once

// ============================================================
//  ScriptCard.h - Voltro Script Hub (Phase 3)
//  Individual script card widget with hover animation,
//  safety badge, hub badge, and action buttons.
// ============================================================

#include "HubManager.h"

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>

class QHBoxLayout;
class QVBoxLayout;
class QEnterEvent;

namespace Voltro {

class ScriptCard : public QFrame
{
    Q_OBJECT

    // Animated border color property
    Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor)

public:
    explicit ScriptCard(const ScriptMeta& meta, QWidget* parent = nullptr);
    ~ScriptCard() override = default;

    const ScriptMeta& meta() const;
    void              updateMeta(const ScriptMeta& meta);

    // Property accessors for animation
    QColor borderColor() const;
    void   setBorderColor(const QColor& color);

signals:
    void openInEditor(ScriptMeta meta);
    void executeDirect(ScriptMeta meta);
    void inspect(ScriptMeta meta);
    void favoriteToggled(QString id, bool favorited);

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event)      override;
    void paintEvent(QPaintEvent* event) override;

private:
    void buildUi();
    void applyTheme();
    void updateFavoriteButton();
    void updateSafetyBadge();
    void startHoverAnimation(bool hovering);

    // -- Helpers --
    static QString safetyLabel(SafetyVerdict v);
    static QColor  safetyColor(SafetyVerdict v);
    static QString verdictIcon(SafetyVerdict v);

    // -- Data --
    ScriptMeta m_meta;
    QColor     m_borderColor { QColor("#1e2530") };

    // -- Widgets --
    QLabel*       m_nameLabel        = nullptr;
    QLabel*       m_descLabel        = nullptr;
    QLabel*       m_authorLabel      = nullptr;
    QLabel*       m_viewsLabel       = nullptr;
    QLabel*       m_hubBadge         = nullptr;
    QLabel*       m_categoryTag      = nullptr;
    QLabel*       m_safetyBadge      = nullptr;
    QPushButton*  m_favoriteBtn      = nullptr;
    QPushButton*  m_openEditorBtn    = nullptr;
    QPushButton*  m_inspectBtn       = nullptr;
    QPushButton*  m_executeBtn       = nullptr;

    QPropertyAnimation*         m_borderAnim  = nullptr;
    QGraphicsDropShadowEffect*  m_shadow       = nullptr;
};

} // namespace Voltro

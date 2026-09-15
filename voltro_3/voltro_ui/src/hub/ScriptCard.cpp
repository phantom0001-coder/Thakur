// ============================================================
//  ScriptCard.cpp - Voltro Script Hub (Phase 3)
//  Full implementation of the script card widget.
//
//  Voltro dark theme:
//    Background  : #07090c
//    Card surface: #0d1117
//    Border idle : #1e2530
//    Border hover: #00ff66 (accent)
//    Text primary: #e6edf3
//    Text muted  : #7d8590
//    Accent      : #00ff66
// ============================================================

#include "ScriptCard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QEnterEvent>
#include <QFontMetrics>
#include <QSizePolicy>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>

namespace Voltro {

// ============================================================
//  Constructor
// ============================================================
ScriptCard::ScriptCard(const ScriptMeta& meta, QWidget* parent)
    : QFrame(parent)
    , m_meta(meta)
{
    setObjectName(QStringLiteral("ScriptCard"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(170);
    setMouseTracking(true);

    buildUi();
    applyTheme();
    updateSafetyBadge();
    updateFavoriteButton();

    // Drop-shadow effect
    m_shadow = new QGraphicsDropShadowEffect(this);
    m_shadow->setBlurRadius(10);
    m_shadow->setOffset(0, 2);
    m_shadow->setColor(QColor(0, 0, 0, 80));
    setGraphicsEffect(m_shadow);

    // Border color animation
    m_borderAnim = new QPropertyAnimation(this, "borderColor", this);
    m_borderAnim->setDuration(180);
    m_borderAnim->setEasingCurve(QEasingCurve::OutCubic);
}

// ============================================================
//  buildUi
// ============================================================
void ScriptCard::buildUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(14, 12, 14, 12);
    rootLayout->setSpacing(6);

    // --- Row 1: name + hub badge + safety badge + favorite ----
    auto* row1 = new QHBoxLayout;
    row1->setSpacing(8);

    m_nameLabel = new QLabel(m_meta.name, this);
    m_nameLabel->setObjectName(QStringLiteral("cardName"));
    m_nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_hubBadge = new QLabel(this);
    m_hubBadge->setObjectName(QStringLiteral("hubBadge"));
    m_hubBadge->setFixedHeight(20);

    m_safetyBadge = new QLabel(this);
    m_safetyBadge->setObjectName(QStringLiteral("safetyBadge"));
    m_safetyBadge->setFixedHeight(20);

    m_favoriteBtn = new QPushButton(this);
    m_favoriteBtn->setObjectName(QStringLiteral("favoriteBtn"));
    m_favoriteBtn->setFixedSize(26, 26);
    m_favoriteBtn->setFlat(true);
    m_favoriteBtn->setCursor(Qt::PointingHandCursor);
    m_favoriteBtn->setToolTip(QStringLiteral("Toggle Favourite"));

    row1->addWidget(m_nameLabel);
    row1->addWidget(m_hubBadge);
    row1->addWidget(m_safetyBadge);
    row1->addWidget(m_favoriteBtn);

    // --- Row 2: description (elided) ---------------------------
    m_descLabel = new QLabel(this);
    m_descLabel->setObjectName(QStringLiteral("cardDesc"));
    m_descLabel->setWordWrap(false);
    m_descLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // --- Row 3: author + views + category tag ------------------
    auto* row3 = new QHBoxLayout;
    row3->setSpacing(10);

    m_authorLabel = new QLabel(this);
    m_authorLabel->setObjectName(QStringLiteral("cardAuthor"));

    m_viewsLabel = new QLabel(this);
    m_viewsLabel->setObjectName(QStringLiteral("cardViews"));

    m_categoryTag = new QLabel(this);
    m_categoryTag->setObjectName(QStringLiteral("categoryTag"));
    m_categoryTag->setFixedHeight(18);

    row3->addWidget(m_authorLabel);
    row3->addWidget(m_viewsLabel);
    row3->addStretch();
    row3->addWidget(m_categoryTag);

    // --- Row 4: action buttons ---------------------------------
    auto* row4 = new QHBoxLayout;
    row4->setSpacing(8);

    m_openEditorBtn = new QPushButton(QStringLiteral("Open in Editor"), this);
    m_openEditorBtn->setObjectName(QStringLiteral("btnOpenEditor"));
    m_openEditorBtn->setCursor(Qt::PointingHandCursor);
    m_openEditorBtn->setFixedHeight(26);

    m_inspectBtn = new QPushButton(QStringLiteral("Inspect"), this);
    m_inspectBtn->setObjectName(QStringLiteral("btnInspect"));
    m_inspectBtn->setCursor(Qt::PointingHandCursor);
    m_inspectBtn->setFixedHeight(26);

    m_executeBtn = new QPushButton(QStringLiteral("Execute"), this);
    m_executeBtn->setObjectName(QStringLiteral("btnExecute"));
    m_executeBtn->setCursor(Qt::PointingHandCursor);
    m_executeBtn->setFixedHeight(26);

    row4->addStretch();
    row4->addWidget(m_openEditorBtn);
    row4->addWidget(m_inspectBtn);
    row4->addWidget(m_executeBtn);

    // --- Assemble ----------------------------------------------
    rootLayout->addLayout(row1);
    rootLayout->addWidget(m_descLabel);
    rootLayout->addLayout(row3);
    rootLayout->addStretch();
    rootLayout->addLayout(row4);

    // --- Fill initial values ----------------------------------
    // Author
    m_authorLabel->setText(
        m_meta.author.isEmpty()
            ? QStringLiteral("Unknown")
            : QStringLiteral("\u270e ") + m_meta.author);

    // Views
    m_viewsLabel->setText(
        m_meta.views > 0
            ? QStringLiteral("\U0001f441 ") + QString::number(m_meta.views)
            : QString());

    // Category tag
    if (!m_meta.category.isEmpty()) {
        m_categoryTag->setText(m_meta.category);
        m_categoryTag->setVisible(true);
    } else {
        m_categoryTag->setVisible(false);
    }

    // Hub badge text
    const HubSource src = HubManager::instance().hubById(m_meta.hub);
    m_hubBadge->setText(src.displayName.isEmpty() ? m_meta.hub : src.displayName);

    // Hub badge color (use source iconColor or fallback)
    const QColor hubColor = src.iconColor.isValid() ? src.iconColor : QColor("#4fc3f7");
    m_hubBadge->setStyleSheet(
        QStringLiteral("QLabel {"
                        "  background: %1;"
                        "  color: #07090c;"
                        "  font-size: 10px;"
                        "  font-weight: 700;"
                        "  border-radius: 4px;"
                        "  padding: 1px 6px;"
                        "}").arg(hubColor.name()));

    // Elided description
    {
        const QFontMetrics fm(m_descLabel->font());
        const QString elided = fm.elidedText(m_meta.description, Qt::ElideRight, 560);
        m_descLabel->setText(elided);
        m_descLabel->setToolTip(m_meta.description);
    }

    // --- Signals -----------------------------------------------
    connect(m_favoriteBtn,   &QPushButton::clicked, this, [this]() {
        m_meta.favorited = !m_meta.favorited;
        HubManager::instance().setFavorite(m_meta.id, m_meta.favorited);
        updateFavoriteButton();
        emit favoriteToggled(m_meta.id, m_meta.favorited);
    });

    connect(m_openEditorBtn, &QPushButton::clicked, this, [this]() {
        emit openInEditor(m_meta);
    });

    connect(m_inspectBtn, &QPushButton::clicked, this, [this]() {
        emit inspect(m_meta);
    });

    connect(m_executeBtn, &QPushButton::clicked, this, [this]() {
        emit executeDirect(m_meta);
    });
}

// ============================================================
//  applyTheme – base stylesheet (borders drawn in paintEvent)
// ============================================================
void ScriptCard::applyTheme()
{
    setStyleSheet(QStringLiteral(R"(
        #ScriptCard {
            background: #0d1117;
            border-radius: 8px;
        }
        #cardName {
            color: #e6edf3;
            font-size: 14px;
            font-weight: 700;
        }
        #cardDesc {
            color: #7d8590;
            font-size: 12px;
        }
        #cardAuthor {
            color: #7d8590;
            font-size: 11px;
        }
        #cardViews {
            color: #7d8590;
            font-size: 11px;
        }
        #categoryTag {
            background: #1c2433;
            color: #7d8590;
            font-size: 10px;
            font-weight: 600;
            border-radius: 3px;
            padding: 1px 6px;
        }
        #btnOpenEditor {
            background: transparent;
            color: #00ff66;
            border: 1px solid #00ff66;
            border-radius: 5px;
            font-size: 11px;
            font-weight: 600;
            padding: 0 10px;
        }
        #btnOpenEditor:hover {
            background: #00ff6622;
        }
        #btnOpenEditor:pressed {
            background: #00ff6644;
        }
        #btnInspect {
            background: transparent;
            color: #4fc3f7;
            border: 1px solid #4fc3f7;
            border-radius: 5px;
            font-size: 11px;
            font-weight: 600;
            padding: 0 10px;
        }
        #btnInspect:hover {
            background: #4fc3f722;
        }
        #btnInspect:pressed {
            background: #4fc3f744;
        }
        #btnExecute {
            background: #00ff66;
            color: #07090c;
            border: none;
            border-radius: 5px;
            font-size: 11px;
            font-weight: 700;
            padding: 0 10px;
        }
        #btnExecute:hover {
            background: #00e65c;
        }
        #btnExecute:pressed {
            background: #00cc52;
        }
        #favoriteBtn {
            background: transparent;
            border: none;
            color: #7d8590;
            font-size: 16px;
        }
        #favoriteBtn:hover {
            color: #f0a500;
        }
    )"));
}

// ============================================================
//  updateFavoriteButton
// ============================================================
void ScriptCard::updateFavoriteButton()
{
    m_favoriteBtn->setText(m_meta.favorited
        ? QStringLiteral("\u2605")   // filled star
        : QStringLiteral("\u2606")); // hollow star
    m_favoriteBtn->setStyleSheet(
        m_meta.favorited
            ? QStringLiteral("#favoriteBtn { background:transparent; border:none; color:#f0a500; font-size:16px; }")
            : QStringLiteral("#favoriteBtn { background:transparent; border:none; color:#7d8590; font-size:16px; }")
    );
}

// ============================================================
//  updateSafetyBadge
// ============================================================
void ScriptCard::updateSafetyBadge()
{
    const QColor  col  = safetyColor(m_meta.safetyVerdict);
    const QString icon = verdictIcon(m_meta.safetyVerdict);
    const QString lbl  = safetyLabel(m_meta.safetyVerdict);

    m_safetyBadge->setText(icon + QLatin1Char(' ') + lbl);
    m_safetyBadge->setToolTip(QStringLiteral("Safety: ") + lbl);
    m_safetyBadge->setStyleSheet(
        QStringLiteral("QLabel {"
                        "  background: %1;"
                        "  color: %2;"
                        "  font-size: 10px;"
                        "  font-weight: 700;"
                        "  border-radius: 4px;"
                        "  padding: 1px 6px;"
                        "}")
            .arg(col.darker(300).name())
            .arg(col.name()));
}

// ============================================================
//  Safety helpers
// ============================================================
QString ScriptCard::safetyLabel(SafetyVerdict v)
{
    switch (v) {
        case SafetyVerdict::Clean:      return QStringLiteral("CLEAN");
        case SafetyVerdict::Caution:    return QStringLiteral("CAUTION");
        case SafetyVerdict::Danger:     return QStringLiteral("DANGER");
        case SafetyVerdict::Obfuscated: return QStringLiteral("OBFUSCATED");
    }
    return QStringLiteral("CLEAN");
}

QColor ScriptCard::safetyColor(SafetyVerdict v)
{
    switch (v) {
        case SafetyVerdict::Clean:      return QColor("#00ff66");
        case SafetyVerdict::Caution:    return QColor("#f0a500");
        case SafetyVerdict::Danger:     return QColor("#ff4444");
        case SafetyVerdict::Obfuscated: return QColor("#ce93d8");
    }
    return QColor("#00ff66");
}

QString ScriptCard::verdictIcon(SafetyVerdict v)
{
    switch (v) {
        case SafetyVerdict::Clean:      return QStringLiteral("\u2714"); // checkmark
        case SafetyVerdict::Caution:    return QStringLiteral("\u26a0"); // warning
        case SafetyVerdict::Danger:     return QStringLiteral("\u2620"); // skull
        case SafetyVerdict::Obfuscated: return QStringLiteral("\u26d4"); // no-entry
    }
    return QStringLiteral("\u2714");
}

// ============================================================
//  meta accessors
// ============================================================
const ScriptMeta& ScriptCard::meta() const { return m_meta; }

void ScriptCard::updateMeta(const ScriptMeta& meta)
{
    m_meta = meta;
    m_nameLabel->setText(m_meta.name);

    const QFontMetrics fm(m_descLabel->font());
    m_descLabel->setText(fm.elidedText(m_meta.description, Qt::ElideRight, 560));
    m_descLabel->setToolTip(m_meta.description);

    m_authorLabel->setText(
        m_meta.author.isEmpty()
            ? QStringLiteral("Unknown")
            : QStringLiteral("\u270e ") + m_meta.author);

    m_viewsLabel->setText(
        m_meta.views > 0
            ? QStringLiteral("\U0001f441 ") + QString::number(m_meta.views)
            : QString());

    updateSafetyBadge();
    updateFavoriteButton();
    update();
}

// ============================================================
//  borderColor property
// ============================================================
QColor ScriptCard::borderColor() const { return m_borderColor; }

void ScriptCard::setBorderColor(const QColor& color)
{
    if (m_borderColor == color) return;
    m_borderColor = color;
    update();
}

// ============================================================
//  Hover events
// ============================================================
void ScriptCard::enterEvent(QEnterEvent* event)
{
    QFrame::enterEvent(event);
    startHoverAnimation(true);
}

void ScriptCard::leaveEvent(QEvent* event)
{
    QFrame::leaveEvent(event);
    startHoverAnimation(false);
}

void ScriptCard::startHoverAnimation(bool hovering)
{
    if (m_borderAnim->state() == QAbstractAnimation::Running)
        m_borderAnim->stop();

    m_borderAnim->setStartValue(m_borderColor);
    m_borderAnim->setEndValue(hovering ? QColor("#00ff66") : QColor("#1e2530"));
    m_borderAnim->start();

    // Adjust shadow on hover
    if (m_shadow) {
        m_shadow->setBlurRadius(hovering ? 22 : 10);
        m_shadow->setColor(hovering ? QColor(0, 255, 102, 60) : QColor(0, 0, 0, 80));
    }
}

// ============================================================
//  paintEvent - draw rounded border with animated color
// ============================================================
void ScriptCard::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF r = rect().adjusted(1, 1, -1, -1);

    // Background fill
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#0d1117"));
    p.drawRoundedRect(r, 8, 8);

    // Animated border
    QPen pen(m_borderColor, 1.4);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(r, 8, 8);
}

} // namespace Voltro

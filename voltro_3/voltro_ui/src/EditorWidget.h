#pragma once
// ============================================================
//  EditorWidget.h  –  Voltro Executor  (Phase 2)
//  Dark Lua code editor with syntax highlighting,
//  line numbers, auto-indent and current-line highlight.
// ============================================================

#ifndef VOLTRO_EDITORWIDGET_H
#define VOLTRO_EDITORWIDGET_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>

class QPaintEvent;
class QResizeEvent;
class QSize;

namespace Voltro {

class LuaHighlighter;
class LineNumberArea;

class CodePlainTextEdit : public QPlainTextEdit
{
public:
    using QPlainTextEdit::QPlainTextEdit;
    using QPlainTextEdit::firstVisibleBlock;
    using QPlainTextEdit::blockBoundingGeometry;
    using QPlainTextEdit::blockBoundingRect;
    using QPlainTextEdit::contentOffset;
    using QPlainTextEdit::setViewportMargins;
};

// ============================================================
//  EditorWidget
//  A feature-rich Lua source code editor.
// ============================================================
class EditorWidget : public QWidget
{
    Q_OBJECT

    // ── Qt properties (accessible from stylesheets / tools) ──
    Q_PROPERTY(QString fontFamily   READ fontFamily   WRITE setFontFamily)
    Q_PROPERTY(int     fontSize     READ fontSize     WRITE setFontSize)
    Q_PROPERTY(bool    wordWrap     READ wordWrap     WRITE setWordWrap)
    Q_PROPERTY(bool    showLineNumbers READ showLineNumbers WRITE setShowLineNumbers)
    Q_PROPERTY(bool    autoIndent   READ autoIndent   WRITE setAutoIndent)
    Q_PROPERTY(int     tabSize      READ tabSize      WRITE setTabSize)

public:
    explicit EditorWidget(QWidget* parent = nullptr);
    ~EditorWidget() override = default;

    // ── Content API ──────────────────────────────────────────
    void    setCode(const QString& code);
    QString code() const;
    void    clear();

    // ── Search / Replace ─────────────────────────────────────
    bool findNext(const QString& text,
                  Qt::CaseSensitivity cs = Qt::CaseInsensitive,
                  bool wrapAround        = true);

    bool replace(const QString& find,
                 const QString& replacement,
                 Qt::CaseSensitivity cs = Qt::CaseInsensitive);

    int  replaceAll(const QString& find,
                    const QString& replacement,
                    Qt::CaseSensitivity cs = Qt::CaseInsensitive);

    // ── Modification state ───────────────────────────────────
    bool isModified() const;
    void setModified(bool modified);

    // ── Property accessors ───────────────────────────────────
    QString fontFamily()    const;
    int     fontSize()      const;
    bool    wordWrap()      const;
    bool    showLineNumbers()const;
    bool    autoIndent()    const;
    int     tabSize()       const;

    void setFontFamily(const QString& family);
    void setFontSize(int pointSize);
    void setWordWrap(bool enabled);
    void setShowLineNumbers(bool enabled);
    void setAutoIndent(bool enabled);
    void setTabSize(int spaces);

    // ── Undo / Redo passthrough ───────────────────────────────
    bool canUndo() const;
    bool canRedo() const;

    // ── Access to the inner editor (for advanced use) ─────────
    QPlainTextEdit* editor() const { return m_editor; }

signals:
    void contentChanged();
    void cursorPositionChanged(int line, int col);
    void modificationChanged(bool modified);

public slots:
    void undo();
    void redo();
    void selectAll();
    void copyToClipboard();
    void cutToClipboard();
    void pasteFromClipboard();

    // Re-apply theme colours when ThemeManager emits themeChanged
    void refreshTheme();

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onCursorPositionChanged();
    void onTextChanged();
    void highlightCurrentLine();
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect& rect, int dy);

private:
    void setupEditor();
    void setupHighlighter();
    void applyEditorFont();
    void applyEditorColors();
    int  lineNumberAreaWidth() const;

    // Key-press filter for auto-indent / tab expansion
    bool eventFilter(QObject* obj, QEvent* event) override;

    // ── Child widgets ─────────────────────────────────────────
    CodePlainTextEdit* m_editor          {nullptr};
    LineNumberArea* m_lineNumberArea  {nullptr};
    LuaHighlighter* m_highlighter     {nullptr};

    // ── Properties ───────────────────────────────────────────
    QString m_fontFamily   {"JetBrains Mono"};
    int     m_fontSize     {13};
    bool    m_wordWrap     {false};
    bool    m_showLineNums {true};
    bool    m_autoIndent   {true};
    int     m_tabSize      {4};

    friend class LineNumberArea;
};


// ============================================================
//  LineNumberArea  –  painted inside the left gutter
// ============================================================
class LineNumberArea : public QWidget
{
    Q_OBJECT
public:
    explicit LineNumberArea(EditorWidget* editor)
        : QWidget(editor), m_editorWidget(editor) {}

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    EditorWidget* m_editorWidget;
};


// ============================================================
//  LuaHighlighter  –  QSyntaxHighlighter for Lua / Roblox API
// ============================================================
class LuaHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit LuaHighlighter(QTextDocument* parent = nullptr);

    /// Update palette colours from the current ThemeManager theme.
    void refreshColors();

protected:
    void highlightBlock(const QString& text) override;

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat    format;
    };

    void buildRules();

    QVector<HighlightRule> m_rules;

    // Long-bracket (multi-line) comment / string state
    QRegularExpression m_longBracketOpen;
    QRegularExpression m_longBracketClose;

    // Formats
    QTextCharFormat m_keywordFmt;
    QTextCharFormat m_builtinFmt;
    QTextCharFormat m_robloxFmt;
    QTextCharFormat m_execGlobalFmt;
    QTextCharFormat m_stringFmt;
    QTextCharFormat m_numberFmt;
    QTextCharFormat m_commentFmt;
    QTextCharFormat m_operatorFmt;
    QTextCharFormat m_selfFmt;
};

} // namespace Voltro

#endif // VOLTRO_EDITORWIDGET_H



class QPaintEvent;
class QResizeEvent;
class QSize;

namespace Thakur {

class LuaHighlighter;
class LineNumberArea;

class EditorWidget : public QWidget
{
    Q_OBJECT

    
    Q_PROPERTY(QString fontFamily   READ fontFamily   WRITE setFontFamily)
    Q_PROPERTY(int     fontSize     READ fontSize     WRITE setFontSize)
    Q_PROPERTY(bool    wordWrap     READ wordWrap     WRITE setWordWrap)
    Q_PROPERTY(bool    showLineNumbers READ showLineNumbers WRITE setShowLineNumbers)
    Q_PROPERTY(bool    autoIndent   READ autoIndent   WRITE setAutoIndent)
    Q_PROPERTY(int     tabSize      READ tabSize      WRITE setTabSize)

public:
    explicit EditorWidget(QWidget* parent = nullptr);
    ~EditorWidget() override = default;

    
    void    setCode(const QString& code);
    QString code() const;
    void    clear();

    
    bool findNext(const QString& text,
                  Qt::CaseSensitivity cs = Qt::CaseInsensitive,
                  bool wrapAround        = true);

    bool replace(const QString& find,
                 const QString& replacement,
                 Qt::CaseSensitivity cs = Qt::CaseInsensitive);

    int  replaceAll(const QString& find,
                    const QString& replacement,
                    Qt::CaseSensitivity cs = Qt::CaseInsensitive);

    
    bool isModified() const;
    void setModified(bool modified);

    
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

    
    bool canUndo() const;
    bool canRedo() const;

    
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

    
    bool eventFilter(QObject* obj, QEvent* event) override;

    
    QPlainTextEdit* m_editor          {nullptr};
    LineNumberArea* m_lineNumberArea  {nullptr};
    LuaHighlighter* m_highlighter     {nullptr};

    
    QString m_fontFamily   {"JetBrains Mono"};
    int     m_fontSize     {13};
    bool    m_wordWrap     {false};
    bool    m_showLineNums {true};
    bool    m_autoIndent   {true};
    int     m_tabSize      {4};

    friend class LineNumberArea;
};

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

class LuaHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit LuaHighlighter(QTextDocument* parent = nullptr);

    
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

    
    QRegularExpression m_longBracketOpen;
    QRegularExpression m_longBracketClose;

    
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

} 


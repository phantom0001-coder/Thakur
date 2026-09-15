

namespace Thakur {

LuaHighlighter::LuaHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    refreshColors();
    buildRules();
}

void LuaHighlighter::refreshColors()
{
    auto& tm = ThemeManager::instance();

    
    m_keywordFmt.setForeground(tm.info());
    m_keywordFmt.setFontWeight(QFont::Bold);

    
    m_builtinFmt.setForeground(QColor("
    m_builtinFmt.setFontItalic(false);

    
    m_robloxFmt.setForeground(QColor("

    
    m_execGlobalFmt.setForeground(QColor("

    
    m_stringFmt.setForeground(tm.accent());

    
    m_numberFmt.setForeground(QColor("

    
    m_commentFmt.setForeground(tm.muted());
    m_commentFmt.setFontItalic(true);

    
    m_operatorFmt.setForeground(QColor("

    
    m_selfFmt.setForeground(tm.text());
    m_selfFmt.setFontItalic(true);

    rehighlight();
}

void LuaHighlighter::buildRules()
{
    m_rules.clear();

    
    const QStringList keywords = {
        "and", "break", "do", "else", "elseif", "end",
        "false", "for", "function", "if", "in", "local",
        "nil", "not", "or", "repeat", "return", "then",
        "true", "until", "while"
    };
    for (const QString& kw : keywords) {
        HighlightRule r;
        r.pattern = QRegularExpression(QStringLiteral("\\b") + kw + QStringLiteral("\\b"));
        r.format  = m_keywordFmt;
        m_rules.append(r);
    }

    
    const QStringList builtins = {
        "print", "warn", "error", "tostring", "tonumber",
        "pairs", "ipairs", "next", "type", "select",
        "unpack", "rawget", "rawset", "rawequal", "rawlen",
        "setmetatable", "getmetatable", "pcall", "xpcall",
        "coroutine", "table", "string", "math", "io", "os",
        "require", "load", "loadstring", "dofile", "loadfile",
        "assert", "collectgarbage"
    };
    for (const QString& b : builtins) {
        HighlightRule r;
        r.pattern = QRegularExpression(QStringLiteral("\\b") + b + QStringLiteral("\\b"));
        r.format  = m_builtinFmt;
        m_rules.append(r);
    }

    
    const QStringList robloxApi = {
        "game", "workspace", "script", "plugin",
        "Players", "RunService", "UserInputService",
        "TweenService", "Lighting", "ReplicatedStorage",
        "ServerStorage", "StarterGui", "StarterPack",
        "SoundService", "PhysicsService", "CollectionService",
        "ContentProvider", "DataStoreService", "HttpService",
        "MarketplaceService", "Teams", "TextService",
        "VirtualInputManager", "GuiService"
    };
    for (const QString& api : robloxApi) {
        HighlightRule r;
        r.pattern = QRegularExpression(QStringLiteral("\\b") + api + QStringLiteral("\\b"));
        r.format  = m_robloxFmt;
        m_rules.append(r);
    }

    
    const QStringList execGlobals = {
        "identifyexecutor", "getexecutorname",
        "checkcaller", "islclosure", "iscclosure",
        "getgenv", "getrenv", "getfenv", "setfenv",
        "getrawmetatable", "setrawmetatable",
        "hookfunction", "newcclosure", "newlclosure",
        "clonefunction", "cloneref",
        "getsenv", "getscripts", "getloadedmodules",
        "getthreadidentity", "setthreadidentity",
        "readfile", "writefile", "appendfile", "listfiles",
        "isfile", "isfolder", "makefolder", "delfolder",
        "delfile", "loadstring", "syn", "rconsoleprint",
        "rconsolewarn", "rconsoleerr", "rconsolename",
        "firetouchinterest", "fireproximityprompt",
        "fireclickdetector", "mouse1click", "mouse2click",
        "keypress", "keyrelease", "mousemoverel", "mousemoveabs"
    };
    for (const QString& g : execGlobals) {
        HighlightRule r;
        r.pattern = QRegularExpression(QStringLiteral("\\b") + g + QStringLiteral("\\b"));
        r.format  = m_execGlobalFmt;
        m_rules.append(r);
    }

    
    {
        HighlightRule r;
        r.pattern = QRegularExpression(R"(\bself\b|\.\.\.)");
        r.format  = m_selfFmt;
        m_rules.append(r);
    }

    
    {
        HighlightRule r;
        r.pattern = QRegularExpression(R"([+\-*/%^
        r.format  = m_operatorFmt;
        m_rules.append(r);
    }

    
    {
        HighlightRule r;
        r.pattern = QRegularExpression(
            R"(\b(0[xX][0-9a-fA-F]+|\d+(\.\d+)?([eE][+-]?\d+)?)\b)");
        r.format  = m_numberFmt;
        m_rules.append(r);
    }

    
    {
        HighlightRule r;
        r.pattern = QRegularExpression(R"("(?:[^"\\]|\\.)*")");
        r.format  = m_stringFmt;
        m_rules.append(r);
    }

    
    {
        HighlightRule r;
        r.pattern = QRegularExpression(R"('(?:[^'\\]|\\.)*')");
        r.format  = m_stringFmt;
        m_rules.append(r);
    }

    
    {
        HighlightRule r;
        r.pattern = QRegularExpression(R"(--(?!\[=*\[).*$)");
        r.format  = m_commentFmt;
        m_rules.append(r);
    }

    
    m_longBracketOpen  = QRegularExpression(R"(--\[=*\[|\[=*\[)");
    m_longBracketClose = QRegularExpression(R"(\]=*\])");
}

void LuaHighlighter::highlightBlock(const QString& text)
{
    
    
    
    
    
    
    
    
    

    enum BlockState { Normal = 0, InLongComment = -1, InLongString = -2 };

    int prevState = previousBlockState();

    if (prevState == InLongComment || prevState == InLongString) {
        
        QTextCharFormat& fmt = (prevState == InLongComment) ? m_commentFmt : m_stringFmt;
        auto m = m_longBracketClose.match(text);
        if (m.hasMatch()) {
            setFormat(0, m.capturedEnd(), fmt);
            setCurrentBlockState(Normal);
            
            
            int startPos = m.capturedEnd();
            
            for (const HighlightRule& rule : std::as_const(m_rules)) {
                auto it = rule.pattern.globalMatch(text, startPos);
                while (it.hasNext()) {
                    auto rm = it.next();
                    setFormat(rm.capturedStart(), rm.capturedLength(), rule.format);
                }
            }
        } else {
            setFormat(0, text.length(), fmt);
            setCurrentBlockState(prevState);
        }
        return;
    }

    
    setCurrentBlockState(Normal);

    for (const HighlightRule& rule : std::as_const(m_rules)) {
        auto it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            auto m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), rule.format);
        }
    }

    
    auto it = m_longBracketOpen.globalMatch(text);
    while (it.hasNext()) {
        auto m = it.next();
        bool isComment = text.mid(m.capturedStart(), 2) == "--";
        QTextCharFormat& fmt = isComment ? m_commentFmt : m_stringFmt;
        int startPos = m.capturedStart();

        
        auto closeIt = m_longBracketClose.match(text, m.capturedEnd());
        if (closeIt.hasMatch()) {
            
            setFormat(startPos, closeIt.capturedEnd() - startPos, fmt);
        } else {
            
            setFormat(startPos, text.length() - startPos, fmt);
            setCurrentBlockState(isComment ? InLongComment : InLongString);
            return; 
        }
    }
}

QSize LineNumberArea::sizeHint() const
{
    return {m_editorWidget->lineNumberAreaWidth(), 0};
}

void LineNumberArea::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    auto& tm = ThemeManager::instance();

    
    painter.fillRect(event->rect(), tm.background());

    
    painter.setPen(tm.surface());
    painter.drawLine(width() - 1, event->rect().top(),
                     width() - 1, event->rect().bottom());

    
    QPlainTextEdit* ed = m_editorWidget->m_editor;
    QTextBlock block   = ed->firstVisibleBlock();
    int blockNumber    = block.blockNumber();
    int top    = qRound(ed->blockBoundingGeometry(block).translated(ed->contentOffset()).top());
    int bottom = top + qRound(ed->blockBoundingRect(block).height());

    QFont f = ed->font();
    f.setPointSize(f.pointSize() - 1);
    painter.setFont(f);

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            
            if (blockNumber == ed->textCursor().blockNumber()) {
                painter.setPen(tm.accent());
                QFont bf = f;
                bf.setBold(true);
                painter.setFont(bf);
            } else {
                painter.setPen(tm.muted());
                painter.setFont(f);
            }
            painter.drawText(0, top, width() - 6, ed->fontMetrics().height(),
                             Qt::AlignRight, number);
        }
        block  = block.next();
        top    = bottom;
        bottom = top + qRound(ed->blockBoundingRect(block).height());
        ++blockNumber;
    }
}

EditorWidget::EditorWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setLayout(layout);

    m_editor         = new QPlainTextEdit(this);
    m_lineNumberArea = new LineNumberArea(this);
    m_highlighter    = new LuaHighlighter(m_editor->document());

    layout->addWidget(m_lineNumberArea);
    layout->addWidget(m_editor);

    setupEditor();

    
    connect(m_editor, &QPlainTextEdit::blockCountChanged,
            this, &EditorWidget::updateLineNumberAreaWidth);
    connect(m_editor, &QPlainTextEdit::updateRequest,
            this, &EditorWidget::updateLineNumberArea);
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged,
            this, &EditorWidget::onCursorPositionChanged);
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged,
            this, &EditorWidget::highlightCurrentLine);
    connect(m_editor, &QPlainTextEdit::textChanged,
            this, &EditorWidget::onTextChanged);

    
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &EditorWidget::refreshTheme);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    
    m_editor->installEventFilter(this);
}

void EditorWidget::setupEditor()
{
    m_editor->setLineWrapMode(m_wordWrap
        ? QPlainTextEdit::WidgetWidth
        : QPlainTextEdit::NoWrap);
    m_editor->setTabStopDistance(
        QFontMetricsF(m_editor->font()).horizontalAdvance(' ') * m_tabSize);

    applyEditorFont();
    applyEditorColors();
}

void EditorWidget::applyEditorFont()
{
    
    QStringList families = {"JetBrains Mono", "Cascadia Code", "Consolas",
                            "Fira Code", "Courier New"};
    QFont font;
    bool found = false;
    for (const QString& fam : families) {
        font = QFont(fam, m_fontSize);
        if (QFontDatabase().hasFamily(fam)) {
            m_fontFamily = fam;
            found = true;
            break;
        }
    }
    if (!found) {
        font = QFont();
        font.setStyleHint(QFont::Monospace);
        font.setFixedPitch(true);
        font.setPointSize(m_fontSize);
    }

    font.setFixedPitch(true);
    m_editor->setFont(font);
    m_lineNumberArea->setFont(font);

    
    m_editor->setTabStopDistance(
        QFontMetricsF(font).horizontalAdvance(' ') * m_tabSize);
}

void EditorWidget::applyEditorColors()
{
    auto& tm = ThemeManager::instance();

    
    QPalette p = m_editor->palette();
    p.setColor(QPalette::Base,            tm.background());
    p.setColor(QPalette::Text,            tm.text());
    p.setColor(QPalette::Window,          tm.background());
    p.setColor(QPalette::WindowText,      tm.text());
    p.setColor(QPalette::Highlight,       tm.accent());
    p.setColor(QPalette::HighlightedText, tm.background());
    m_editor->setPalette(p);

    
    QString css = QString(
        "QPlainTextEdit {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 0;"
        "  padding: 2px;"
        "  selection-background-color: %4;"
        "  selection-color: %5;"
        "}"
    ).arg(
        tm.background().name(),
        tm.text().name(),
        tm.surface().name(),
        
        [&]() { QColor c = tm.accent(); c.setAlpha(80); return c.name(QColor::HexArgb); }(),
        tm.text().name()
    );
    m_editor->setStyleSheet(css);

    
    QPalette lp = m_lineNumberArea->palette();
    lp.setColor(QPalette::Window, tm.background());
    m_lineNumberArea->setPalette(lp);
    m_lineNumberArea->setAutoFillBackground(true);
}

int EditorWidget::lineNumberAreaWidth() const
{
    if (!m_showLineNums) return 0;

    int digits = 1;
    int max = qMax(1, m_editor->blockCount());
    while (max >= 10) { max /= 10; ++digits; }

    int space = 8 + m_editor->fontMetrics().horizontalAdvance(QLatin1Char('9')) * (digits + 1);
    return space;
}

void EditorWidget::updateLineNumberAreaWidth(int )
{
    m_editor->setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void EditorWidget::updateLineNumberArea(const QRect& rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(m_editor->viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void EditorWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    QRect cr = m_editor->contentsRect();
    int w    = lineNumberAreaWidth();
    m_lineNumberArea->setGeometry(
        QRect(cr.left(), cr.top(), w, cr.height()));
    m_editor->setViewportMargins(w, 0, 0, 0);
}

void EditorWidget::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extras;

    if (!m_editor->isReadOnly()) {
        QTextEdit::ExtraSelection sel;
        QColor lineColor = ThemeManager::instance().surface();
        lineColor.setAlpha(80);
        sel.format.setBackground(lineColor);
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        sel.cursor = m_editor->textCursor();
        sel.cursor.clearSelection();
        extras.append(sel);
    }

    m_editor->setExtraSelections(extras);
    m_lineNumberArea->update();
}

bool EditorWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj != m_editor || event->type() != QEvent::KeyPress)
        return QWidget::eventFilter(obj, event);

    auto* ke = static_cast<QKeyEvent*>(event);

    
    if (ke->key() == Qt::Key_Tab && !(ke->modifiers() & Qt::ShiftModifier)) {
        QTextCursor cur = m_editor->textCursor();
        if (cur.hasSelection()) {
            
            cur.beginEditBlock();
            int start = cur.selectionStart();
            int end   = cur.selectionEnd();
            cur.setPosition(start);
            cur.movePosition(QTextCursor::StartOfBlock);
            while (cur.position() <= end && !cur.atEnd()) {
                cur.insertText(QString(m_tabSize, ' '));
                end += m_tabSize;
                if (!cur.movePosition(QTextCursor::NextBlock)) break;
            }
            cur.endEditBlock();
        } else {
            cur.insertText(QString(m_tabSize, ' '));
        }
        return true;
    }

    
    if (ke->key() == Qt::Key_Tab && (ke->modifiers() & Qt::ShiftModifier)) {
        QTextCursor cur = m_editor->textCursor();
        cur.beginEditBlock();
        int start = cur.selectionStart();
        int end   = cur.selectionEnd();
        cur.setPosition(start);
        cur.movePosition(QTextCursor::StartOfBlock);
        while (cur.position() <= end) {
            QString line = cur.block().text();
            int toRemove = 0;
            for (int i = 0; i < m_tabSize && i < line.size() && line[i] == ' '; ++i)
                ++toRemove;
            if (toRemove > 0) {
                cur.movePosition(QTextCursor::StartOfBlock);
                cur.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, toRemove);
                cur.removeSelectedText();
                end -= toRemove;
            }
            if (!cur.movePosition(QTextCursor::NextBlock)) break;
        }
        cur.endEditBlock();
        return true;
    }

    
    if (m_autoIndent &&
        (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter))
    {
        QTextCursor cur = m_editor->textCursor();
        const QString blockText = cur.block().text();

        
        int indentLen = 0;
        for (QChar c : blockText) {
            if (c == ' ')  ++indentLen;
            else if (c == '\t') indentLen += m_tabSize;
            else break;
        }

        
        const QString trimmed = blockText.trimmed();
        bool addLevel = trimmed.endsWith("then")   ||
                        trimmed.endsWith("do")      ||
                        trimmed.endsWith("else")    ||
                        trimmed.endsWith("elseif")  ||
                        (trimmed.endsWith(")") && trimmed.startsWith("function")) ||
                        trimmed.endsWith("{");

        cur.beginEditBlock();
        cur.insertBlock();
        cur.insertText(QString(indentLen + (addLevel ? m_tabSize : 0), ' '));
        cur.endEditBlock();
        m_editor->setTextCursor(cur);
        return true;
    }

    
    if (ke->key() == Qt::Key_Backspace && m_autoIndent) {
        QTextCursor cur = m_editor->textCursor();
        if (!cur.hasSelection()) {
            const QString blockText = cur.block().text();
            int col = cur.positionInBlock();
            
            bool onlySpaces = true;
            for (int i = 0; i < col; ++i) {
                if (blockText[i] != ' ') { onlySpaces = false; break; }
            }
            if (onlySpaces && col > 0) {
                int toRemove = col % m_tabSize;
                if (toRemove == 0) toRemove = m_tabSize;
                cur.beginEditBlock();
                for (int i = 0; i < toRemove; ++i)
                    cur.deletePreviousChar();
                cur.endEditBlock();
                return true;
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}

void EditorWidget::setCode(const QString& code)
{
    m_editor->setPlainText(code);
    m_editor->document()->setModified(false);
}

QString EditorWidget::code() const
{
    return m_editor->toPlainText();
}

void EditorWidget::clear()
{
    m_editor->clear();
    m_editor->document()->setModified(false);
}

bool EditorWidget::isModified() const
{
    return m_editor->document()->isModified();
}

void EditorWidget::setModified(bool modified)
{
    m_editor->document()->setModified(modified);
}

bool EditorWidget::findNext(const QString& text, Qt::CaseSensitivity cs, bool wrapAround)
{
    if (text.isEmpty()) return false;

    QTextDocument::FindFlags flags;
    if (cs == Qt::CaseSensitive)
        flags |= QTextDocument::FindCaseSensitively;

    bool found = m_editor->find(text, flags);
    if (!found && wrapAround) {
        
        QTextCursor cur = m_editor->textCursor();
        cur.movePosition(QTextCursor::Start);
        m_editor->setTextCursor(cur);
        found = m_editor->find(text, flags);
    }
    return found;
}

bool EditorWidget::replace(const QString& find, const QString& replacement,
                           Qt::CaseSensitivity cs)
{
    if (find.isEmpty()) return false;

    QTextCursor cur = m_editor->textCursor();
    QString sel     = cur.selectedText();
    bool match      = (cs == Qt::CaseInsensitive)
                        ? sel.compare(find, Qt::CaseInsensitive) == 0
                        : sel == find;

    if (match) {
        cur.insertText(replacement);
        return true;
    }
    
    return findNext(find, cs);
}

int EditorWidget::replaceAll(const QString& find, const QString& replacement,
                             Qt::CaseSensitivity cs)
{
    if (find.isEmpty()) return 0;

    QTextDocument::FindFlags flags;
    if (cs == Qt::CaseSensitive)
        flags |= QTextDocument::FindCaseSensitively;

    int count = 0;
    QTextCursor cur = m_editor->textCursor();
    cur.beginEditBlock();
    cur.movePosition(QTextCursor::Start);
    m_editor->setTextCursor(cur);

    while (m_editor->find(find, flags)) {
        m_editor->textCursor().insertText(replacement);
        ++count;
    }
    cur.endEditBlock();
    return count;
}

QString EditorWidget::fontFamily()     const { return m_fontFamily; }
int     EditorWidget::fontSize()       const { return m_fontSize;   }
bool    EditorWidget::wordWrap()       const { return m_wordWrap;   }
bool    EditorWidget::showLineNumbers()const { return m_showLineNums; }
bool    EditorWidget::autoIndent()     const { return m_autoIndent; }
int     EditorWidget::tabSize()        const { return m_tabSize;    }

void EditorWidget::setFontFamily(const QString& family)
{
    m_fontFamily = family;
    applyEditorFont();
}

void EditorWidget::setFontSize(int pointSize)
{
    m_fontSize = qBound(6, pointSize, 72);
    applyEditorFont();
}

void EditorWidget::setWordWrap(bool enabled)
{
    m_wordWrap = enabled;
    m_editor->setLineWrapMode(enabled
        ? QPlainTextEdit::WidgetWidth
        : QPlainTextEdit::NoWrap);
}

void EditorWidget::setShowLineNumbers(bool enabled)
{
    m_showLineNums = enabled;
    m_lineNumberArea->setVisible(enabled);
    updateLineNumberAreaWidth(0);
}

void EditorWidget::setAutoIndent(bool enabled)
{
    m_autoIndent = enabled;
}

void EditorWidget::setTabSize(int spaces)
{
    m_tabSize = qBound(1, spaces, 16);
    m_editor->setTabStopDistance(
        QFontMetricsF(m_editor->font()).horizontalAdvance(' ') * m_tabSize);
}

bool EditorWidget::canUndo() const { return m_editor->document()->isUndoAvailable(); }
bool EditorWidget::canRedo() const { return m_editor->document()->isRedoAvailable(); }

void EditorWidget::undo()               { m_editor->undo(); }
void EditorWidget::redo()               { m_editor->redo(); }
void EditorWidget::selectAll()          { m_editor->selectAll(); }
void EditorWidget::copyToClipboard()    { m_editor->copy(); }
void EditorWidget::cutToClipboard()     { m_editor->cut(); }
void EditorWidget::pasteFromClipboard() { m_editor->paste(); }

void EditorWidget::refreshTheme()
{
    applyEditorColors();
    m_highlighter->refreshColors();
    highlightCurrentLine();
    m_lineNumberArea->update();
}

void EditorWidget::onCursorPositionChanged()
{
    QTextCursor cur = m_editor->textCursor();
    int line = cur.blockNumber() + 1;
    int col  = cur.positionInBlock() + 1;
    emit cursorPositionChanged(line, col);
}

void EditorWidget::onTextChanged()
{
    emit contentChanged();
    emit modificationChanged(m_editor->document()->isModified());
}

} 

/****************************************************************************
** Meta object code from reading C++ file 'EditorWidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../voltro_ui/src/EditorWidget.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EditorWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_Voltro__EditorWidget_t {
    uint offsetsAndSizes[58];
    char stringdata0[21];
    char stringdata1[15];
    char stringdata2[1];
    char stringdata3[22];
    char stringdata4[5];
    char stringdata5[4];
    char stringdata6[20];
    char stringdata7[9];
    char stringdata8[5];
    char stringdata9[5];
    char stringdata10[10];
    char stringdata11[16];
    char stringdata12[15];
    char stringdata13[19];
    char stringdata14[13];
    char stringdata15[24];
    char stringdata16[14];
    char stringdata17[21];
    char stringdata18[26];
    char stringdata19[14];
    char stringdata20[21];
    char stringdata21[5];
    char stringdata22[3];
    char stringdata23[11];
    char stringdata24[9];
    char stringdata25[9];
    char stringdata26[16];
    char stringdata27[11];
    char stringdata28[8];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_Voltro__EditorWidget_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_Voltro__EditorWidget_t qt_meta_stringdata_Voltro__EditorWidget = {
    {
        QT_MOC_LITERAL(0, 20),  // "Voltro::EditorWidget"
        QT_MOC_LITERAL(21, 14),  // "contentChanged"
        QT_MOC_LITERAL(36, 0),  // ""
        QT_MOC_LITERAL(37, 21),  // "cursorPositionChanged"
        QT_MOC_LITERAL(59, 4),  // "line"
        QT_MOC_LITERAL(64, 3),  // "col"
        QT_MOC_LITERAL(68, 19),  // "modificationChanged"
        QT_MOC_LITERAL(88, 8),  // "modified"
        QT_MOC_LITERAL(97, 4),  // "undo"
        QT_MOC_LITERAL(102, 4),  // "redo"
        QT_MOC_LITERAL(107, 9),  // "selectAll"
        QT_MOC_LITERAL(117, 15),  // "copyToClipboard"
        QT_MOC_LITERAL(133, 14),  // "cutToClipboard"
        QT_MOC_LITERAL(148, 18),  // "pasteFromClipboard"
        QT_MOC_LITERAL(167, 12),  // "refreshTheme"
        QT_MOC_LITERAL(180, 23),  // "onCursorPositionChanged"
        QT_MOC_LITERAL(204, 13),  // "onTextChanged"
        QT_MOC_LITERAL(218, 20),  // "highlightCurrentLine"
        QT_MOC_LITERAL(239, 25),  // "updateLineNumberAreaWidth"
        QT_MOC_LITERAL(265, 13),  // "newBlockCount"
        QT_MOC_LITERAL(279, 20),  // "updateLineNumberArea"
        QT_MOC_LITERAL(300, 4),  // "rect"
        QT_MOC_LITERAL(305, 2),  // "dy"
        QT_MOC_LITERAL(308, 10),  // "fontFamily"
        QT_MOC_LITERAL(319, 8),  // "fontSize"
        QT_MOC_LITERAL(328, 8),  // "wordWrap"
        QT_MOC_LITERAL(337, 15),  // "showLineNumbers"
        QT_MOC_LITERAL(353, 10),  // "autoIndent"
        QT_MOC_LITERAL(364, 7)   // "tabSize"
    },
    "Voltro::EditorWidget",
    "contentChanged",
    "",
    "cursorPositionChanged",
    "line",
    "col",
    "modificationChanged",
    "modified",
    "undo",
    "redo",
    "selectAll",
    "copyToClipboard",
    "cutToClipboard",
    "pasteFromClipboard",
    "refreshTheme",
    "onCursorPositionChanged",
    "onTextChanged",
    "highlightCurrentLine",
    "updateLineNumberAreaWidth",
    "newBlockCount",
    "updateLineNumberArea",
    "rect",
    "dy",
    "fontFamily",
    "fontSize",
    "wordWrap",
    "showLineNumbers",
    "autoIndent",
    "tabSize"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_Voltro__EditorWidget[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       6,  131, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  104,    2, 0x06,    7 /* Public */,
       3,    2,  105,    2, 0x06,    8 /* Public */,
       6,    1,  110,    2, 0x06,   11 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       8,    0,  113,    2, 0x0a,   13 /* Public */,
       9,    0,  114,    2, 0x0a,   14 /* Public */,
      10,    0,  115,    2, 0x0a,   15 /* Public */,
      11,    0,  116,    2, 0x0a,   16 /* Public */,
      12,    0,  117,    2, 0x0a,   17 /* Public */,
      13,    0,  118,    2, 0x0a,   18 /* Public */,
      14,    0,  119,    2, 0x0a,   19 /* Public */,
      15,    0,  120,    2, 0x08,   20 /* Private */,
      16,    0,  121,    2, 0x08,   21 /* Private */,
      17,    0,  122,    2, 0x08,   22 /* Private */,
      18,    1,  123,    2, 0x08,   23 /* Private */,
      20,    2,  126,    2, 0x08,   25 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    4,    5,
    QMetaType::Void, QMetaType::Bool,    7,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   19,
    QMetaType::Void, QMetaType::QRect, QMetaType::Int,   21,   22,

 // properties: name, type, flags
      23, QMetaType::QString, 0x00015103, uint(-1), 0,
      24, QMetaType::Int, 0x00015103, uint(-1), 0,
      25, QMetaType::Bool, 0x00015103, uint(-1), 0,
      26, QMetaType::Bool, 0x00015103, uint(-1), 0,
      27, QMetaType::Bool, 0x00015103, uint(-1), 0,
      28, QMetaType::Int, 0x00015103, uint(-1), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject Voltro::EditorWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_Voltro__EditorWidget.offsetsAndSizes,
    qt_meta_data_Voltro__EditorWidget,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_Voltro__EditorWidget_t,
        // property 'fontFamily'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'fontSize'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'wordWrap'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'showLineNumbers'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'autoIndent'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'tabSize'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<EditorWidget, std::true_type>,
        // method 'contentChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'cursorPositionChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'modificationChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'undo'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'redo'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'selectAll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'copyToClipboard'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'cutToClipboard'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'pasteFromClipboard'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refreshTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onCursorPositionChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onTextChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'highlightCurrentLine'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateLineNumberAreaWidth'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'updateLineNumberArea'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QRect &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>
    >,
    nullptr
} };

void Voltro::EditorWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EditorWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->contentChanged(); break;
        case 1: _t->cursorPositionChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 2: _t->modificationChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 3: _t->undo(); break;
        case 4: _t->redo(); break;
        case 5: _t->selectAll(); break;
        case 6: _t->copyToClipboard(); break;
        case 7: _t->cutToClipboard(); break;
        case 8: _t->pasteFromClipboard(); break;
        case 9: _t->refreshTheme(); break;
        case 10: _t->onCursorPositionChanged(); break;
        case 11: _t->onTextChanged(); break;
        case 12: _t->highlightCurrentLine(); break;
        case 13: _t->updateLineNumberAreaWidth((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 14: _t->updateLineNumberArea((*reinterpret_cast< std::add_pointer_t<QRect>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (EditorWidget::*)();
            if (_t _q_method = &EditorWidget::contentChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (EditorWidget::*)(int , int );
            if (_t _q_method = &EditorWidget::cursorPositionChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (EditorWidget::*)(bool );
            if (_t _q_method = &EditorWidget::modificationChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
    }else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<EditorWidget *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->fontFamily(); break;
        case 1: *reinterpret_cast< int*>(_v) = _t->fontSize(); break;
        case 2: *reinterpret_cast< bool*>(_v) = _t->wordWrap(); break;
        case 3: *reinterpret_cast< bool*>(_v) = _t->showLineNumbers(); break;
        case 4: *reinterpret_cast< bool*>(_v) = _t->autoIndent(); break;
        case 5: *reinterpret_cast< int*>(_v) = _t->tabSize(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<EditorWidget *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setFontFamily(*reinterpret_cast< QString*>(_v)); break;
        case 1: _t->setFontSize(*reinterpret_cast< int*>(_v)); break;
        case 2: _t->setWordWrap(*reinterpret_cast< bool*>(_v)); break;
        case 3: _t->setShowLineNumbers(*reinterpret_cast< bool*>(_v)); break;
        case 4: _t->setAutoIndent(*reinterpret_cast< bool*>(_v)); break;
        case 5: _t->setTabSize(*reinterpret_cast< int*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *Voltro::EditorWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Voltro::EditorWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Voltro__EditorWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int Voltro::EditorWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 15;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void Voltro::EditorWidget::contentChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void Voltro::EditorWidget::cursorPositionChanged(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void Voltro::EditorWidget::modificationChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
namespace {
struct qt_meta_stringdata_Voltro__LineNumberArea_t {
    uint offsetsAndSizes[2];
    char stringdata0[23];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_Voltro__LineNumberArea_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_Voltro__LineNumberArea_t qt_meta_stringdata_Voltro__LineNumberArea = {
    {
        QT_MOC_LITERAL(0, 22)   // "Voltro::LineNumberArea"
    },
    "Voltro::LineNumberArea"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_Voltro__LineNumberArea[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

Q_CONSTINIT const QMetaObject Voltro::LineNumberArea::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_Voltro__LineNumberArea.offsetsAndSizes,
    qt_meta_data_Voltro__LineNumberArea,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_Voltro__LineNumberArea_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<LineNumberArea, std::true_type>
    >,
    nullptr
} };

void Voltro::LineNumberArea::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *Voltro::LineNumberArea::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Voltro::LineNumberArea::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Voltro__LineNumberArea.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int Voltro::LineNumberArea::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    return _id;
}
namespace {
struct qt_meta_stringdata_Voltro__LuaHighlighter_t {
    uint offsetsAndSizes[2];
    char stringdata0[23];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_Voltro__LuaHighlighter_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_Voltro__LuaHighlighter_t qt_meta_stringdata_Voltro__LuaHighlighter = {
    {
        QT_MOC_LITERAL(0, 22)   // "Voltro::LuaHighlighter"
    },
    "Voltro::LuaHighlighter"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_Voltro__LuaHighlighter[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

Q_CONSTINIT const QMetaObject Voltro::LuaHighlighter::staticMetaObject = { {
    QMetaObject::SuperData::link<QSyntaxHighlighter::staticMetaObject>(),
    qt_meta_stringdata_Voltro__LuaHighlighter.offsetsAndSizes,
    qt_meta_data_Voltro__LuaHighlighter,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_Voltro__LuaHighlighter_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<LuaHighlighter, std::true_type>
    >,
    nullptr
} };

void Voltro::LuaHighlighter::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *Voltro::LuaHighlighter::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Voltro::LuaHighlighter::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Voltro__LuaHighlighter.stringdata0))
        return static_cast<void*>(this);
    return QSyntaxHighlighter::qt_metacast(_clname);
}

int Voltro::LuaHighlighter::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QSyntaxHighlighter::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE

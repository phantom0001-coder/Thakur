/****************************************************************************
** Meta object code from reading C++ file 'Updater.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../voltro_ui/src/updater/Updater.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Updater.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_Voltro__Updater_t {
    uint offsetsAndSizes[28];
    char stringdata0[16];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[20];
    char stringdata4[8];
    char stringdata5[18];
    char stringdata6[17];
    char stringdata7[9];
    char stringdata8[6];
    char stringdata9[17];
    char stringdata10[10];
    char stringdata11[14];
    char stringdata12[12];
    char stringdata13[6];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_Voltro__Updater_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_Voltro__Updater_t qt_meta_stringdata_Voltro__Updater = {
    {
        QT_MOC_LITERAL(0, 15),  // "Voltro::Updater"
        QT_MOC_LITERAL(16, 15),  // "updateAvailable"
        QT_MOC_LITERAL(32, 0),  // ""
        QT_MOC_LITERAL(33, 19),  // "Voltro::ReleaseInfo"
        QT_MOC_LITERAL(53, 7),  // "release"
        QT_MOC_LITERAL(61, 17),  // "noUpdateAvailable"
        QT_MOC_LITERAL(79, 16),  // "downloadProgress"
        QT_MOC_LITERAL(96, 8),  // "received"
        QT_MOC_LITERAL(105, 5),  // "total"
        QT_MOC_LITERAL(111, 16),  // "downloadComplete"
        QT_MOC_LITERAL(128, 9),  // "localPath"
        QT_MOC_LITERAL(138, 13),  // "updateApplied"
        QT_MOC_LITERAL(152, 11),  // "updateError"
        QT_MOC_LITERAL(164, 5)   // "error"
    },
    "Voltro::Updater",
    "updateAvailable",
    "",
    "Voltro::ReleaseInfo",
    "release",
    "noUpdateAvailable",
    "downloadProgress",
    "received",
    "total",
    "downloadComplete",
    "localPath",
    "updateApplied",
    "updateError",
    "error"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_Voltro__Updater[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   50,    2, 0x06,    1 /* Public */,
       5,    0,   53,    2, 0x06,    3 /* Public */,
       6,    2,   54,    2, 0x06,    4 /* Public */,
       9,    1,   59,    2, 0x06,    7 /* Public */,
      11,    0,   62,    2, 0x06,    9 /* Public */,
      12,    1,   63,    2, 0x06,   10 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,
    QMetaType::Void, QMetaType::LongLong, QMetaType::LongLong,    7,    8,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   13,

       0        // eod
};

Q_CONSTINIT const QMetaObject Voltro::Updater::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_Voltro__Updater.offsetsAndSizes,
    qt_meta_data_Voltro__Updater,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_Voltro__Updater_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<Updater, std::true_type>,
        // method 'updateAvailable'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const Voltro::ReleaseInfo &, std::false_type>,
        // method 'noUpdateAvailable'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'downloadProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'downloadComplete'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'updateApplied'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void Voltro::Updater::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Updater *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->updateAvailable((*reinterpret_cast< std::add_pointer_t<Voltro::ReleaseInfo>>(_a[1]))); break;
        case 1: _t->noUpdateAvailable(); break;
        case 2: _t->downloadProgress((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2]))); break;
        case 3: _t->downloadComplete((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->updateApplied(); break;
        case 5: _t->updateError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Voltro::ReleaseInfo >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Updater::*)(const Voltro::ReleaseInfo & );
            if (_t _q_method = &Updater::updateAvailable; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Updater::*)();
            if (_t _q_method = &Updater::noUpdateAvailable; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (Updater::*)(qint64 , qint64 );
            if (_t _q_method = &Updater::downloadProgress; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (Updater::*)(const QString & );
            if (_t _q_method = &Updater::downloadComplete; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (Updater::*)();
            if (_t _q_method = &Updater::updateApplied; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (Updater::*)(const QString & );
            if (_t _q_method = &Updater::updateError; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
    }
}

const QMetaObject *Voltro::Updater::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Voltro::Updater::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Voltro__Updater.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Voltro::Updater::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void Voltro::Updater::updateAvailable(const Voltro::ReleaseInfo & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Voltro::Updater::noUpdateAvailable()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void Voltro::Updater::downloadProgress(qint64 _t1, qint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void Voltro::Updater::downloadComplete(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void Voltro::Updater::updateApplied()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void Voltro::Updater::updateError(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE

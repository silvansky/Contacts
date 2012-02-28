/****************************************************************************
** Meta object code from reading C++ file 'vidgui.h'
**
** Created: Sat 24. Dec 00:25:32 2011
**      by: The Qt Meta Object Compiler version 62 (Qt 4.7.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../vidgui.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'vidgui.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.7.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MainWin[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: signature, parameters, type, tag, flags
      11,    9,    8,    8, 0x05,
      35,    8,    8,    8, 0x05,
      56,    8,    8,    8, 0x05,
      80,    8,    8,    8, 0x05,

 // slots: signature, parameters, type, tag, flags
     106,    8,    8,    8, 0x0a,
     116,    8,    8,    8, 0x0a,
     123,    8,    8,    8, 0x0a,
     132,    8,    8,    8, 0x0a,
     152,  139,    8,    8, 0x0a,
     172,    8,    8,    8, 0x0a,
     189,    8,    8,    8, 0x0a,
     211,  207,    8,    8, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_MainWin[] = {
    "MainWin\0\0,\0signalNewCall(int,bool)\0"
    "signalCallReleased()\0signalInitVideoWindow()\0"
    "signalShowStatus(QString)\0preview()\0"
    "call()\0hangup()\0quit()\0cid,incoming\0"
    "onNewCall(int,bool)\0onCallReleased()\0"
    "initVideoWindow()\0msg\0doShowStatus(QString)\0"
};

const QMetaObject MainWin::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_MainWin,
      qt_meta_data_MainWin, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MainWin::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MainWin::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MainWin::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MainWin))
        return static_cast<void*>(const_cast< MainWin*>(this));
    return QWidget::qt_metacast(_clname);
}

int MainWin::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: signalNewCall((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 1: signalCallReleased(); break;
        case 2: signalInitVideoWindow(); break;
        case 3: signalShowStatus((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: preview(); break;
        case 5: call(); break;
        case 6: hangup(); break;
        case 7: quit(); break;
        case 8: onNewCall((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 9: onCallReleased(); break;
        case 10: initVideoWindow(); break;
        case 11: doShowStatus((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void MainWin::signalNewCall(int _t1, bool _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void MainWin::signalCallReleased()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void MainWin::signalInitVideoWindow()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void MainWin::signalShowStatus(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_END_MOC_NAMESPACE

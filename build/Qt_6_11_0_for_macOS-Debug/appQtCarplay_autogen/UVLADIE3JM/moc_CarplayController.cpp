/****************************************************************************
** Meta object code from reading C++ file 'CarplayController.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/CarplayController.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CarplayController.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN17CarplayControllerE_t {};
} // unnamed namespace

template <> constexpr inline auto CarplayController::qt_create_metaobjectdata<qt_meta_tag_ZN17CarplayControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "CarplayController",
        "statusChanged",
        "",
        "dongleReadyChanged",
        "streamingChanged",
        "receivingVideoChanged",
        "statsChanged",
        "videoGeometryChanged",
        "audioActiveChanged",
        "audioStatsChanged",
        "videoSinkChanged",
        "verifyHardware",
        "startStream",
        "width",
        "height",
        "fps",
        "stop",
        "playCapture",
        "captureDirectory",
        "touchDown",
        "x",
        "y",
        "touchMove",
        "touchUp",
        "status",
        "dongleReady",
        "streaming",
        "receivingVideo",
        "frameCount",
        "decodedFps",
        "videoWidth",
        "videoHeight",
        "audioActive",
        "audioPacketCount",
        "videoSink",
        "QVideoSink*"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'statusChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'dongleReadyChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'streamingChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'receivingVideoChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'statsChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'videoGeometryChanged'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'audioActiveChanged'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'audioStatsChanged'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'videoSinkChanged'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'verifyHardware'
        QtMocHelpers::MethodData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'startStream'
        QtMocHelpers::MethodData<void(int, int, int)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 13 }, { QMetaType::Int, 14 }, { QMetaType::Int, 15 },
        }}),
        // Method 'stop'
        QtMocHelpers::MethodData<void()>(16, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'playCapture'
        QtMocHelpers::MethodData<void(const QString &, int)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 18 }, { QMetaType::Int, 15 },
        }}),
        // Method 'touchDown'
        QtMocHelpers::MethodData<void(double, double)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 20 }, { QMetaType::Double, 21 },
        }}),
        // Method 'touchMove'
        QtMocHelpers::MethodData<void(double, double)>(22, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 20 }, { QMetaType::Double, 21 },
        }}),
        // Method 'touchUp'
        QtMocHelpers::MethodData<void(double, double)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 20 }, { QMetaType::Double, 21 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'status'
        QtMocHelpers::PropertyData<QString>(24, QMetaType::QString, QMC::DefaultPropertyFlags, 0),
        // property 'dongleReady'
        QtMocHelpers::PropertyData<bool>(25, QMetaType::Bool, QMC::DefaultPropertyFlags, 1),
        // property 'streaming'
        QtMocHelpers::PropertyData<bool>(26, QMetaType::Bool, QMC::DefaultPropertyFlags, 2),
        // property 'receivingVideo'
        QtMocHelpers::PropertyData<bool>(27, QMetaType::Bool, QMC::DefaultPropertyFlags, 3),
        // property 'frameCount'
        QtMocHelpers::PropertyData<int>(28, QMetaType::Int, QMC::DefaultPropertyFlags, 4),
        // property 'decodedFps'
        QtMocHelpers::PropertyData<double>(29, QMetaType::Double, QMC::DefaultPropertyFlags, 4),
        // property 'videoWidth'
        QtMocHelpers::PropertyData<int>(30, QMetaType::Int, QMC::DefaultPropertyFlags, 5),
        // property 'videoHeight'
        QtMocHelpers::PropertyData<int>(31, QMetaType::Int, QMC::DefaultPropertyFlags, 5),
        // property 'audioActive'
        QtMocHelpers::PropertyData<bool>(32, QMetaType::Bool, QMC::DefaultPropertyFlags, 6),
        // property 'audioPacketCount'
        QtMocHelpers::PropertyData<int>(33, QMetaType::Int, QMC::DefaultPropertyFlags, 7),
        // property 'videoSink'
        QtMocHelpers::PropertyData<QVideoSink*>(34, 0x80000000 | 35, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 8),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<CarplayController, qt_meta_tag_ZN17CarplayControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject CarplayController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17CarplayControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17CarplayControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN17CarplayControllerE_t>.metaTypes,
    nullptr
} };

void CarplayController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CarplayController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->statusChanged(); break;
        case 1: _t->dongleReadyChanged(); break;
        case 2: _t->streamingChanged(); break;
        case 3: _t->receivingVideoChanged(); break;
        case 4: _t->statsChanged(); break;
        case 5: _t->videoGeometryChanged(); break;
        case 6: _t->audioActiveChanged(); break;
        case 7: _t->audioStatsChanged(); break;
        case 8: _t->videoSinkChanged(); break;
        case 9: _t->verifyHardware(); break;
        case 10: _t->startStream((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 11: _t->stop(); break;
        case 12: _t->playCapture((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 13: _t->touchDown((*reinterpret_cast<std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<double>>(_a[2]))); break;
        case 14: _t->touchMove((*reinterpret_cast<std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<double>>(_a[2]))); break;
        case 15: _t->touchUp((*reinterpret_cast<std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<double>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (CarplayController::*)()>(_a, &CarplayController::statusChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (CarplayController::*)()>(_a, &CarplayController::dongleReadyChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (CarplayController::*)()>(_a, &CarplayController::streamingChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (CarplayController::*)()>(_a, &CarplayController::receivingVideoChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (CarplayController::*)()>(_a, &CarplayController::statsChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (CarplayController::*)()>(_a, &CarplayController::videoGeometryChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (CarplayController::*)()>(_a, &CarplayController::audioActiveChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (CarplayController::*)()>(_a, &CarplayController::audioStatsChanged, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (CarplayController::*)()>(_a, &CarplayController::videoSinkChanged, 8))
            return;
    }
    if (_c == QMetaObject::RegisterPropertyMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 10:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QVideoSink* >(); break;
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QString*>(_v) = _t->status(); break;
        case 1: *reinterpret_cast<bool*>(_v) = _t->dongleReady(); break;
        case 2: *reinterpret_cast<bool*>(_v) = _t->streaming(); break;
        case 3: *reinterpret_cast<bool*>(_v) = _t->receivingVideo(); break;
        case 4: *reinterpret_cast<int*>(_v) = _t->frameCount(); break;
        case 5: *reinterpret_cast<double*>(_v) = _t->decodedFps(); break;
        case 6: *reinterpret_cast<int*>(_v) = _t->videoWidth(); break;
        case 7: *reinterpret_cast<int*>(_v) = _t->videoHeight(); break;
        case 8: *reinterpret_cast<bool*>(_v) = _t->audioActive(); break;
        case 9: *reinterpret_cast<int*>(_v) = _t->audioPacketCount(); break;
        case 10: *reinterpret_cast<QVideoSink**>(_v) = _t->videoSink(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 10: _t->setVideoSink(*reinterpret_cast<QVideoSink**>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *CarplayController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CarplayController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17CarplayControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int CarplayController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 16;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void CarplayController::statusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void CarplayController::dongleReadyChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void CarplayController::streamingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void CarplayController::receivingVideoChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void CarplayController::statsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void CarplayController::videoGeometryChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void CarplayController::audioActiveChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void CarplayController::audioStatsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void CarplayController::videoSinkChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}
QT_WARNING_POP

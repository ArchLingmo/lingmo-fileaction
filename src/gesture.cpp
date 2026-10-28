/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "gesture.h"

#include <QDebug>
#include <QLoggingCategory>
#include <QSocketNotifier>
#include <QUrl>

#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xfixes.h>
#include <X11/keysym.h>

#include <climits>

Q_LOGGING_CATEGORY(lcGesture, "lingmo.fileaction.gesture", QtInfoMsg)

DragGesture::DragGesture(QObject *parent)
    : QObject(parent)
{
    // A connection of our own: Qt's is busy being a GUI, and this one only listens
    m_dpy = XOpenDisplay(nullptr);
    if (!m_dpy) {
        qWarning() << "lingmo-fileaction: no X11 display, the drag gesture is off";
        return;
    }
    int event, error, major = 2, minor = 2;
    if (!XQueryExtension(m_dpy, "XInputExtension", &m_xiOpcode, &event, &error)
        || XIQueryVersion(m_dpy, &major, &minor) != Success) {
        qWarning() << "lingmo-fileaction: XInput 2.2 missing, the drag gesture is off";
        XCloseDisplay(m_dpy);
        m_dpy = nullptr;
        return;
    }
    int fixesError;
    if (!XFixesQueryExtension(m_dpy, &m_fixesEvent, &fixesError)) {
        qWarning() << "lingmo-fileaction: XFixes missing, the drag gesture is off";
        XCloseDisplay(m_dpy);
        m_dpy = nullptr;
        return;
    }
    m_root = DefaultRootWindow(m_dpy);
    // Requestor for the selection conversion: never mapped
    m_window = XCreateSimpleWindow(m_dpy, m_root, -10, -10, 1, 1, 0, 0, 0);
    m_xdndSelection = XInternAtom(m_dpy, "XdndSelection", False);
    m_uriList = XInternAtom(m_dpy, "text/uri-list", False);
    m_property = XInternAtom(m_dpy, "LINGMO_FILEACTION_DRAG", False);

    unsigned char mask[XIMaskLen(XI_LASTEVENT)] = {};
    XISetMask(mask, XI_RawKeyPress);
    XISetMask(mask, XI_RawKeyRelease);
    XISetMask(mask, XI_RawButtonPress);
    XISetMask(mask, XI_RawButtonRelease);
    XIEventMask em;
    em.deviceid = XIAllMasterDevices;
    em.mask_len = sizeof(mask);
    em.mask = mask;
    XISelectEvents(m_dpy, m_root, &em, 1);
    XFixesSelectSelectionInput(m_dpy, m_root, m_xdndSelection, XFixesSetSelectionOwnerNotifyMask);
    m_owner = XGetSelectionOwner(m_dpy, m_xdndSelection) != None;
    XFlush(m_dpy);

    m_notifier = new QSocketNotifier(ConnectionNumber(m_dpy), QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &DragGesture::processEvents);
}

DragGesture::~DragGesture()
{
    if (m_dpy) {
        XDestroyWindow(m_dpy, m_window);
        XCloseDisplay(m_dpy);
    }
}

// The tap keys; which wheel each opens is the user's choice (Prefs::wheelForKey)
static QString tapKeyForKeysym(KeySym sym)
{
    switch (sym) {
    case XK_Shift_L:
    case XK_Shift_R:
        return QStringLiteral("shift");
    case XK_Control_L:
    case XK_Control_R:
        return QStringLiteral("ctrl");
    case XK_Alt_L:
    case XK_Alt_R:
    case XK_Meta_L:
    case XK_Meta_R:
        return QStringLiteral("alt");
    default:
        return QString();
    }
}

static QString keyName(KeySym sym)
{
    if (sym >= XK_1 && sym <= XK_9)
        return QString::number(int(sym - XK_0));
    if (sym >= XK_KP_1 && sym <= XK_KP_9)
        return QString::number(int(sym - XK_KP_0));
    switch (sym) {
    case XK_r:
    case XK_R:
        return QStringLiteral("r");
    case XK_Escape:
        return QStringLiteral("escape");
    case XK_Tab:
        return QStringLiteral("tab");
    default:
        return QString();
    }
}

void DragGesture::updateDrag()
{
    // Times are server milliseconds; a new owner after the button went down is a drag
    const bool active = m_button && m_owner && m_ownerTime >= m_buttonTime && m_ownerTime != 0;
    if (active == m_dragActive)
        return;
    m_dragActive = active;
    qCDebug(lcGesture) << "drag" << (active ? "started" : "ended");
    if (active)
        Q_EMIT dragStarted();
    else
        Q_EMIT dragEnded();
}

void DragGesture::processEvents()
{
    while (XPending(m_dpy)) {
        XEvent ev;
        XNextEvent(m_dpy, &ev);

        if (ev.type == m_fixesEvent + XFixesSelectionNotify) {
            auto *sel = reinterpret_cast<XFixesSelectionNotifyEvent *>(&ev);
            if (sel->selection == m_xdndSelection) {
                m_owner = sel->owner != None;
                m_ownerTime = sel->timestamp;
                m_selectionTime = sel->selection_timestamp;
                qCDebug(lcGesture) << "XdndSelection owner" << sel->owner << "time" << sel->timestamp << "button" << m_button << m_buttonTime;
                updateDrag();
            }
            continue;
        }

        if (ev.type == SelectionNotify) {
            if (ev.xselection.selection != m_xdndSelection)
                continue;
            QStringList paths;
            if (ev.xselection.property != None) {
                Atom type;
                int format;
                unsigned long items, after;
                unsigned char *data = nullptr;
                if (XGetWindowProperty(m_dpy, m_window, m_property, 0, LONG_MAX / 4, True, AnyPropertyType, &type,
                                       &format, &items, &after, &data) == Success && data) {
                    const QByteArray list(reinterpret_cast<const char *>(data), int(items * (format / 8)));
                    for (const QByteArray &raw : list.split('\n')) {
                        const QByteArray line = raw.trimmed();
                        if (line.isEmpty() || line.startsWith('#'))
                            continue;
                        const QUrl url = QUrl::fromEncoded(line);
                        if (url.isLocalFile())
                            paths << url.toLocalFile();
                    }
                    XFree(data);
                }
            }
            qCDebug(lcGesture) << "drag files" << paths << "property" << ev.xselection.property;
            Q_EMIT dragFiles(paths);
            continue;
        }

        if (ev.xcookie.type != GenericEvent || ev.xcookie.extension != m_xiOpcode)
            continue;
        if (!XGetEventData(m_dpy, &ev.xcookie))
            continue;
        const auto *raw = static_cast<const XIRawEvent *>(ev.xcookie.data);
        // the same hardware event can arrive once per device (master and slave): count it once
        if (ev.xcookie.evtype == m_lastRawType && raw->detail == m_lastRawDetail && raw->time == m_lastRawTime) {
            XFreeEventData(m_dpy, &ev.xcookie);
            continue;
        }
        m_lastRawType = ev.xcookie.evtype;
        m_lastRawDetail = raw->detail;
        m_lastRawTime = raw->time;
        switch (ev.xcookie.evtype) {
        case XI_RawButtonPress:
            m_pendingWheel.clear(); // modifier + click is not a tap
            if (raw->detail == 1) {
                m_button = true;
                m_buttonTime = raw->time;
                updateDrag();
            }
            break;
        case XI_RawButtonRelease:
            if (raw->detail == 1) {
                m_button = false;
                updateDrag();
            }
            break;
        case XI_RawKeyPress: {
            if (raw->flags & XIKeyRepeat)
                break; // a held key
            const KeySym sym = XkbKeycodeToKeysym(m_dpy, KeyCode(raw->detail), 0, 0);
            const QString wheel = tapKeyForKeysym(sym);
            if (!wheel.isEmpty()) {
                // a second modifier pressed while the first is held: a chord, not a tap
                m_pendingWheel = m_pendingWheel.isEmpty() ? wheel : QStringLiteral("-");
                m_pendingTime = raw->time;
            } else {
                m_pendingWheel.clear();
                if (m_dragActive) {
                    const QString name = keyName(sym);
                    if (!name.isEmpty())
                        Q_EMIT keyDuringDrag(name);
                }
            }
            break;
        }
        case XI_RawKeyRelease: {
            const KeySym sym = XkbKeycodeToKeysym(m_dpy, KeyCode(raw->detail), 0, 0);
            const QString wheel = tapKeyForKeysym(sym);
            const bool tap = !wheel.isEmpty() && wheel == m_pendingWheel && raw->time - m_pendingTime <= unsigned(m_tapMs) && m_dragActive;
            // cleared before anyone hears of the tap: a second release (xdotool sends one) is no tap
            m_pendingWheel.clear();
            if (tap) {
                qCDebug(lcGesture) << "tap" << wheel;
                Q_EMIT tapped(wheel);
            }
            break;
        }
        }
        XFreeEventData(m_dpy, &ev.xcookie);
    }
}

void DragGesture::requestDragFiles()
{
    if (!m_dpy)
        return;
    // XDND sources answer for the drag whose timestamp they set on the selection
    XConvertSelection(m_dpy, m_xdndSelection, m_uriList, m_property, m_window,
                      m_selectionTime ? m_selectionTime : CurrentTime);
    XFlush(m_dpy);
    // never re-entered from a signal emitted inside processEvents()
    QMetaObject::invokeMethod(this, &DragGesture::processEvents, Qt::QueuedConnection);
}

void DragGesture::nudgePointer()
{
    if (!m_dpy || !m_dragActive)
        return;
    XWarpPointer(m_dpy, None, None, 0, 0, 0, 0, 1, 0);
    XFlush(m_dpy);
    XWarpPointer(m_dpy, None, None, 0, 0, 0, 0, -1, 0);
    XFlush(m_dpy);
}

QString DragGesture::activeWindowClass() const
{
    if (!m_dpy)
        return QString();
    const Atom activeAtom = XInternAtom(m_dpy, "_NET_ACTIVE_WINDOW", False);
    Atom type;
    int format;
    unsigned long items, after;
    unsigned char *data = nullptr;
    Window active = None;
    if (XGetWindowProperty(m_dpy, m_root, activeAtom, 0, 1, False, AnyPropertyType, &type, &format, &items, &after, &data)
            == Success && data) {
        if (items == 1)
            active = *reinterpret_cast<Window *>(data);
        XFree(data);
    }
    if (active == None)
        return QString();
    XClassHint hint{};
    QString result;
    if (XGetClassHint(m_dpy, active, &hint)) {
        result = QString::fromLocal8Bit(hint.res_class ? hint.res_class : "").toLower() + QLatin1Char(' ')
                 + QString::fromLocal8Bit(hint.res_name ? hint.res_name : "").toLower();
        if (hint.res_class)
            XFree(hint.res_class);
        if (hint.res_name)
            XFree(hint.res_name);
    }
    // the round trips above may have queued events the socket notifier won't see
    QMetaObject::invokeMethod(const_cast<DragGesture *>(this), &DragGesture::processEvents, Qt::QueuedConnection);
    return result;
}

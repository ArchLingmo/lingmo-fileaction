/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QObject>
#include <QStringList>

class QSocketNotifier;
typedef struct _XDisplay Display;

// The drag-and-tap gesture on X11 (the Windows FileAction does it with global
// hooks). Nothing is grabbed: XInput2 *raw* key and button events are only
// observed, so Shift/Ctrl/Alt keep working as copy/move modifiers in every
// file manager.
//
//  - A drag is in progress while mouse button 1 is held and the XdndSelection
//    got a new owner after it went down (XFixes selection notify): every XDND
//    source (Qt, GTK, browsers) takes that selection when a drag starts.
//  - A tap is Shift, Ctrl or Alt pressed and released within 300 ms (configurable) with no
//    other key or button in between (holding a modifier to choose copy/move is
//    not a tap). A tap during a drag emits tapped().
//  - While a drag is in progress, R, Esc, Tab and 1-9 are reported too (the drag
//    source holds the keyboard, so the wheel can't get them as key events).
class DragGesture : public QObject
{
    Q_OBJECT
public:
    explicit DragGesture(QObject *parent = nullptr);
    ~DragGesture() override;

    bool isValid() const { return m_dpy != nullptr; }
    bool dragActive() const { return m_dragActive; }

    // Asks the drag source for its files (XdndSelection as text/uri-list); answers with dragFiles()
    void requestDragFiles();

    // Moves the pointer 1 px and back: the drag source then tells the window now under the
    // pointer (the wheel) about the drag at once (XdndEnter/XdndPosition), without waiting for the hand
    void nudgePointer();

    // WM_CLASS of the active window (_NET_ACTIVE_WINDOW), lower case
    QString activeWindowClass() const;

    // a tap is a press and release within this time (Settings › Gestures, 300 ms by default)
    void setTapMs(int ms) { m_tapMs = ms; }

Q_SIGNALS:
    void tapped(const QString &key); // "shift", "ctrl", "alt" (Alt or Meta)
    void keyDuringDrag(const QString &key); // "r", "escape", "tab", "1".."9"
    void dragStarted();
    void dragEnded();
    void dragFiles(const QStringList &paths);

private:
    void processEvents();
    void updateDrag();

    Display *m_dpy = nullptr;
    unsigned long m_root = 0;
    unsigned long m_window = 0;
    int m_xiOpcode = 0;
    int m_fixesEvent = 0;
    unsigned long m_xdndSelection = 0;
    unsigned long m_uriList = 0;
    unsigned long m_property = 0;
    QSocketNotifier *m_notifier = nullptr;

    bool m_button = false;
    unsigned long m_buttonTime = 0;
    bool m_owner = false;
    unsigned long m_ownerTime = 0;
    unsigned long m_selectionTime = 0; // the source's own timestamp for the selection
    int m_lastRawType = 0;
    int m_lastRawDetail = 0;
    unsigned long m_lastRawTime = 0;
    bool m_dragActive = false;

    int m_tapMs = 300;
    QString m_pendingWheel;
    unsigned long m_pendingTime = 0;
};

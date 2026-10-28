/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QJsonObject>
#include <QObject>
#include <QPoint>
#include <QVariantList>

class JobModel;
class DragGesture;

// What the wheel (qml/Wheel.qml) shows and does. Three wheels:
//   convert (Shift)  formats for these files, from lingmo-fileaction-engine list --json
//   tools   (Ctrl)   compress, shrink, rotate, trim, merge... (also from the engine)
//   actions (Alt)    copy to, move to, copy path, open terminal here, share
// plus "repeat": one slice that does the last action again with the same settings.
class WheelController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)
    Q_PROPERTY(bool dragMode READ dragMode NOTIFY visibleChanged)
    Q_PROPERTY(QPoint anchor READ anchor NOTIFY visibleChanged)
    Q_PROPERTY(QStringList files READ files NOTIFY filesChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY filesChanged)
    Q_PROPERTY(QString wheel READ wheel NOTIFY slicesChanged)
    Q_PROPERTY(bool repeatMode READ repeatMode NOTIFY slicesChanged)
    Q_PROPERTY(QVariantList slices READ slices NOTIFY slicesChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY slicesChanged)
    Q_PROPERTY(QString error READ error NOTIFY slicesChanged)
    Q_PROPERTY(bool canRepeat READ canRepeat NOTIFY slicesChanged)
    Q_PROPERTY(QString lastLabel READ lastLabel NOTIFY slicesChanged)

public:
    WheelController(JobModel *jobs, DragGesture *gesture, QObject *parent = nullptr);

    bool visible() const { return m_visible; }
    bool dragMode() const { return m_dragMode; }
    QPoint anchor() const { return m_anchor; }
    QStringList files() const { return m_files; }
    QString summary() const;
    QString wheel() const { return m_wheel; }
    bool repeatMode() const { return m_repeat; }
    QVariantList slices() const;
    bool loading() const { return m_loading; }
    QString error() const { return m_error; }
    bool canRepeat() const;
    QString lastLabel() const;

    // Shows the wheel at the pointer. dragMode: opened by the gesture, files arrive with the drop
    void open(const QStringList &files, const QString &wheel, bool dragMode);
    // Meta+Shift+A: the files selected in Lingmo Files, else the clipboard, else an empty drop zone
    void openKeyboard();

    Q_INVOKABLE void setDroppedFiles(const QVariantList &urls);
    Q_INVOKABLE void switchWheel(const QString &wheel);
    Q_INVOKABLE void cycleWheel(int delta);
    Q_INVOKABLE void repeat();
    // Runs slice `index` (urls: the dropped files, if any). Returns false when the slice needs
    // parameters first (parametersRequested) or a folder (folderRequested)
    Q_INVOKABLE bool activate(int index, const QVariantList &urls = {});
    Q_INVOKABLE void runWithParameters(int index, const QVariantMap &params);
    Q_INVOKABLE void runWithFolder(const QString &actionId, const QUrl &folder);
    Q_INVOKABLE QVariantMap lastParameters(const QString &id) const;
    Q_INVOKABLE void close();
    // The wheel is on screen in a drag: let the drag source notice it (see DragGesture::nudgePointer)
    Q_INVOKABLE void shown();
    // the gear on the wheel
    Q_INVOKABLE void openSettings();

    // The Actions wheel before the user's choices: up to openWith apps for this file type, then the actions
    static QVariantList baseActionSlices(const QString &mime, int openWith);

Q_SIGNALS:
    void visibleChanged();
    void filesChanged();
    void slicesChanged();
    void parametersRequested(int index, const QVariantMap &slice);
    void folderRequested(const QString &actionId, bool move);
    void keyDuringDrag(const QString &key);
    void settingsRequested();

private:
    void load();
    void runSlice(const QVariantMap &slice, const QStringList &files, const QVariantMap &params);
    void runAction(const QString &id, const QStringList &files, const QVariantMap &params);
    void remember(const QVariantMap &slice, const QVariantMap &params);
    QVariantList actionSlices() const;
    QVariantList opSlices(const QString &wheel) const;
    QVariantMap repeatSlice() const;
    void transfer(const QStringList &files, const QString &folder, bool move);

    JobModel *m_jobs;
    DragGesture *m_gesture;
    bool m_visible = false;
    bool m_dragMode = false;
    QPoint m_anchor;
    QStringList m_files;
    QString m_wheel = QStringLiteral("convert");
    bool m_repeat = false;
    bool m_loading = false;
    QString m_error;
    QJsonObject m_listing; // engine "list --json" for m_files
    int m_loadSerial = 0;
};

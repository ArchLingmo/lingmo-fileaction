/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QVariantList>

class QProcess;

// The settings window (qml/Settings.qml) and everything that honours it.
//
// Where each setting lives:
//   ~/.config/fileaction/settings.json   FileAction's own file, read by lingmo-fileaction-engine on every run:
//                                        OutputDirectory, Quality {JpegQuality, CompressQuality, PdfDpi,
//                                        VideoCrf, CompressVideoCrf, StripLocation}, HiddenOperations,
//                                        FavoriteOperations (upstream's names; unknown keys are kept)
//   ~/.config/lingmoos/fileaction.conf   ours: gesture/{enabled,tapMs,shift,ctrl,alt}, after/{island,openFolder},
//                                        wheels/{openWith,order_convert,order_tools,order_actions}
//   ~/.config/autostart/lingmo-fileaction-daemon.desktop   Hidden=true turns the session daemon off
//   ~/.config/lingmoglobalshortcutsrc     the [<keys>] group whose Exec is "lingmo-fileaction --keyboard"
//                                        (lingmo-core's chotkeys reloads it when it changes)
class Prefs : public QObject
{
    Q_OBJECT
    // bumped on every change: bindings that call the invokables below read it to update
    Q_PROPERTY(int revision READ revision NOTIFY changed)

    // General
    Q_PROPERTY(bool autostart READ autostart WRITE setAutostart NOTIFY changed)
    Q_PROPERTY(QString outputDirectory READ outputDirectory WRITE setOutputDirectory NOTIFY changed)
    Q_PROPERTY(bool showInIsland READ showInIsland WRITE setShowInIsland NOTIFY changed)
    Q_PROPERTY(bool openFolder READ openFolder WRITE setOpenFolder NOTIFY changed)
    Q_PROPERTY(bool wheelGlass READ wheelGlass WRITE setWheelGlass NOTIFY changed)
    Q_PROPERTY(QString settingsError READ settingsError NOTIFY changed)

    // Gestures and keys
    Q_PROPERTY(bool gestureEnabled READ gestureEnabled WRITE setGestureEnabled NOTIFY changed)
    Q_PROPERTY(int tapMs READ tapMs WRITE setTapMs NOTIFY changed)
    Q_PROPERTY(QVariantMap keyLabels READ keyLabels NOTIFY changed)   // wheel id -> "⇧" / "Ctrl" / "Alt" / ""
    Q_PROPERTY(QString shortcut READ shortcut NOTIFY changed)

    // Quality (FileAction's Settings class)
    Q_PROPERTY(int jpegQuality READ jpegQuality WRITE setJpegQuality NOTIFY changed)
    Q_PROPERTY(int compressQuality READ compressQuality WRITE setCompressQuality NOTIFY changed)
    Q_PROPERTY(int pdfDpi READ pdfDpi WRITE setPdfDpi NOTIFY changed)
    Q_PROPERTY(int videoCrf READ videoCrf WRITE setVideoCrf NOTIFY changed)
    Q_PROPERTY(int compressVideoCrf READ compressVideoCrf WRITE setCompressVideoCrf NOTIFY changed)
    Q_PROPERTY(bool stripLocation READ stripLocation WRITE setStripLocation NOTIFY changed)

    // Wheels
    Q_PROPERTY(int openWithCount READ openWithCount WRITE setOpenWithCount NOTIFY changed)
    Q_PROPERTY(bool catalogLoading READ catalogLoading NOTIFY catalogChanged)
    Q_PROPERTY(QString catalogError READ catalogError NOTIFY catalogChanged)
    Q_PROPERTY(int maxSlices READ maxSlices CONSTANT)

    // Helper programs
    Q_PROPERTY(QVariantList helpers READ helpers NOTIFY helpersChanged)
    Q_PROPERTY(QString installing READ installing NOTIFY helpersChanged)      // helper id being installed
    Q_PROPERTY(QString installMessage READ installMessage NOTIFY helpersChanged)
    Q_PROPERTY(bool installOk READ installOk NOTIFY helpersChanged)

    // About
    Q_PROPERTY(QString engineVersion READ engineVersion NOTIFY catalogChanged)

public:
    static Prefs *instance();

    static constexpr int MaxSlices = 12;
    static constexpr int MaxOpenWith = 4;
    static constexpr int DefaultTapMs = 300;
    static QString defaultShortcut() { return QStringLiteral("Meta+Shift+A"); }

    int revision() const { return m_revision; }

    bool autostart() const;
    void setAutostart(bool on);
    QString outputDirectory() const;
    void setOutputDirectory(const QString &dir);
    bool showInIsland() const;
    void setShowInIsland(bool on);
    bool openFolder() const;
    void setOpenFolder(bool on);
    // false draws the wheel solid; read again each time the wheel opens
    bool wheelGlass() const;
    void setWheelGlass(bool on);
    QString settingsError() const { return m_settingsError; }

    bool gestureEnabled() const;
    void setGestureEnabled(bool on);
    int tapMs() const;
    void setTapMs(int ms);
    QVariantMap keyLabels() const;
    // "shift" | "ctrl" | "alt" -> "convert" | "tools" | "actions" | "" (nothing)
    Q_INVOKABLE QString wheelForKey(const QString &key) const;
    Q_INVOKABLE void setWheelForKey(const QString &key, const QString &wheel);
    QString shortcut() const;
    // Returns an error text, or "" when saved
    Q_INVOKABLE QString setShortcut(const QString &sequence);
    // A key press in QML as "Meta+Shift+A" (empty for a lone modifier)
    Q_INVOKABLE QString sequenceFor(int key, int modifiers) const;

    int jpegQuality() const { return quality(QStringLiteral("JpegQuality"), 90); }
    void setJpegQuality(int v) { setQuality(QStringLiteral("JpegQuality"), qBound(10, v, 100)); }
    int compressQuality() const { return quality(QStringLiteral("CompressQuality"), 70); }
    void setCompressQuality(int v) { setQuality(QStringLiteral("CompressQuality"), qBound(10, v, 100)); }
    int pdfDpi() const { return quality(QStringLiteral("PdfDpi"), 300); }
    void setPdfDpi(int v) { setQuality(QStringLiteral("PdfDpi"), qBound(72, v, 600)); }
    int videoCrf() const { return quality(QStringLiteral("VideoCrf"), 23); }
    void setVideoCrf(int v) { setQuality(QStringLiteral("VideoCrf"), qBound(0, v, 51)); }
    int compressVideoCrf() const { return quality(QStringLiteral("CompressVideoCrf"), 28); }
    void setCompressVideoCrf(int v) { setQuality(QStringLiteral("CompressVideoCrf"), qBound(0, v, 51)); }
    bool stripLocation() const;
    void setStripLocation(bool on);
    Q_INVOKABLE void resetQuality();

    int openWithCount() const;
    void setOpenWithCount(int n);
    bool catalogLoading() const { return m_catalogLoading; }
    QString catalogError() const { return m_catalogError; }
    int maxSlices() const { return MaxSlices; }

    // The wheels page: categories of a wheel with how many slices they show, the slices of one category
    // (all of them, with "hidden"/"favorite"), and the slices as the wheel shows them (the preview)
    Q_INVOKABLE QVariantList categories(const QString &wheel) const;
    Q_INVOKABLE QVariantList sliceList(const QString &wheel, const QString &category) const;
    Q_INVOKABLE QVariantList previewSlices(const QString &wheel, const QString &category) const;
    Q_INVOKABLE void setHidden(const QString &id, bool hidden);
    Q_INVOKABLE void setFavorite(const QString &id, bool favorite);
    // Moves a slice one place up (-1) or down (+1) among the slices of that category
    Q_INVOKABLE void move(const QString &wheel, const QString &category, const QString &id, int delta);
    Q_INVOKABLE void resetWheel(const QString &wheel);
    Q_INVOKABLE void reloadCatalog();

    // What the real wheel shows: hidden slices out, favourites first (in the order they were starred),
    // then the user's order, then the engine's; "Open with" apps right after the favourites
    QVariantList arrange(const QVariantList &slices, const QString &wheel) const;

    QVariantList helpers() const { return m_helpers; }
    QString installing() const { return m_installing; }
    QString installMessage() const { return m_installMessage; }
    bool installOk() const { return m_installOk; }
    Q_INVOKABLE void refreshHelpers();
    Q_INVOKABLE void install(const QString &helperId);

    QString engineVersion() const { return m_engineVersion; }

    Q_INVOKABLE void openUrl(const QString &url) const;

Q_SIGNALS:
    void changed();
    void catalogChanged();
    void helpersChanged();

private:
    explicit Prefs(QObject *parent = nullptr);

    QJsonObject engineSettings() const;          // settings.json, re-read when it changed on disk
    bool writeEngineSettings(const QJsonObject &doc);
    int quality(const QString &key, int fallback) const;
    void setQuality(const QString &key, const QJsonValue &value);
    QStringList engineList(const QString &key) const;
    void setEngineList(const QString &key, const QStringList &list);
    QStringList order(const QString &wheel) const;
    void bump();

    int m_revision = 0;
    QString m_settingsError;
    mutable QJsonObject m_engine;
    mutable qint64 m_engineStamp = -2;

    bool m_catalogLoading = false;
    QString m_catalogError;
    QVariantList m_catalog;   // every operation: {id, wheel, label, subtitle, available, missing, category}
    QString m_engineVersion;

    QVariantList m_helpers;
    QString m_installing;
    QString m_installMessage;
    bool m_installOk = false;
    QPointer<QProcess> m_installer;
};

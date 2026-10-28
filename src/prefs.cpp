/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "prefs.h"
#include "engine.h"
#include "labels.h"
#include "wheel.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QKeySequence>
#include <QLocale>
#include <QProcess>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

#include <algorithm>
#include <climits>

static QSettings conf()
{
    return QSettings(QStringLiteral("lingmoos"), QStringLiteral("fileaction"));
}

static QString configHome()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
}

// FileAction's LinuxSettings.FilePath: $XDG_CONFIG_HOME/fileaction/settings.json
static QString engineSettingsPath()
{
    return configHome() + QStringLiteral("/fileaction/settings.json");
}

static QString autostartName()
{
    return QStringLiteral("lingmo-fileaction-daemon.desktop");
}

static QString shortcutsPath()
{
    return configHome() + QStringLiteral("/lingmoglobalshortcutsrc");
}

static const QString KeyboardCommand = QStringLiteral("lingmo-fileaction --keyboard");
static const QStringList Wheels{QStringLiteral("convert"), QStringLiteral("tools"), QStringLiteral("actions")};

// Operation id -> the file type it is for (the engine's ids start with the kind of file)
static QString categoryOf(const QString &id)
{
    const QString kind = id.section(QLatin1Char('.'), 0, 0);
    if (kind == QLatin1String("image") || kind == QLatin1String("gif") || id == QLatin1String("tools.ocr"))
        return QStringLiteral("images");
    if (kind == QLatin1String("video"))
        return QStringLiteral("video");
    if (kind == QLatin1String("audio"))
        return QStringLiteral("audio");
    if (kind == QLatin1String("pdf"))
        return QStringLiteral("pdf");
    if (kind == QLatin1String("doc"))
        return QStringLiteral("documents");
    if (kind == QLatin1String("slides"))
        return QStringLiteral("slides");
    if (kind == QLatin1String("sheet"))
        return QStringLiteral("spreadsheets");
    if (kind == QLatin1String("subs"))
        return QStringLiteral("subtitles");
    if (kind == QLatin1String("archive") || kind == QLatin1String("pack"))
        return QStringLiteral("archives");
    if (kind == QLatin1String("preset"))
        return QStringLiteral("presets");
    return QStringLiteral("other");
}

Prefs *Prefs::instance()
{
    static Prefs *prefs = new Prefs;
    return prefs;
}

Prefs::Prefs(QObject *parent)
    : QObject(parent)
{
    // the catalog and the helper programs are looked up when the settings window opens (reloadCatalog)
}

void Prefs::bump()
{
    ++m_revision;
    Q_EMIT changed();
}

// ------------------------------------------------------------------ settings.json (FileAction's file)

QJsonObject Prefs::engineSettings() const
{
    const QFileInfo info(engineSettingsPath());
    const qint64 stamp = info.exists() ? info.lastModified().toMSecsSinceEpoch() * 1000 + (info.size() % 1000) : -1;
    if (stamp != m_engineStamp) {
        m_engineStamp = stamp;
        m_engine = QJsonObject();
        QFile f(info.filePath());
        if (f.open(QIODevice::ReadOnly))
            m_engine = QJsonDocument::fromJson(f.readAll()).object();
    }
    return m_engine;
}

bool Prefs::writeEngineSettings(const QJsonObject &doc)
{
    const QString path = engineSettingsPath();
    // a file we can't parse is someone's: never replace it with ours
    QFile current(path);
    if (current.open(QIODevice::ReadOnly)) {
        const QByteArray data = current.readAll().trimmed();
        QJsonParseError error;
        QJsonDocument::fromJson(data, &error);
        if (!data.isEmpty() && error.error != QJsonParseError::NoError) {
            m_settingsError = tr("%1 can't be read, so it was not changed.").arg(path);
            bump();
            return false;
        }
    }
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly)) {
        m_settingsError = tr("%1 can't be written.").arg(path);
        bump();
        return false;
    }
    out.write(QJsonDocument(doc).toJson(QJsonDocument::Indented));
    if (!out.commit()) {
        m_settingsError = tr("%1 can't be written.").arg(path);
        bump();
        return false;
    }
    m_settingsError.clear();
    m_engine = doc;
    const QFileInfo info(path);
    m_engineStamp = info.lastModified().toMSecsSinceEpoch() * 1000 + (info.size() % 1000);
    bump();
    return true;
}

int Prefs::quality(const QString &key, int fallback) const
{
    const QJsonValue v = engineSettings().value(QStringLiteral("Quality")).toObject().value(key);
    return v.isDouble() ? v.toInt() : fallback;
}

void Prefs::setQuality(const QString &key, const QJsonValue &value)
{
    QJsonObject doc = engineSettings();
    QJsonObject q = doc.value(QStringLiteral("Quality")).toObject();
    if (q.value(key) == value)
        return;
    // the whole object, so the engine never mixes a partial one with its defaults
    const QJsonObject defaults{{QStringLiteral("JpegQuality"), 90}, {QStringLiteral("CompressQuality"), 70},
                               {QStringLiteral("PdfDpi"), 300}, {QStringLiteral("VideoCrf"), 23},
                               {QStringLiteral("CompressVideoCrf"), 28}, {QStringLiteral("StripLocation"), true}};
    for (auto it = defaults.begin(); it != defaults.end(); ++it) {
        if (!q.contains(it.key()))
            q.insert(it.key(), it.value());
    }
    q.insert(key, value);
    doc.insert(QStringLiteral("Quality"), q);
    writeEngineSettings(doc);
}

bool Prefs::stripLocation() const
{
    const QJsonValue v = engineSettings().value(QStringLiteral("Quality")).toObject().value(QStringLiteral("StripLocation"));
    return v.isBool() ? v.toBool() : true;
}

void Prefs::setStripLocation(bool on)
{
    setQuality(QStringLiteral("StripLocation"), on);
}

void Prefs::resetQuality()
{
    QJsonObject doc = engineSettings();
    doc.remove(QStringLiteral("Quality"));
    writeEngineSettings(doc);
}

QStringList Prefs::engineList(const QString &key) const
{
    QStringList list;
    for (const QJsonValue &v : engineSettings().value(key).toArray())
        list << v.toString();
    return list;
}

void Prefs::setEngineList(const QString &key, const QStringList &list)
{
    QJsonObject doc = engineSettings();
    doc.insert(key, QJsonArray::fromStringList(list));
    writeEngineSettings(doc);
}

QString Prefs::outputDirectory() const
{
    return engineSettings().value(QStringLiteral("OutputDirectory")).toString();
}

void Prefs::setOutputDirectory(const QString &dir)
{
    QString path = dir;
    if (path.startsWith(QLatin1String("file:")))
        path = QUrl(path).toLocalFile();
    if (path == outputDirectory())
        return;
    QJsonObject doc = engineSettings();
    doc.insert(QStringLiteral("OutputDirectory"), path.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(path));
    writeEngineSettings(doc);
}

// ------------------------------------------------------------------ General

static QString autostartOverride()
{
    return configHome() + QStringLiteral("/autostart/") + autostartName();
}

bool Prefs::autostart() const
{
    QFile f(autostartOverride());
    if (!f.open(QIODevice::ReadOnly))
        return true; // /etc/xdg/autostart starts it
    for (const QByteArray &raw : f.readAll().split('\n')) {
        const QByteArray line = raw.trimmed();
        if (line == "Hidden=true" || line == "X-GNOME-Autostart-enabled=false")
            return false;
    }
    return true;
}

void Prefs::setAutostart(bool on)
{
    if (on == autostart())
        return;
    const QString path = autostartOverride();
    if (on) {
        QFile::remove(path);
    } else {
        // the system entry with Hidden=true: the session leaves it out (XDG autostart spec)
        QByteArray content;
        const QString system = QStandardPaths::locate(QStandardPaths::GenericConfigLocation,
                                                      QStringLiteral("autostart/") + autostartName());
        QFile sys(system);
        if (!system.isEmpty() && system != path && sys.open(QIODevice::ReadOnly)) {
            for (const QByteArray &line : sys.readAll().split('\n')) {
                if (!line.startsWith("Hidden=") && !line.trimmed().isEmpty())
                    content += line + '\n';
            }
        } else {
            content = "[Desktop Entry]\nType=Application\nName=File actions (drag gesture)\nExec=lingmo-fileaction --daemon\n";
        }
        content += "Hidden=true\n";
        QDir().mkpath(QFileInfo(path).absolutePath());
        QSaveFile out(path);
        if (out.open(QIODevice::WriteOnly)) {
            out.write(content);
            out.commit();
        }
    }
    bump();
}

bool Prefs::showInIsland() const
{
    return conf().value(QStringLiteral("after/island"), true).toBool();
}

void Prefs::setShowInIsland(bool on)
{
    conf().setValue(QStringLiteral("after/island"), on);
    bump();
}

bool Prefs::openFolder() const
{
    return conf().value(QStringLiteral("after/openFolder"), false).toBool();
}

void Prefs::setOpenFolder(bool on)
{
    conf().setValue(QStringLiteral("after/openFolder"), on);
    bump();
}

bool Prefs::wheelGlass() const
{
    // the wheel runs in the daemon, the switch in the settings window
    QSettings settings = conf();
    settings.sync();
    return settings.value(QStringLiteral("wheel/glass"), true).toBool();
}

void Prefs::setWheelGlass(bool on)
{
    conf().setValue(QStringLiteral("wheel/glass"), on);
    bump();
}

// ------------------------------------------------------------------ Gestures and keys

bool Prefs::gestureEnabled() const
{
    return conf().value(QStringLiteral("gesture/enabled"), true).toBool();
}

void Prefs::setGestureEnabled(bool on)
{
    conf().setValue(QStringLiteral("gesture/enabled"), on);
    bump();
}

int Prefs::tapMs() const
{
    return qBound(150, conf().value(QStringLiteral("gesture/tapMs"), DefaultTapMs).toInt(), 800);
}

void Prefs::setTapMs(int ms)
{
    conf().setValue(QStringLiteral("gesture/tapMs"), qBound(150, ms, 800));
    bump();
}

QString Prefs::wheelForKey(const QString &key) const
{
    static const QMap<QString, QString> defaults{{QStringLiteral("shift"), QStringLiteral("convert")},
                                                 {QStringLiteral("ctrl"), QStringLiteral("tools")},
                                                 {QStringLiteral("alt"), QStringLiteral("actions")}};
    if (!defaults.contains(key))
        return QString();
    const QString wheel = conf().value(QStringLiteral("gesture/") + key, defaults.value(key)).toString();
    return Wheels.contains(wheel) ? wheel : QString();
}

void Prefs::setWheelForKey(const QString &key, const QString &wheel)
{
    if (key != QLatin1String("shift") && key != QLatin1String("ctrl") && key != QLatin1String("alt"))
        return;
    conf().setValue(QStringLiteral("gesture/") + key, Wheels.contains(wheel) ? wheel : QStringLiteral("none"));
    bump();
}

QVariantMap Prefs::keyLabels() const
{
    QVariantMap labels;
    const QList<QPair<QString, QString>> keys{{QStringLiteral("shift"), QStringLiteral("⇧")},
                                              {QStringLiteral("ctrl"), QStringLiteral("Ctrl")},
                                              {QStringLiteral("alt"), QStringLiteral("Alt")}};
    for (const auto &k : keys) {
        const QString wheel = wheelForKey(k.first);
        if (!wheel.isEmpty() && !labels.contains(wheel))
            labels.insert(wheel, k.second);
    }
    return labels;
}

QString Prefs::shortcut() const
{
    QSettings s(shortcutsPath(), QSettings::IniFormat);
    for (const QString &group : s.childGroups()) {
        if (s.value(group + QStringLiteral("/Exec")).toString().contains(KeyboardCommand))
            return group;
    }
    // lingmo-core's chotkeys adds it on the next login
    return defaultShortcut();
}

QString Prefs::setShortcut(const QString &sequence)
{
    const QKeySequence seq = QKeySequence::fromString(sequence, QKeySequence::PortableText);
    if (seq.isEmpty() || seq[0].key() == Qt::Key_unknown)
        return tr("That is not a key combination.");
    const QString name = seq.toString(QKeySequence::PortableText);
    QSettings s(shortcutsPath(), QSettings::IniFormat);
    const QStringList groups = s.childGroups();
    if (groups.contains(name)) {
        const QString exec = s.value(name + QStringLiteral("/Exec")).toString();
        if (exec.contains(KeyboardCommand))
            return QString();
        const QString comment = s.value(name + QStringLiteral("/Comment"), exec).toString();
        return tr("%1 is already used by “%2”.").arg(name, comment);
    }
    for (const QString &group : groups) {
        if (s.value(group + QStringLiteral("/Exec")).toString().contains(KeyboardCommand))
            s.remove(group);
    }
    s.beginGroup(name);
    s.setValue(QStringLiteral("Comment"), QStringLiteral("File actions"));
    s.setValue(QStringLiteral("Exec"), KeyboardCommand);
    s.endGroup();
    s.sync();
    bump();
    return s.status() == QSettings::NoError ? QString() : tr("%1 can't be written.").arg(shortcutsPath());
}

QString Prefs::sequenceFor(int key, int modifiers) const
{
    switch (key) {
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Alt:
    case Qt::Key_Meta:
    case Qt::Key_Super_L:
    case Qt::Key_Super_R:
    case Qt::Key_AltGr:
    case Qt::Key_unknown:
    case 0:
        return QString();
    }
    const Qt::KeyboardModifiers mods = Qt::KeyboardModifiers(modifiers)
                                       & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
    return QKeySequence(QKeyCombination(mods, Qt::Key(key))).toString(QKeySequence::PortableText);
}

// ------------------------------------------------------------------ Wheels

int Prefs::openWithCount() const
{
    return qBound(0, conf().value(QStringLiteral("wheels/openWith"), MaxOpenWith).toInt(), MaxOpenWith);
}

void Prefs::setOpenWithCount(int n)
{
    conf().setValue(QStringLiteral("wheels/openWith"), qBound(0, n, MaxOpenWith));
    bump();
}

QStringList Prefs::order(const QString &wheel) const
{
    return conf().value(QStringLiteral("wheels/order_") + wheel).toStringList();
}

void Prefs::reloadCatalog()
{
    m_catalogLoading = true;
    m_catalogError.clear();
    Q_EMIT catalogChanged();
    QProcess *proc = Engine::start({QStringLiteral("catalog"), QStringLiteral("--json")}, this);
    connect(proc, &QProcess::finished, this, [this, proc](int code) {
        proc->deleteLater();
        const QList<QByteArray> lines = proc->readAllStandardOutput().trimmed().split('\n');
        const QJsonObject doc = QJsonDocument::fromJson(lines.isEmpty() ? QByteArray() : lines.last()).object();
        m_catalog.clear();
        for (const QJsonValue &v : doc.value(QStringLiteral("operations")).toArray()) {
            const QJsonObject o = v.toObject();
            const QString id = o.value(QStringLiteral("id")).toString();
            QStringList missing;
            for (const QJsonValue &m : o.value(QStringLiteral("missing")).toArray())
                missing << m.toString();
            m_catalog << QVariantMap{
                {QStringLiteral("kind"), QStringLiteral("op")},
                {QStringLiteral("id"), id},
                {QStringLiteral("wheel"), o.value(QStringLiteral("wheel")).toString()},
                {QStringLiteral("label"), Labels::engine(o.value(QStringLiteral("label")).toString())},
                {QStringLiteral("subtitle"), Labels::engine(o.value(QStringLiteral("subtitle")).toString())},
                {QStringLiteral("available"), o.value(QStringLiteral("available")).toBool()},
                {QStringLiteral("missing"), missing},
                {QStringLiteral("category"), categoryOf(id)},
            };
        }
        m_catalogError = m_catalog.isEmpty() ? (code == 0 ? tr("The conversion engine sent nothing.")
                                                          : tr("lingmo-fileaction-engine is not installed"))
                                             : QString();
        m_catalogLoading = false;
        Q_EMIT catalogChanged();
        bump();
        refreshHelpers();
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        proc->deleteLater();
        m_catalogError = tr("lingmo-fileaction-engine is not installed");
        m_catalogLoading = false;
        Q_EMIT catalogChanged();
    });

    QProcess *version = Engine::start({QStringLiteral("--version")}, this);
    connect(version, &QProcess::finished, this, [this, version] {
        version->deleteLater();
        m_engineVersion = QString::fromUtf8(version->readAllStandardOutput()).trimmed().section(QLatin1Char(' '), -1);
        Q_EMIT catalogChanged();
    });
    connect(version, &QProcess::errorOccurred, version, &QObject::deleteLater);
}

static QString categoryLabel(const QString &id)
{
    if (id == QLatin1String("images")) return Prefs::tr("Images");
    if (id == QLatin1String("video")) return Prefs::tr("Video");
    if (id == QLatin1String("audio")) return Prefs::tr("Audio");
    if (id == QLatin1String("pdf")) return Prefs::tr("PDF");
    if (id == QLatin1String("documents")) return Prefs::tr("Documents");
    if (id == QLatin1String("slides")) return Prefs::tr("Slides");
    if (id == QLatin1String("spreadsheets")) return Prefs::tr("Spreadsheets");
    if (id == QLatin1String("subtitles")) return Prefs::tr("Subtitles");
    if (id == QLatin1String("archives")) return Prefs::tr("Archives");
    if (id == QLatin1String("presets")) return Prefs::tr("Presets");
    if (id == QLatin1String("all")) return Prefs::tr("All files");
    return Prefs::tr("Other");
}

static QString categoryIcon(const QString &id)
{
    if (id == QLatin1String("images")) return QStringLiteral("image-x-generic");
    if (id == QLatin1String("video")) return QStringLiteral("video-x-generic");
    if (id == QLatin1String("audio")) return QStringLiteral("audio-x-generic");
    if (id == QLatin1String("pdf")) return QStringLiteral("application-pdf");
    if (id == QLatin1String("documents")) return QStringLiteral("x-office-document");
    if (id == QLatin1String("slides")) return QStringLiteral("x-office-presentation");
    if (id == QLatin1String("spreadsheets")) return QStringLiteral("x-office-spreadsheet");
    if (id == QLatin1String("subtitles")) return QStringLiteral("text-x-generic");
    if (id == QLatin1String("archives")) return QStringLiteral("package-x-generic");
    return QStringLiteral("text-x-generic");
}

// a sample file type per category, for the "Open with" apps of the preview
static QString sampleMime()
{
    return QStringLiteral("image/png");
}

static QVariantList categorySlices(const QVariantList &catalog, const QString &wheel, const QString &category)
{
    QVariantList list;
    for (const QVariant &v : catalog) {
        const QVariantMap op = v.toMap();
        if (op.value(QStringLiteral("wheel")).toString() == wheel && op.value(QStringLiteral("category")).toString() == category)
            list << op;
    }
    return list;
}

QVariantList Prefs::categories(const QString &wheel) const
{
    QVariantList result;
    if (wheel == QLatin1String("actions")) {
        const QVariantList all = WheelController::baseActionSlices(QString(), 0);
        result << QVariantMap{{QStringLiteral("id"), QStringLiteral("all")}, {QStringLiteral("label"), categoryLabel(QStringLiteral("all"))},
                              {QStringLiteral("icon"), QStringLiteral("folder")},
                              {QStringLiteral("count"), arrange(all, wheel).size() + openWithCount()},
                              {QStringLiteral("total"), all.size() + MaxOpenWith}};
        return result;
    }
    static const QStringList orderOfCategories{QStringLiteral("images"), QStringLiteral("video"), QStringLiteral("audio"),
                                               QStringLiteral("pdf"), QStringLiteral("documents"), QStringLiteral("slides"),
                                               QStringLiteral("spreadsheets"), QStringLiteral("subtitles"),
                                               QStringLiteral("archives"), QStringLiteral("presets"), QStringLiteral("other")};
    for (const QString &category : orderOfCategories) {
        const QVariantList all = categorySlices(m_catalog, wheel, category);
        if (all.isEmpty())
            continue;
        result << QVariantMap{{QStringLiteral("id"), category}, {QStringLiteral("label"), categoryLabel(category)},
                              {QStringLiteral("icon"), categoryIcon(category)},
                              {QStringLiteral("count"), arrange(all, wheel).size()},
                              {QStringLiteral("total"), all.size()}};
    }
    return result;
}

namespace {
struct Ranked {
    QVariantMap slice;
    int group;   // 0 favourite, 1 "Open with" app, 2 the rest
    int rank;
    int index;
};
}

static QList<Ranked> rank(const QVariantList &slices, const QStringList &hidden, const QStringList &favorites,
                          const QStringList &order, bool dropHidden)
{
    QList<Ranked> ranked;
    for (int i = 0; i < slices.size(); ++i) {
        QVariantMap slice = slices.at(i).toMap();
        const QString id = slice.value(QStringLiteral("id")).toString();
        const bool isHidden = hidden.contains(id);
        if (isHidden && dropHidden)
            continue;
        const int fav = favorites.indexOf(id);
        slice.insert(QStringLiteral("hidden"), isHidden);
        slice.insert(QStringLiteral("favorite"), fav >= 0);
        if (fav >= 0) {
            ranked.append({slice, 0, fav, i});
        } else if (id.startsWith(QLatin1String("open.with:"))) {
            ranked.append({slice, 1, 0, i});
        } else {
            const int r = order.indexOf(id);
            ranked.append({slice, 2, r < 0 ? INT_MAX : r, i});
        }
    }
    std::stable_sort(ranked.begin(), ranked.end(), [](const Ranked &a, const Ranked &b) {
        if (a.group != b.group)
            return a.group < b.group;
        if (a.rank != b.rank)
            return a.rank < b.rank;
        return a.index < b.index;
    });
    return ranked;
}

QVariantList Prefs::arrange(const QVariantList &slices, const QString &wheel) const
{
    QVariantList list;
    for (const Ranked &r : rank(slices, engineList(QStringLiteral("HiddenOperations")),
                                engineList(QStringLiteral("FavoriteOperations")), order(wheel), true))
        list << r.slice;
    return list;
}

QVariantList Prefs::sliceList(const QString &wheel, const QString &category) const
{
    const QVariantList all = wheel == QLatin1String("actions") ? WheelController::baseActionSlices(QString(), 0)
                                                               : categorySlices(m_catalog, wheel, category);
    QVariantList list;
    for (const Ranked &r : rank(all, engineList(QStringLiteral("HiddenOperations")),
                                engineList(QStringLiteral("FavoriteOperations")), order(wheel), false))
        list << r.slice;
    return list;
}

QVariantList Prefs::previewSlices(const QString &wheel, const QString &category) const
{
    const QVariantList all = wheel == QLatin1String("actions") ? WheelController::baseActionSlices(sampleMime(), openWithCount())
                                                               : categorySlices(m_catalog, wheel, category);
    return arrange(all, wheel).mid(0, MaxSlices);
}

void Prefs::setHidden(const QString &id, bool hidden)
{
    QStringList list = engineList(QStringLiteral("HiddenOperations"));
    QStringList favs = engineList(QStringLiteral("FavoriteOperations"));
    list.removeAll(id);
    if (hidden)
        list << id;
    QJsonObject doc = engineSettings();
    doc.insert(QStringLiteral("HiddenOperations"), QJsonArray::fromStringList(list));
    if (hidden && favs.removeAll(id)) // a hidden slice is no favourite
        doc.insert(QStringLiteral("FavoriteOperations"), QJsonArray::fromStringList(favs));
    writeEngineSettings(doc);
}

void Prefs::setFavorite(const QString &id, bool favorite)
{
    QStringList favs = engineList(QStringLiteral("FavoriteOperations"));
    QStringList hidden = engineList(QStringLiteral("HiddenOperations"));
    if (favorite == favs.contains(id))
        return;
    favs.removeAll(id);
    if (favorite)
        favs << id; // after the other favourites: they keep their places
    QJsonObject doc = engineSettings();
    doc.insert(QStringLiteral("FavoriteOperations"), QJsonArray::fromStringList(favs));
    if (favorite && hidden.removeAll(id))
        doc.insert(QStringLiteral("HiddenOperations"), QJsonArray::fromStringList(hidden));
    writeEngineSettings(doc);
}

void Prefs::move(const QString &wheel, const QString &category, const QString &id, int delta)
{
    const QVariantList list = sliceList(wheel, category);
    QStringList favIds, restIds;
    for (const QVariant &v : list) {
        const QVariantMap s = v.toMap();
        const QString sid = s.value(QStringLiteral("id")).toString();
        if (s.value(QStringLiteral("favorite")).toBool())
            favIds << sid;
        else if (!sid.startsWith(QLatin1String("open.with:")))
            restIds << sid;
    }
    if (const int i = favIds.indexOf(id); i >= 0) {
        const int j = i + delta;
        if (j < 0 || j >= favIds.size())
            return;
        QStringList favs = engineList(QStringLiteral("FavoriteOperations"));
        favs.swapItemsAt(favs.indexOf(id), favs.indexOf(favIds.at(j)));
        setEngineList(QStringLiteral("FavoriteOperations"), favs);
        return;
    }
    const int i = restIds.indexOf(id);
    const int j = i + delta;
    if (i < 0 || j < 0 || j >= restIds.size())
        return;
    restIds.swapItemsAt(i, j);
    QStringList stored = order(wheel);
    for (const QString &s : restIds)
        stored.removeAll(s);
    conf().setValue(QStringLiteral("wheels/order_") + wheel, restIds + stored);
    bump();
}

void Prefs::resetWheel(const QString &wheel)
{
    QStringList ids;
    if (wheel == QLatin1String("actions")) {
        for (const QVariant &v : WheelController::baseActionSlices(QString(), 0))
            ids << v.toMap().value(QStringLiteral("id")).toString();
        conf().remove(QStringLiteral("wheels/openWith"));
    } else {
        for (const QVariant &v : m_catalog) {
            if (v.toMap().value(QStringLiteral("wheel")).toString() == wheel)
                ids << v.toMap().value(QStringLiteral("id")).toString();
        }
    }
    conf().remove(QStringLiteral("wheels/order_") + wheel);
    QStringList hidden = engineList(QStringLiteral("HiddenOperations"));
    QStringList favs = engineList(QStringLiteral("FavoriteOperations"));
    for (const QString &id : ids) {
        hidden.removeAll(id);
        favs.removeAll(id);
    }
    QJsonObject doc = engineSettings();
    doc.insert(QStringLiteral("HiddenOperations"), QJsonArray::fromStringList(hidden));
    doc.insert(QStringLiteral("FavoriteOperations"), QJsonArray::fromStringList(favs));
    writeEngineSettings(doc); // bumps
}

// ------------------------------------------------------------------ Helper programs

static bool hasProgram(const QStringList &names)
{
    for (const QString &n : names) {
        if (!QStandardPaths::findExecutable(n).isEmpty())
            return true;
    }
    return false;
}

static QStringList tesseractLanguages()
{
    QString dir = qEnvironmentVariable("TESSDATA_PREFIX");
    if (dir.isEmpty() || !QDir(dir).exists())
        dir = QStringLiteral("/usr/share/tessdata");
    QStringList langs;
    for (const QString &f : QDir(dir).entryList({QStringLiteral("*.traineddata")}, QDir::Files, QDir::Name)) {
        const QString code = f.chopped(12);
        if (code != QLatin1String("osd") && code != QLatin1String("equ"))
            langs << code;
    }
    return langs;
}

// Tesseract's name for the system language (por, spa...), or ""
static QString tesseractCodeForLocale()
{
    static const QMap<QString, QString> codes{
        {QStringLiteral("pt"), QStringLiteral("por")}, {QStringLiteral("es"), QStringLiteral("spa")},
        {QStringLiteral("fr"), QStringLiteral("fra")}, {QStringLiteral("de"), QStringLiteral("deu")},
        {QStringLiteral("it"), QStringLiteral("ita")}, {QStringLiteral("cs"), QStringLiteral("ces")},
        {QStringLiteral("nl"), QStringLiteral("nld")}, {QStringLiteral("pl"), QStringLiteral("pol")},
        {QStringLiteral("ru"), QStringLiteral("rus")}, {QStringLiteral("tr"), QStringLiteral("tur")},
    };
    return codes.value(QLocale().name().section(QLatin1Char('_'), 0, 0));
}

void Prefs::refreshHelpers()
{
    // the slices the catalog shows dimmed for want of a program
    auto dimmed = [this](const QString &program) {
        QStringList labels;
        for (const QVariant &v : m_catalog) {
            const QVariantMap op = v.toMap();
            if (op.value(QStringLiteral("missing")).toStringList().contains(program))
                labels << QStringLiteral("%1 (%2)").arg(op.value(QStringLiteral("label")).toString(),
                                                         categoryLabel(op.value(QStringLiteral("category")).toString()));
        }
        return labels;
    };
    const QStringList langs = tesseractLanguages();
    const bool tesseract = hasProgram({QStringLiteral("tesseract")});
    QStringList ocrPackages;
    if (!tesseract)
        ocrPackages << QStringLiteral("tesseract");
    for (const QString &code : {QStringLiteral("eng"), tesseractCodeForLocale()}) {
        if (!code.isEmpty() && !langs.contains(code))
            ocrPackages << QStringLiteral("tesseract-data-") + code;
    }

    m_helpers = {
        QVariantMap{{QStringLiteral("id"), QStringLiteral("libreoffice")},
                    {QStringLiteral("name"), QStringLiteral("LibreOffice")},
                    {QStringLiteral("icon"), QStringLiteral("libreoffice-main")},
                    {QStringLiteral("installed"), hasProgram({QStringLiteral("soffice"), QStringLiteral("libreoffice")})},
                    {QStringLiteral("packages"), QStringList{QStringLiteral("libreoffice-fresh")}},
                    {QStringLiteral("needs"), tr("Word, Excel and PowerPoint files: DOCX, XLSX, PPTX, ODT, ODS and ODP to PDF and back, PDF to PPTX.")},
                    {QStringLiteral("dimmed"), dimmed(QStringLiteral("LibreOffice"))},
                    {QStringLiteral("detail"), QString()}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("pandoc")},
                    {QStringLiteral("name"), QStringLiteral("Pandoc")},
                    {QStringLiteral("icon"), QStringLiteral("x-office-document")},
                    {QStringLiteral("installed"), hasProgram({QStringLiteral("pandoc")})},
                    {QStringLiteral("packages"), QStringList{QStringLiteral("pandoc-cli")}},
                    {QStringLiteral("needs"), tr("Documents: Markdown, HTML, EPUB, RTF, ODT and DOCX to each other.")},
                    {QStringLiteral("dimmed"), dimmed(QStringLiteral("Pandoc"))},
                    {QStringLiteral("detail"), QString()}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("tesseract")},
                    {QStringLiteral("name"), QStringLiteral("Tesseract")},
                    {QStringLiteral("icon"), QStringLiteral("scanner")},
                    {QStringLiteral("installed"), tesseract && ocrPackages.isEmpty()},
                    {QStringLiteral("packages"), ocrPackages},
                    {QStringLiteral("needs"), tr("Extract text (OCR) from pictures and scans.")},
                    {QStringLiteral("dimmed"), tesseract ? QStringList() : QStringList{tr("Extract text (Images)")}},
                    {QStringLiteral("detail"), langs.isEmpty() ? (tesseract ? tr("No reading languages installed") : QString())
                                                               : tr("Languages: %1").arg(langs.join(QStringLiteral(", ")))}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("chromium")},
                    {QStringLiteral("name"), QStringLiteral("Chromium")},
                    {QStringLiteral("icon"), QStringLiteral("chromium")},
                    {QStringLiteral("installed"), hasProgram({QStringLiteral("chromium"), QStringLiteral("chromium-browser"),
                                                              QStringLiteral("google-chrome"), QStringLiteral("google-chrome-stable"),
                                                              QStringLiteral("microsoft-edge")})},
                    {QStringLiteral("packages"), QStringList{QStringLiteral("chromium")}},
                    {QStringLiteral("needs"), tr("Markdown and HTML to PDF, with the page as a browser prints it.")},
                    {QStringLiteral("dimmed"), QStringList()},
                    {QStringLiteral("detail"), QString()}},
    };
    Q_EMIT helpersChanged();
}

void Prefs::install(const QString &helperId)
{
    if (m_installer)
        return;
    QStringList packages;
    QString name;
    for (const QVariant &v : m_helpers) {
        const QVariantMap h = v.toMap();
        if (h.value(QStringLiteral("id")).toString() == helperId) {
            packages = h.value(QStringLiteral("packages")).toStringList();
            name = h.value(QStringLiteral("name")).toString();
        }
    }
    if (packages.isEmpty())
        return;
    // the system's privileged path, like Lingmo Settings: the Store's root helper (one pacman
    // transaction, its lock checks), else pacman itself; pkexec asks for the password
    const QString pkexec = qEnvironmentVariable("LINGMO_FILEACTION_PKEXEC", QStringLiteral("pkexec"));
    const QString helper = QStringLiteral("/usr/lib/lingmo-store/lingmo-store-helper");
    QStringList args = QFileInfo(helper).isExecutable()
                           ? QStringList{helper, QStringLiteral("install")}
                           : QStringList{QStringLiteral("pacman"), QStringLiteral("-S"), QStringLiteral("--needed"), QStringLiteral("--noconfirm")};
    args << packages;

    m_installer = new QProcess(this);
    m_installer->setProcessChannelMode(QProcess::MergedChannels);
    m_installing = helperId;
    m_installMessage = tr("Waiting for authorization…");
    m_installOk = false;
    Q_EMIT helpersChanged();
    QProcess *proc = m_installer;
    connect(proc, &QProcess::readyRead, this, [this, proc] {
        while (proc->canReadLine()) {
            const QString line = QString::fromUtf8(proc->readLine()).trimmed();
            if (line.contains(QLatin1String("downloading")))
                m_installMessage = tr("Downloading…");
            else if (line.startsWith(QLatin1String("@@STEP")) || line.contains(QLatin1String("installing")))
                m_installMessage = tr("Installing…");
            else
                continue;
            Q_EMIT helpersChanged();
        }
    });
    connect(proc, &QProcess::finished, this, [this, proc, name](int code, QProcess::ExitStatus status) {
        const QByteArray rest = proc->readAll();
        proc->deleteLater();
        m_installing.clear();
        m_installOk = status == QProcess::NormalExit && code == 0;
        if (m_installOk)
            m_installMessage = tr("%1 is installed.").arg(name);
        else if (code == 126 || code == 127)
            m_installMessage = tr("The installation was cancelled.");
        else if (rest.contains("@@ERROR lock"))
            m_installMessage = tr("Another installation is running. Try again when it finishes.");
        else
            m_installMessage = tr("%1 could not be installed. Check the internet connection.").arg(name);
        Q_EMIT helpersChanged();
        reloadCatalog(); // the dimmed slices come alive (and refreshHelpers)
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        proc->deleteLater();
        m_installing.clear();
        m_installOk = false;
        m_installMessage = tr("Could not start pkexec. Make sure polkit is installed.");
        Q_EMIT helpersChanged();
    });
    proc->start(pkexec, args);
}

void Prefs::openUrl(const QString &url) const
{
    QDesktopServices::openUrl(QUrl(url));
}

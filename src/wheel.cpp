/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "wheel.h"
#include "engine.h"
#include "gesture.h"
#include "jobs.h"
#include "labels.h"
#include "prefs.h"

#include <KApplicationTrader>
#include <KIO/ApplicationLauncherJob>
#include <KService>
#include <QMimeDatabase>
#include <QClipboard>
#include <QCursor>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QJsonArray>
#include <QMimeData>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcWheel, "lingmo.fileaction.wheel", QtInfoMsg)

static const QStringList wheelOrder{QStringLiteral("convert"), QStringLiteral("tools"), QStringLiteral("actions")};
static constexpr int MaxSlices = Prefs::MaxSlices;

static QSettings settings()
{
    return QSettings(QStringLiteral("lingmoos"), QStringLiteral("fileaction"));
}

WheelController::WheelController(JobModel *jobs, DragGesture *gesture, QObject *parent)
    : QObject(parent)
    , m_jobs(jobs)
    , m_gesture(gesture)
{
    // a change in Settings shows at once on an open wheel
    connect(Prefs::instance(), &Prefs::changed, this, &WheelController::slicesChanged);
    if (m_gesture) {
        connect(m_gesture, &DragGesture::dragFiles, this, [this](const QStringList &paths) {
            if (m_visible && m_dragMode && m_files.isEmpty() && !paths.isEmpty()) {
                m_files = paths;
                Q_EMIT filesChanged();
                load();
            }
        });
        connect(m_gesture, &DragGesture::keyDuringDrag, this, [this](const QString &key) {
            if (m_visible && m_dragMode)
                Q_EMIT keyDuringDrag(key);
        });
    }
}

QString WheelController::summary() const
{
    if (m_files.isEmpty())
        return m_dragMode ? QString() : tr("Drop files here");
    if (m_files.size() == 1)
        return QFileInfo(m_files.first()).fileName();
    return tr("%n files", nullptr, m_files.size());
}

void WheelController::open(const QStringList &files, const QString &wheel, bool dragMode)
{
    qCDebug(lcWheel) << "open" << wheel << "drag" << dragMode << files;
    m_files = files;
    m_wheel = wheelOrder.contains(wheel) ? wheel : QStringLiteral("convert");
    m_repeat = false;
    m_dragMode = dragMode;
    m_anchor = QCursor::pos();
    m_visible = true;
    m_error.clear();
    Q_EMIT filesChanged();
    Q_EMIT visibleChanged();
    load();
    if (dragMode && files.isEmpty() && m_gesture)
        m_gesture->requestDragFiles();
}

void WheelController::openKeyboard()
{
    QStringList files;
    // 1. the selection of the Lingmo Files window in front
    if (m_gesture && m_gesture->activeWindowClass().contains(QLatin1String("lingmo-filemanager"))) {
        QDBusMessage call = QDBusMessage::createMethodCall(QStringLiteral("com.lingmo.FileManager"),
                                                           QStringLiteral("/FileManager"),
                                                           QStringLiteral("com.lingmo.FileManager"),
                                                           QStringLiteral("selectedFiles"));
        const QDBusMessage reply = QDBusConnection::sessionBus().call(call, QDBus::Block, 1500);
        if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty())
            files = reply.arguments().first().toStringList();
    }
    // 2. files, a picture or paths on the clipboard
    if (files.isEmpty()) {
        const QMimeData *mime = QGuiApplication::clipboard()->mimeData();
        if (mime && mime->hasUrls()) {
            for (const QUrl &url : mime->urls()) {
                if (url.isLocalFile() && QFileInfo::exists(url.toLocalFile()))
                    files << url.toLocalFile();
            }
        }
        if (files.isEmpty() && mime && mime->hasImage()) {
            // a screenshot: saved first (the folder name stays English on disk), the results go next to it
            const QImage image = qvariant_cast<QImage>(mime->imageData());
            const QString dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + QStringLiteral("/FileAction");
            QDir().mkpath(dir);
            const QString path = dir + QLatin1Char('/') + tr("Clipboard") + QLatin1Char(' ')
                                 + QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH.mm.ss")) + QStringLiteral(".png");
            if (!image.isNull() && image.save(path))
                files << path;
        }
        if (files.isEmpty() && mime && mime->hasText()) {
            for (const QString &line : mime->text().split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
                const QString p = line.trimmed();
                if (p.startsWith(QLatin1Char('/')) && QFileInfo::exists(p))
                    files << p;
            }
        }
    }
    open(files, QStringLiteral("convert"), false);
}

void WheelController::setDroppedFiles(const QVariantList &urls)
{
    QStringList paths;
    for (const QVariant &v : urls) {
        const QUrl url = v.toUrl();
        if (url.isLocalFile())
            paths << url.toLocalFile();
    }
    if (paths.isEmpty() || paths == m_files)
        return;
    m_files = paths;
    Q_EMIT filesChanged();
    load();
}

void WheelController::load()
{
    const int serial = ++m_loadSerial;
    m_error.clear();
    if (m_files.isEmpty()) {
        m_listing = QJsonObject();
        m_loading = false;
        Q_EMIT slicesChanged();
        return;
    }
    m_loading = true;
    Q_EMIT slicesChanged();
    Engine::list(m_files, this, [this, serial](const QJsonObject &doc, const QString &error) {
        if (serial != m_loadSerial)
            return;
        m_listing = doc;
        m_error = Labels::message(error);
        m_loading = false;
        Q_EMIT slicesChanged();
    });
}

void WheelController::switchWheel(const QString &wheel)
{
    qCDebug(lcWheel) << "switch to" << wheel << "from" << m_wheel << "repeat" << m_repeat;
    if (!wheelOrder.contains(wheel))
        return;
    if (wheel == m_wheel && !m_repeat) {
        // the same key again: repeat the last action
        repeat();
        return;
    }
    m_wheel = wheel;
    m_repeat = false;
    Q_EMIT slicesChanged();
}

void WheelController::cycleWheel(int delta)
{
    const int i = (wheelOrder.indexOf(m_wheel) + delta + wheelOrder.size()) % wheelOrder.size();
    m_wheel = wheelOrder.at(i);
    m_repeat = false;
    Q_EMIT slicesChanged();
}

void WheelController::repeat()
{
    qCDebug(lcWheel) << "repeat; can" << canRepeat();
    if (!canRepeat())
        return;
    m_repeat = true;
    Q_EMIT slicesChanged();
}

bool WheelController::canRepeat() const
{
    QSettings s = settings();
    const QString id = s.value(QStringLiteral("last/id")).toString();
    if (id.isEmpty())
        return false;
    // the files or their list are still coming: offer it (the engine says if it doesn't apply)
    if (s.value(QStringLiteral("last/wheel")).toString() == QLatin1String("actions") || m_files.isEmpty() || m_loading
        || m_listing.isEmpty())
        return true;
    for (const QString &w : {QStringLiteral("convert"), QStringLiteral("tools")}) {
        for (const QJsonValue &v : m_listing.value(QStringLiteral("wheels")).toObject().value(w).toArray()) {
            const QJsonObject o = v.toObject();
            if (o.value(QStringLiteral("id")).toString() == id)
                return o.value(QStringLiteral("available")).toBool();
        }
    }
    return false;
}

QString WheelController::lastLabel() const
{
    return settings().value(QStringLiteral("last/label")).toString();
}

QVariantMap WheelController::repeatSlice() const
{
    QSettings s = settings();
    return {
        {QStringLiteral("kind"), QStringLiteral("repeat")},
        {QStringLiteral("id"), s.value(QStringLiteral("last/id"))},
        {QStringLiteral("label"), tr("Repeat")},
        {QStringLiteral("subtitle"), s.value(QStringLiteral("last/label"))},
        {QStringLiteral("available"), true},
        {QStringLiteral("icon"), QStringLiteral("view-refresh")},
    };
}

QVariantList WheelController::baseActionSlices(const QString &mime, int openWith)
{
    auto action = [](const QString &id, const QString &label, const QString &icon) {
        return QVariantMap{{QStringLiteral("kind"), QStringLiteral("action")}, {QStringLiteral("id"), id},
                           {QStringLiteral("label"), label}, {QStringLiteral("icon"), icon},
                           {QStringLiteral("available"), true}, {QStringLiteral("wheel"), QStringLiteral("actions")}};
    };
    QVariantList list;
    // "Open with": the apps for this file type, the user's default first (Lingmo's
    // mimeapps.list), like the file manager's Open With menu
    if (!mime.isEmpty()) {
        int added = 0;
        for (const KService::Ptr &app : KApplicationTrader::queryByMimeType(mime)) {
            if (added >= openWith)
                break;
            list << QVariantMap{{QStringLiteral("kind"), QStringLiteral("action")},
                                {QStringLiteral("id"), QStringLiteral("open.with:") + app->storageId()},
                                {QStringLiteral("label"), app->name()},
                                {QStringLiteral("subtitle"), tr("Open with")},
                                {QStringLiteral("icon"), app->icon()},
                                {QStringLiteral("available"), true},
                                {QStringLiteral("wheel"), QStringLiteral("actions")}};
            ++added;
        }
    }
    list << QVariantList{
        action(QStringLiteral("copy.to"), tr("Copy to…"), QStringLiteral("edit-copy")),
        action(QStringLiteral("move.to"), tr("Move to…"), QStringLiteral("go-next")),
        action(QStringLiteral("copy.path"), tr("Copy path"), QStringLiteral("edit-paste")),
        action(QStringLiteral("terminal"), tr("Terminal here"), QStringLiteral("utilities-terminal")),
    };
    if (!QStandardPaths::findExecutable(QStringLiteral("lingmo-share")).isEmpty())
        list << action(QStringLiteral("share"), tr("Share"), QStringLiteral("document-send"));
    return list;
}

QVariantList WheelController::actionSlices() const
{
    const QString mime = m_files.isEmpty() ? QString() : QMimeDatabase().mimeTypeForFile(m_files.first()).name();
    Prefs *prefs = Prefs::instance();
    return prefs->arrange(baseActionSlices(mime, prefs->openWithCount()), QStringLiteral("actions")).mid(0, MaxSlices);
}

QVariantList WheelController::opSlices(const QString &wheel) const
{
    QVariantList list;
    QSettings s = settings();
    for (const QJsonValue &v : m_listing.value(QStringLiteral("wheels")).toObject().value(wheel).toArray()) {
        const QJsonObject o = v.toObject();
        const QString id = o.value(QStringLiteral("id")).toString();
        QVariantList params;
        const QVariantMap last = s.value(QStringLiteral("params/") + id).toMap();
        for (const QJsonValue &p : o.value(QStringLiteral("parameters")).toArray()) {
            const QJsonObject po = p.toObject();
            const QString key = po.value(QStringLiteral("key")).toString();
            params << QVariantMap{
                {QStringLiteral("key"), key},
                {QStringLiteral("label"), Labels::engine(po.value(QStringLiteral("label")).toString())},
                {QStringLiteral("value"), last.value(key, po.value(QStringLiteral("default")).toString())},
                {QStringLiteral("hint"), Labels::engine(po.value(QStringLiteral("hint")).toString())},
            };
        }
        QStringList missing;
        for (const QJsonValue &m : o.value(QStringLiteral("missing")).toArray())
            missing << m.toString();
        const bool available = o.value(QStringLiteral("available")).toBool();
        QString subtitle = Labels::engine(o.value(QStringLiteral("subtitle")).toString());
        if (!available)
            subtitle = tr("needs %1").arg(missing.join(QStringLiteral(", ")));
        list << QVariantMap{
            {QStringLiteral("kind"), QStringLiteral("op")},
            {QStringLiteral("id"), id},
            {QStringLiteral("wheel"), wheel},
            {QStringLiteral("label"), Labels::engine(o.value(QStringLiteral("label")).toString())},
            {QStringLiteral("subtitle"), subtitle},
            {QStringLiteral("available"), available},
            {QStringLiteral("missing"), missing},
            {QStringLiteral("parameters"), params},
        };
    }
    // the user's choices (Settings › Wheels): hidden slices out, favourites first
    return Prefs::instance()->arrange(list, wheel).mid(0, MaxSlices);
}

QVariantList WheelController::slices() const
{
    if (m_repeat)
        return {repeatSlice()};
    if (m_wheel == QLatin1String("actions"))
        return actionSlices();
    return opSlices(m_wheel);
}

QVariantMap WheelController::lastParameters(const QString &id) const
{
    return settings().value(QStringLiteral("params/") + id).toMap();
}

bool WheelController::activate(int index, const QVariantList &urls)
{
    const QVariantList list = slices();
    if (index < 0 || index >= list.size())
        return false;
    if (!urls.isEmpty()) {
        QStringList paths;
        for (const QVariant &v : urls) {
            const QUrl url = v.toUrl();
            if (url.isLocalFile())
                paths << url.toLocalFile();
        }
        if (!paths.isEmpty() && paths != m_files) {
            m_files = paths;
            Q_EMIT filesChanged();
        }
    }
    if (m_files.isEmpty())
        return false;
    const QVariantMap slice = list.at(index).toMap();
    const QString kind = slice.value(QStringLiteral("kind")).toString();
    const QString id = slice.value(QStringLiteral("id")).toString();

    if (kind == QLatin1String("op") && !slice.value(QStringLiteral("available")).toBool()) {
        const QString missing = slice.value(QStringLiteral("missing")).toStringList().join(QStringLiteral(", "));
        m_jobs->addFinished(slice.value(QStringLiteral("label")).toString(), m_files, {},
                            tr("%1 is not installed. Install it in the Store or with pacman.").arg(missing));
        close();
        return true;
    }
    if (kind == QLatin1String("op") && !slice.value(QStringLiteral("parameters")).toList().isEmpty()) {
        close();
        Q_EMIT parametersRequested(index, slice);
        return false;
    }
    if (kind == QLatin1String("action") && (id == QLatin1String("copy.to") || id == QLatin1String("move.to"))) {
        close();
        Q_EMIT folderRequested(id, id == QLatin1String("move.to"));
        return false;
    }
    runSlice(slice, m_files, {});
    close();
    return true;
}

void WheelController::runWithParameters(int index, const QVariantMap &params)
{
    const QVariantList list = slices();
    if (index < 0 || index >= list.size() || m_files.isEmpty())
        return;
    const QVariantMap slice = list.at(index).toMap();
    settings().setValue(QStringLiteral("params/") + slice.value(QStringLiteral("id")).toString(), params);
    runSlice(slice, m_files, params);
}

void WheelController::runWithFolder(const QString &actionId, const QUrl &folder)
{
    if (m_files.isEmpty() || !folder.isLocalFile())
        return;
    const QVariantMap params{{QStringLiteral("folder"), folder.toLocalFile()}};
    const QVariantMap slice{{QStringLiteral("kind"), QStringLiteral("action")}, {QStringLiteral("id"), actionId},
                            {QStringLiteral("wheel"), QStringLiteral("actions")},
                            {QStringLiteral("label"), actionId == QLatin1String("move.to") ? tr("Move to %1").arg(folder.fileName())
                                                                                          : tr("Copy to %1").arg(folder.fileName())}};
    runSlice(slice, m_files, params);
}

void WheelController::remember(const QVariantMap &slice, const QVariantMap &params)
{
    QSettings s = settings();
    s.setValue(QStringLiteral("last/id"), slice.value(QStringLiteral("id")));
    s.setValue(QStringLiteral("last/wheel"), slice.value(QStringLiteral("wheel")));
    QString label = slice.value(QStringLiteral("label")).toString();
    if (slice.value(QStringLiteral("wheel")).toString() == QLatin1String("convert"))
        label = tr("Convert to %1").arg(label);
    s.setValue(QStringLiteral("last/label"), label);
    s.setValue(QStringLiteral("last/params"), params);
}

void WheelController::runSlice(const QVariantMap &slice, const QStringList &files, const QVariantMap &params)
{
    const QString kind = slice.value(QStringLiteral("kind")).toString();
    if (kind == QLatin1String("repeat")) {
        QSettings s = settings();
        const QString id = s.value(QStringLiteral("last/id")).toString();
        const QString wheel = s.value(QStringLiteral("last/wheel")).toString();
        const QVariantMap lastParams = s.value(QStringLiteral("last/params")).toMap();
        if (wheel == QLatin1String("actions"))
            runAction(id, files, lastParams);
        else
            m_jobs->start(id, s.value(QStringLiteral("last/label")).toString(), QString(), files, lastParams);
        return;
    }
    remember(slice, params);
    if (kind == QLatin1String("action"))
        runAction(slice.value(QStringLiteral("id")).toString(), files, params);
    else
        m_jobs->start(slice.value(QStringLiteral("id")).toString(), slice.value(QStringLiteral("label")).toString(),
                      slice.value(QStringLiteral("wheel")).toString(), files, params);
}

static QString terminalProgram()
{
    // the terminal chosen in Settings › Default apps, like Lingmo Files does
    const QString chosen = QSettings(QStringLiteral("lingmoos"), QStringLiteral("defaultApps")).value(QStringLiteral("terminal")).toString();
    QStringList candidates;
    if (!chosen.isEmpty())
        candidates << QString(chosen).remove(QStringLiteral(".desktop"));
    candidates << QStringLiteral("lingmo-terminal") << QStringLiteral("konsole") << QStringLiteral("xterm");
    for (const QString &c : candidates) {
        if (!QStandardPaths::findExecutable(c).isEmpty())
            return c;
    }
    return QString();
}

void WheelController::runAction(const QString &id, const QStringList &files, const QVariantMap &params)
{
    if (id.startsWith(QLatin1String("open.with:"))) {
        const KService::Ptr app = KService::serviceByStorageId(id.mid(10));
        if (!app) {
            m_jobs->addFinished(tr("Open with"), files, {}, tr("That app is no longer installed."));
            return;
        }
        auto *job = new KIO::ApplicationLauncherJob(app);
        QList<QUrl> urls;
        for (const QString &f : files)
            urls << QUrl::fromLocalFile(f);
        job->setUrls(urls);
        job->start();
        return;
    }
    if (id == QLatin1String("copy.path")) {
        QGuiApplication::clipboard()->setText(files.join(QLatin1Char('\n')));
        m_jobs->addFinished(files.size() == 1 ? tr("Path copied") : tr("Paths copied"), files, {}, QString());
    } else if (id == QLatin1String("terminal")) {
        const QFileInfo first(files.first());
        const QString dir = first.isDir() ? first.absoluteFilePath() : first.absolutePath();
        const QString program = terminalProgram();
        if (program.isEmpty() || !QProcess::startDetached(program, {}, dir))
            m_jobs->addFinished(tr("Terminal here"), files, {}, tr("No terminal app was found."));
    } else if (id == QLatin1String("share")) {
        QProcess::startDetached(QStringLiteral("lingmo-share"), QStringList{QStringLiteral("send"), QStringLiteral("--pick")} + files);
    } else if (id == QLatin1String("copy.to") || id == QLatin1String("move.to")) {
        const QString folder = params.value(QStringLiteral("folder")).toString();
        if (!folder.isEmpty())
            transfer(files, folder, id == QLatin1String("move.to"));
    }
}

// "name (2).ext" when name.ext exists (never overwrites, like the engine)
static QString freePath(const QString &dir, const QString &name)
{
    QString base = name, ext;
    const int dot = name.lastIndexOf(QLatin1Char('.'));
    if (dot > 0) {
        base = name.left(dot);
        ext = name.mid(dot);
        if (base.endsWith(QLatin1String(".tar"))) {
            base.chop(4);
            ext.prepend(QStringLiteral(".tar"));
        }
    }
    QString candidate = QDir(dir).filePath(name);
    for (int i = 2; QFileInfo::exists(candidate); ++i)
        candidate = QDir(dir).filePath(QStringLiteral("%1 (%2)%3").arg(base).arg(i).arg(ext));
    return candidate;
}

static bool copyRecursively(const QString &from, const QString &to, QString *error)
{
    const QFileInfo info(from);
    if (info.isDir() && !info.isSymLink()) {
        if (!QDir().mkpath(to)) {
            *error = WheelController::tr("Could not create %1").arg(to);
            return false;
        }
        const QDir dir(from);
        for (const QString &entry : dir.entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot)) {
            if (!copyRecursively(dir.filePath(entry), QDir(to).filePath(entry), error))
                return false;
        }
        return true;
    }
    if (!QFile::copy(from, to)) {
        *error = QFile(from).errorString();
        return false;
    }
    return true;
}

void WheelController::transfer(const QStringList &files, const QString &folder, bool move)
{
    auto *thread = QThread::create([this, files, folder, move] {
        QStringList outputs;
        QString error;
        for (const QString &file : files) {
            const QFileInfo src(file);
            if (move && src.absolutePath() == QDir(folder).absolutePath())
                continue; // already there
            const QString dest = freePath(folder, src.fileName());
            if (move && QFile::rename(file, dest)) {
                outputs << dest;
                continue;
            }
            if (!copyRecursively(file, dest, &error))
                break;
            outputs << dest;
            if (move) {
                // other file system: the copy is complete, now the original goes
                const bool removed = src.isDir() ? QDir(file).removeRecursively() : QFile::remove(file);
                if (!removed) {
                    error = tr("Copied, but %1 could not be removed.").arg(src.fileName());
                    break;
                }
            }
        }
        QMetaObject::invokeMethod(this, [this, files, folder, move, outputs, error] {
            const QString name = QFileInfo(folder).fileName();
            m_jobs->addFinished(move ? tr("Moved to %1").arg(name) : tr("Copied to %1").arg(name), files, outputs, error);
        });
    });
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void WheelController::shown()
{
    if (m_dragMode && m_files.isEmpty() && m_gesture)
        m_gesture->nudgePointer();
}

void WheelController::close()
{
    qCDebug(lcWheel) << "close; visible" << m_visible;
    if (!m_visible)
        return;
    m_visible = false;
    Q_EMIT visibleChanged();
}

void WheelController::openSettings()
{
    close();
    Q_EMIT settingsRequested();
}

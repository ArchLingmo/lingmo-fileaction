/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Ações de arquivo (lingmo-fileaction): convert files by dropping them on a
// radial wheel. Based on FileAction (MIT) by Metamorfolog, inspired by Tangerine.
//
//   lingmo-fileaction                          the wheel as a drop zone at the pointer
//   lingmo-fileaction FILE...                  the wheel for these files
//   lingmo-fileaction --wheel tools FILE...    convert | tools | actions
//   lingmo-fileaction --keyboard               files selected in Lingmo Files, or on the clipboard (Meta+Shift+A)
//   lingmo-fileaction --daemon                 session daemon (XDG autostart): drag a file, tap Shift/Ctrl/Alt
//   lingmo-fileaction --settings               the settings window
//   lingmo-fileaction --about
//
// One instance per session (com.lingmo.FileAction): a second launch hands its
// request over and exits. Conversions run in lingmo-fileaction-engine.

#include "gesture.h"
#include "jobs.h"
#include "prefs.h"
#include "wheel.h"
#ifdef HAVE_LINGMO_INTENTS
#include "intents.h"
#endif

#include <QDBusConnection>
#include <QDBusInterface>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include <QTranslator>
#include <QUrl>

static const QString DBusService = QStringLiteral("com.lingmo.FileAction");
static const QString DBusPath = QStringLiteral("/FileAction");

class FileActionService : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.lingmo.FileAction")
public:
    FileActionService(WheelController *wheel, QObject *root, QObject *parent)
        : QObject(parent), m_wheel(wheel), m_root(root) {}

public Q_SLOTS:
    void Open(const QStringList &files, const QString &wheel) { m_wheel->open(files, wheel, false); }
    void OpenKeyboard() { m_wheel->openKeyboard(); }
    void About() { QMetaObject::invokeMethod(m_root, "showAbout"); }
    void Settings() { QMetaObject::invokeMethod(m_root, "showSettings"); }

private:
    WheelController *m_wheel;
    QObject *m_root;
};

static void installTranslator(QCoreApplication &app, QTranslator &translator)
{
    for (const QString &dir : {app.applicationDirPath() + QStringLiteral("/../share/lingmo-fileaction/translations"),
                               app.applicationDirPath(), QStringLiteral(TRANSLATIONS_DIR)}) {
        if (translator.load(QLocale(), QStringLiteral("lingmo-fileaction"), QStringLiteral("_"), dir)) {
            app.installTranslator(&translator);
            break;
        }
    }
}

static QStringList fileArguments(const QStringList &args)
{
    QStringList files;
    for (int i = 1; i < args.size(); ++i) {
        const QString &a = args.at(i);
        if (a == QLatin1String("--wheel")) {
            ++i;
            continue;
        }
        if (a.startsWith(QLatin1String("--")))
            continue;
        const QString path = a.startsWith(QLatin1String("file://")) ? QUrl(a).toLocalFile() : QFileInfo(a).absoluteFilePath();
        if (QFileInfo::exists(path))
            files << path;
    }
    return files;
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("lingmoos"));
    app.setApplicationName(QStringLiteral("lingmo-fileaction"));
    app.setApplicationVersion(QStringLiteral(APP_VERSION));
    app.setDesktopFileName(QStringLiteral("lingmo-fileaction"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("lingmo-fileaction"), QIcon(QStringLiteral(":/icons/lingmo-fileaction.svg"))));
    app.setQuitOnLastWindowClosed(false);
    QTranslator translator;
    installTranslator(app, translator);
    app.setApplicationDisplayName(QCoreApplication::translate("main", "File actions"));

    const QStringList args = app.arguments();
    const bool daemon = args.contains(QStringLiteral("--daemon"));
    const bool keyboard = args.contains(QStringLiteral("--keyboard"));
    const bool about = args.contains(QStringLiteral("--about"));
    QString wheelArg = QStringLiteral("convert");
    if (int i = args.indexOf(QStringLiteral("--wheel")); i > 0 && i + 1 < args.size())
        wheelArg = args.at(i + 1);
    const QStringList files = fileArguments(args);
    // Opened from the menu or Spotlight with no files: the settings, not an empty wheel
    const bool settingsWindow = args.contains(QStringLiteral("--settings"))
            || (files.isEmpty() && !daemon && !keyboard && !about && !args.contains(QStringLiteral("--wheel")));

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (bus.isConnected() && !bus.registerService(DBusService)) {
        if (daemon)
            return 0; // already running
        QDBusInterface iface(DBusService, DBusPath, DBusService, bus);
        if (keyboard)
            iface.call(QStringLiteral("OpenKeyboard"));
        else if (about)
            iface.call(QStringLiteral("About"));
        else if (settingsWindow)
            iface.call(QStringLiteral("Settings"));
        else
            iface.call(QStringLiteral("Open"), files, wheelArg);
        return 0;
    }

    // The gesture needs X11; elsewhere the wheel still works from the menu, the shortcut and the file manager
    DragGesture *gesture = nullptr;
    if (daemon && QGuiApplication::platformName() == QLatin1String("xcb")) {
        gesture = new DragGesture(&app);
        if (!gesture->isValid()) {
            delete gesture;
            gesture = nullptr;
        }
    }

    Prefs *prefs = Prefs::instance();
    JobModel jobs;
    WheelController wheel(&jobs, gesture);
    // After finishing (Settings › General): open the folder with the results
    QObject::connect(&jobs, &JobModel::jobFinished, prefs, [prefs](int, bool ok, const QStringList &outputs) {
        if (ok && !outputs.isEmpty() && prefs->openFolder())
            JobModel::revealFiles(outputs);
    });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("fa"), &wheel);
    engine.rootContext()->setContextProperty(QStringLiteral("jobs"), &jobs);
    engine.rootContext()->setContextProperty(QStringLiteral("prefs"), prefs);
    engine.rootContext()->setContextProperty(QStringLiteral("appVersion"), QStringLiteral(APP_VERSION));
    engine.load(QUrl(QStringLiteral("qrc:/qml/Shell.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;
    QObject *root = engine.rootObjects().first();

    QObject::connect(&wheel, &WheelController::settingsRequested, root, [root] {
        QMetaObject::invokeMethod(root, "showSettings");
    });

    if (gesture) {
        gesture->setTapMs(prefs->tapMs());
        QObject::connect(prefs, &Prefs::changed, gesture, [gesture, prefs] { gesture->setTapMs(prefs->tapMs()); });
        QObject::connect(gesture, &DragGesture::tapped, &wheel, [&wheel, prefs](const QString &key) {
            // Settings › Gestures: the drag gesture can be off, and each key opens the wheel chosen there
            const QString kind = prefs->wheelForKey(key);
            if (!prefs->gestureEnabled() || kind.isEmpty())
                return;
            if (wheel.visible() && wheel.dragMode())
                wheel.switchWheel(kind); // another key switches the wheel, the same one repeats
            else
                wheel.open({}, kind, true);
        });
        QObject::connect(gesture, &DragGesture::dragEnded, &wheel, [&wheel] {
            // dropped somewhere else (or Esc): the wheel goes; a drop on it was handled already
            QTimer::singleShot(400, &wheel, [&wheel] {
                if (wheel.dragMode())
                    wheel.close();
            });
        });
    }

    FileActionService service(&wheel, root, &app);
    if (bus.isConnected())
        bus.registerObject(DBusPath, &service, QDBusConnection::ExportAllSlots);
#ifdef HAVE_LINGMO_INTENTS
    if (bus.isConnected())
        publishFileActionIntents(&jobs, &app);
#endif

    if (!daemon) {
        if (keyboard)
            wheel.openKeyboard();
        else if (about)
            QMetaObject::invokeMethod(root, "showAbout");
        else if (settingsWindow)
            QMetaObject::invokeMethod(root, "showSettings");
        else
            wheel.open(files, wheelArg, false);
        // A one-off instance leaves when nothing is left: wheel closed, jobs gone, dialogs closed
        auto *idle = new QTimer(&app);
        idle->setInterval(2000);
        QObject::connect(idle, &QTimer::timeout, &app, [&wheel, &jobs, root] {
            if (!wheel.visible() && jobs.rowCount() == 0 && !root->property("busy").toBool())
                QCoreApplication::quit();
        });
        idle->start();
    }
    return app.exec();
}

#include "main.moc"

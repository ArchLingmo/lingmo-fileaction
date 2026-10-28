/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "engine.h"

#include <QJsonDocument>
#include <QPointer>

namespace Engine {

static QStringList command()
{
    const QString custom = qEnvironmentVariable("LINGMO_FILEACTION_ENGINE");
    QStringList cmd = custom.isEmpty() ? QStringList{QStringLiteral(ENGINE_COMMAND)} : QProcess::splitCommand(custom);
    if (cmd.isEmpty())
        cmd << QStringLiteral(ENGINE_COMMAND);
    return cmd;
}

QString program()
{
    return command().first();
}

QStringList arguments(const QStringList &args)
{
    return command().mid(1) + args;
}

QProcessEnvironment environment()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("LC_ALL"), QStringLiteral("C.UTF-8"));
    env.insert(QStringLiteral("DOTNET_CLI_TELEMETRY_OPTOUT"), QStringLiteral("1"));
    return env;
}

QProcess *start(const QStringList &args, QObject *parent)
{
    auto *proc = new QProcess(parent);
    proc->setProcessEnvironment(environment());
    proc->setProgram(program());
    proc->setArguments(arguments(args));
    proc->start();
    return proc;
}

void list(const QStringList &files, QObject *receiver, std::function<void(const QJsonObject &, const QString &)> done)
{
    QProcess *proc = start(QStringList{QStringLiteral("list"), QStringLiteral("--json")} + files, receiver);
    QPointer<QObject> guard(receiver);
    QObject::connect(proc, &QProcess::finished, receiver, [proc, done, guard](int code, QProcess::ExitStatus status) {
        proc->deleteLater();
        if (!guard)
            return;
        const QByteArray out = proc->readAllStandardOutput();
        // the last line is the document (an error line looks the same)
        const QList<QByteArray> lines = out.trimmed().split('\n');
        const QJsonObject doc = QJsonDocument::fromJson(lines.isEmpty() ? QByteArray() : lines.last()).object();
        if (status != QProcess::NormalExit || code != 0 || doc.isEmpty() || doc.contains(QStringLiteral("event"))) {
            QString message = doc.value(QStringLiteral("message")).toString();
            if (message.isEmpty())
                message = QString::fromUtf8(proc->readAllStandardError()).trimmed();
            done(QJsonObject(), message.isEmpty() ? QStringLiteral("engine exited with %1").arg(code) : message);
            return;
        }
        done(doc, QString());
    });
    QObject::connect(proc, &QProcess::errorOccurred, receiver, [proc, done, guard](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        proc->deleteLater();
        if (guard)
            done(QJsonObject(), QStringLiteral("lingmo-fileaction-engine is not installed"));
    });
}

}

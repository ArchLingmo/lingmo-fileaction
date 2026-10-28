/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "jobs.h"
#include "engine.h"
#include "labels.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDesktopServices>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUrl>

#include <signal.h>

JobModel::JobModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int JobModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_jobs.size();
}

QHash<int, QByteArray> JobModel::roleNames() const
{
    return {
        {JobIdRole, "jobId"}, {TitleRole, "title"}, {SubtitleRole, "subtitle"}, {ProgressRole, "progress"},
        {StateRole, "state"}, {MessageRole, "message"}, {OutputsRole, "outputs"}, {ExpiredRole, "expired"},
        {IconRole, "iconName"},
    };
}

QVariant JobModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_jobs.size())
        return {};
    const Job &j = m_jobs.at(index.row());
    switch (role) {
    case JobIdRole: return j.id;
    case TitleRole: return j.title;
    case SubtitleRole: return j.subtitle;
    case ProgressRole: return j.progress;
    case StateRole: return j.state;
    case MessageRole: return j.message;
    case OutputsRole: return j.outputs;
    case ExpiredRole: return j.expired;
    case IconRole: return j.icon;
    }
    return {};
}

int JobModel::running() const
{
    int n = 0;
    for (const Job &j : m_jobs)
        n += j.state == QLatin1String("running");
    return n;
}

int JobModel::rowOf(int jobId) const
{
    for (int i = 0; i < m_jobs.size(); ++i) {
        if (m_jobs.at(i).id == jobId)
            return i;
    }
    return -1;
}

void JobModel::changed(int jobId)
{
    const int row = rowOf(jobId);
    if (row >= 0)
        Q_EMIT dataChanged(index(row), index(row));
}

static QString filesSummary(const QStringList &files)
{
    if (files.size() == 1)
        return QFileInfo(files.first()).fileName();
    return Labels::tr("%n files", nullptr, files.size());
}

int JobModel::start(const QString &opId, const QString &label, const QString &wheel, const QStringList &files,
                    const QVariantMap &params)
{
    Job job;
    job.id = m_nextId++;
    job.title = wheel == QLatin1String("convert") ? Labels::convertingTo(label) : label;
    job.subtitle = filesSummary(files);
    job.icon = QStringLiteral("lingmo-fileaction");
    job.progress = 0;

    QStringList args{QStringLiteral("run"), QStringLiteral("--json"), opId};
    args += files;
    for (auto it = params.cbegin(); it != params.cend(); ++it)
        args << QStringLiteral("--param") << it.key() + QLatin1Char('=') + it.value().toString();

    beginInsertRows(QModelIndex(), m_jobs.size(), m_jobs.size());
    m_jobs.append(job);
    endInsertRows();
    Q_EMIT countChanged();
    Q_EMIT runningChanged();

    const int id = job.id;
    QProcess *proc = Engine::start(args, this);
    m_jobs[rowOf(id)].process = proc;
    connect(proc, &QProcess::readyReadStandardOutput, this, [this, id, proc] {
        const int row = rowOf(id);
        if (row < 0)
            return;
        m_jobs[row].buffer += proc->readAllStandardOutput();
        int nl;
        while ((nl = m_jobs[row].buffer.indexOf('\n')) >= 0) {
            const QByteArray line = m_jobs[row].buffer.left(nl);
            m_jobs[row].buffer.remove(0, nl + 1);
            onLine(id, line);
            if (rowOf(id) != row)
                return;
        }
    });
    connect(proc, &QProcess::finished, this, [this, id, proc](int code, QProcess::ExitStatus) {
        proc->deleteLater();
        const int row = rowOf(id);
        if (row < 0 || m_jobs.at(row).state != QLatin1String("running"))
            return;
        // no "done"/"error" line: the engine crashed or was killed
        const QString err = QString::fromUtf8(proc->readAllStandardError()).trimmed().section(QLatin1Char('\n'), -1);
        finish(id, code == 130 ? QStringLiteral("cancelled") : QStringLiteral("failed"),
               err.isEmpty() ? Labels::tr("The conversion stopped unexpectedly.") : err, {});
    });
    connect(proc, &QProcess::errorOccurred, this, [this, id](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            finish(id, QStringLiteral("failed"), Labels::tr("The conversion engine (lingmo-fileaction-engine) is not installed."), {});
    });
    return id;
}

void JobModel::onLine(int jobId, const QByteArray &line)
{
    const QJsonObject o = QJsonDocument::fromJson(line).object();
    const QString event = o.value(QStringLiteral("event")).toString();
    const int row = rowOf(jobId);
    if (row < 0)
        return;
    if (event == QLatin1String("progress")) {
        m_jobs[row].progress = o.value(QStringLiteral("value")).toDouble();
        changed(jobId);
    } else if (event == QLatin1String("done")) {
        QStringList outputs;
        for (const QJsonValue &v : o.value(QStringLiteral("outputs")).toArray())
            outputs << v.toString();
        QString message = o.value(QStringLiteral("message")).toString();
        if (message.isEmpty())
            message = o.value(QStringLiteral("note")).toString();
        finish(jobId, QStringLiteral("done"), Labels::message(message), outputs);
    } else if (event == QLatin1String("error")) {
        const QString code = o.value(QStringLiteral("code")).toString();
        finish(jobId,
               code == QLatin1String("cancelled") ? QStringLiteral("cancelled")
               : code == QLatin1String("nothing") ? QStringLiteral("nothing")
                                                  : QStringLiteral("failed"),
               Labels::message(o.value(QStringLiteral("message")).toString()), {});
    }
}

void JobModel::finish(int jobId, const QString &state, const QString &message, const QStringList &outputs)
{
    const int row = rowOf(jobId);
    if (row < 0 || m_jobs.at(row).state != QLatin1String("running"))
        return;
    Job &j = m_jobs[row];
    j.state = state;
    j.message = message;
    j.outputs = outputs;
    if (state == QLatin1String("done")) {
        j.progress = 1;
        j.title = outputs.size() == 1 ? Labels::tr("Ready: %1").arg(QFileInfo(outputs.first()).fileName())
                                      : Labels::tr("%n files ready", nullptr, outputs.size());
    }
    changed(jobId);
    Q_EMIT runningChanged();
    Q_EMIT jobFinished(jobId, state == QLatin1String("done"), outputs, message);
    // Done: "Open" / "Show in folder" stay a while; a failure stays long enough to read
    expireLater(jobId, state == QLatin1String("done") ? 12000 : state == QLatin1String("cancelled") ? 0 : 8000);
}

int JobModel::addFinished(const QString &title, const QStringList &files, const QStringList &outputs, const QString &error)
{
    Job job;
    job.id = m_nextId++;
    job.title = title;
    job.subtitle = filesSummary(files);
    job.icon = QStringLiteral("lingmo-fileaction");
    job.progress = error.isEmpty() ? 1 : -1;
    job.state = error.isEmpty() ? QStringLiteral("done") : QStringLiteral("failed");
    job.message = error;
    job.outputs = outputs;
    beginInsertRows(QModelIndex(), m_jobs.size(), m_jobs.size());
    m_jobs.append(job);
    endInsertRows();
    Q_EMIT countChanged();
    expireLater(job.id, error.isEmpty() ? 8000 : 8000);
    return job.id;
}

void JobModel::expireLater(int jobId, int ms)
{
    QTimer::singleShot(ms, this, [this, jobId] {
        const int row = rowOf(jobId);
        if (row < 0 || m_jobs.at(row).expired)
            return;
        m_jobs[row].expired = true;
        changed(jobId);
        // gone from the Island (with its finish message): drop the row
        QTimer::singleShot(4000, this, [this, jobId] {
            const int row = rowOf(jobId);
            if (row < 0)
                return;
            beginRemoveRows(QModelIndex(), row, row);
            m_jobs.removeAt(row);
            endRemoveRows();
            Q_EMIT countChanged();
        });
    });
}

void JobModel::cancel(int jobId)
{
    const int row = rowOf(jobId);
    if (row < 0)
        return;
    QProcess *proc = m_jobs.at(row).process;
    if (m_jobs.at(row).state == QLatin1String("running") && proc && proc->state() != QProcess::NotRunning) {
        ::kill(proc->processId(), SIGTERM); // the engine removes its partial output
        QTimer::singleShot(5000, proc, [proc] { proc->kill(); });
    } else {
        dismiss(jobId);
    }
}

void JobModel::openResult(int jobId)
{
    const int row = rowOf(jobId);
    if (row < 0 || m_jobs.at(row).outputs.isEmpty())
        return;
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_jobs.at(row).outputs.first()));
    dismiss(jobId);
}

void JobModel::showInFolder(int jobId)
{
    const int row = rowOf(jobId);
    if (row < 0 || m_jobs.at(row).outputs.isEmpty())
        return;
    revealFiles(m_jobs.at(row).outputs);
    dismiss(jobId);
}

void JobModel::revealFiles(const QStringList &paths)
{
    // org.freedesktop.FileManager1 (Lingmo Files implements it): opens the folder with the files selected
    QStringList uris;
    for (const QString &p : paths)
        uris << QUrl::fromLocalFile(p).toString();
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.FileManager1"),
                                                      QStringLiteral("/org/freedesktop/FileManager1"),
                                                      QStringLiteral("org.freedesktop.FileManager1"),
                                                      QStringLiteral("ShowItems"));
    msg << uris << QString();
    if (!QDBusConnection::sessionBus().send(msg))
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(paths.first()).absolutePath()));
}

void JobModel::dismiss(int jobId)
{
    const int row = rowOf(jobId);
    if (row < 0 || m_jobs.at(row).expired)
        return;
    if (m_jobs.at(row).state == QLatin1String("running"))
        return cancel(jobId);
    expireLater(jobId, 0);
}

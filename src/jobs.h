/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QAbstractListModel>
#include <QPointer>
#include <QProcess>
#include <QVariantMap>

// Conversions in progress and just finished, one row per job. The Island
// (qml/Shell.qml: one LingmoUI.IslandActivity per row) shows the progress and,
// when done, "Open" and "Show in folder"; Cancel stops the engine (SIGTERM:
// it removes its partial file).
class JobModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int running READ running NOTIFY runningChanged)

public:
    enum Roles {
        JobIdRole = Qt::UserRole + 1,
        TitleRole,
        SubtitleRole,
        ProgressRole,
        StateRole,
        MessageRole,
        OutputsRole,
        ExpiredRole,
        IconRole,
    };

    explicit JobModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int running() const;

    // Runs operation `opId` of the engine on files; label is the slice name ("WEBP", "Comprimir")
    int start(const QString &opId, const QString &label, const QString &wheel, const QStringList &files,
              const QVariantMap &params);

    // A finished file action of the Actions wheel (copy/move): shown like a job
    int addFinished(const QString &title, const QStringList &files, const QStringList &outputs, const QString &error);

    Q_INVOKABLE void cancel(int jobId);
    Q_INVOKABLE void openResult(int jobId);
    Q_INVOKABLE void showInFolder(int jobId);
    Q_INVOKABLE void dismiss(int jobId);

    static void revealFiles(const QStringList &paths);

Q_SIGNALS:
    void countChanged();
    void runningChanged();
    void jobFinished(int jobId, bool ok, const QStringList &outputs, const QString &message);

private:
    struct Job {
        int id = 0;
        QString title;
        QString subtitle;
        QString icon;
        double progress = -1;
        QString state = QStringLiteral("running");
        QString message;
        QStringList outputs;
        bool expired = false;
        QPointer<QProcess> process;
        QByteArray buffer;
    };

    int rowOf(int jobId) const;
    void changed(int jobId);
    void onLine(int jobId, const QByteArray &line);
    void finish(int jobId, const QString &state, const QString &message, const QStringList &outputs);
    void expireLater(int jobId, int ms);

    QList<Job> m_jobs;
    int m_nextId = 1;
};

/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "intents.h"
#include "engine.h"
#include "jobs.h"
#include "labels.h"

#include <LingmoIntents/intentservice.h>

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>

using LingmoIntents::IntentCall;
using LingmoIntents::IntentResult;

namespace {

QString normalizedFormat(QString to)
{
    to = to.trimmed().toLower();
    while (to.startsWith(QLatin1Char('.')))
        to.remove(0, 1);
    static const QHash<QString, QString> aliases{
        {QStringLiteral("jpeg"), QStringLiteral("jpg")}, {QStringLiteral("tif"), QStringLiteral("tiff")},
        {QStringLiteral("htm"), QStringLiteral("html")}, {QStringLiteral("markdown"), QStringLiteral("md")},
        {QStringLiteral("word"), QStringLiteral("docx")}, {QStringLiteral("excel"), QStringLiteral("xlsx")},
        {QStringLiteral("text"), QStringLiteral("txt")}, {QStringLiteral("texto"), QStringLiteral("txt")},
        {QStringLiteral("aif"), QStringLiteral("aiff")}, {QStringLiteral("targz"), QStringLiteral("tar.gz")},
    };
    return aliases.value(to, to);
}

// Runs op on file through the job list (so the Island shows it) and answers the call when it ends
void runJob(JobModel *jobs, QObject *ctxParent, const IntentCall &call, const QJsonObject &op, const QString &wheel,
            const QString &file)
{
    const int id = jobs->start(op.value(QStringLiteral("id")).toString(),
                               Labels::engine(op.value(QStringLiteral("label")).toString()), wheel, {file}, {});
    auto *ctx = new QObject(ctxParent);
    QObject::connect(jobs, &JobModel::jobFinished, ctx,
                     [call, ctx, id](int jobId, bool ok, const QStringList &outputs, const QString &message) {
        if (jobId != id)
            return;
        ctx->deleteLater();
        if (ok && !outputs.isEmpty())
            call.finish(IntentResult::ok({{QStringLiteral("output"), outputs.first()},
                                          {QStringLiteral("outputs"), outputs.join(QLatin1Char('\n'))}}));
        else
            call.finish(IntentResult::failed(message.isEmpty() ? Labels::tr("The conversion failed.") : message));
    });
}

}

void publishFileActionIntents(JobModel *jobs, QObject *parent)
{
    auto *intents = new LingmoIntents::IntentService(QStringLiteral("lingmo-fileaction"), parent);

    intents->handleAsync(QStringLiteral("files.convert"), [jobs, intents](const IntentCall &call) {
        const QString file = call.params().value(QStringLiteral("file")).toString();
        const QString to = normalizedFormat(call.params().value(QStringLiteral("to")).toString());
        if (!QFileInfo(file).isFile())
            return call.finish(IntentResult::failed(Labels::tr("File not found: %1").arg(file)));
        Engine::list({file}, intents, [=](const QJsonObject &doc, const QString &error) {
            if (!error.isEmpty())
                return call.finish(IntentResult::failed(Labels::message(error)));
            QStringList offered;
            for (const QJsonValue &v : doc.value(QStringLiteral("wheels")).toObject().value(QStringLiteral("convert")).toArray()) {
                const QJsonObject op = v.toObject();
                const QString target = op.value(QStringLiteral("target")).toString();
                offered << target;
                if (target != to)
                    continue;
                if (!op.value(QStringLiteral("available")).toBool()) {
                    QStringList missing;
                    for (const QJsonValue &m : op.value(QStringLiteral("missing")).toArray())
                        missing << m.toString();
                    return call.finish(IntentResult::failed(
                        Labels::tr("%1 is needed for this and is not installed.").arg(missing.join(QStringLiteral(", ")))));
                }
                return runJob(jobs, intents, call, op, QStringLiteral("convert"), file);
            }
            call.finish(IntentResult::failed(Labels::tr("%1 can't be converted to %2. It can become: %3.")
                                                 .arg(QFileInfo(file).fileName(), to.toUpper(),
                                                      offered.isEmpty() ? QStringLiteral("—") : offered.join(QStringLiteral(", ")).toUpper()),
                                             LingmoIntents::Errors::InvalidParams));
        });
    });

    intents->handleAsync(QStringLiteral("files.compress"), [jobs, intents](const IntentCall &call) {
        const QString file = call.params().value(QStringLiteral("file")).toString();
        if (!QFileInfo(file).isFile())
            return call.finish(IntentResult::failed(Labels::tr("File not found: %1").arg(file)));
        Engine::list({file}, intents, [=](const QJsonObject &doc, const QString &error) {
            if (!error.isEmpty())
                return call.finish(IntentResult::failed(Labels::message(error)));
            for (const QJsonValue &v : doc.value(QStringLiteral("wheels")).toObject().value(QStringLiteral("tools")).toArray()) {
                const QJsonObject op = v.toObject();
                const QString id = op.value(QStringLiteral("id")).toString();
                if ((id.endsWith(QLatin1String(".compress")) || id == QLatin1String("pdf.optimize"))
                    && op.value(QStringLiteral("available")).toBool())
                    return runJob(jobs, intents, call, op, QStringLiteral("tools"), file);
            }
            call.finish(IntentResult::failed(Labels::tr("There is no way to compress %1.").arg(QFileInfo(file).fileName())));
        });
    });

    intents->handleAsync(QStringLiteral("files.actions.list"), [intents](const IntentCall &call) {
        const QString file = call.params().value(QStringLiteral("file")).toString();
        if (!QFileInfo::exists(file))
            return call.finish(IntentResult::failed(Labels::tr("File not found: %1").arg(file)));
        Engine::list({file}, intents, [=](const QJsonObject &doc, const QString &error) {
            if (!error.isEmpty())
                return call.finish(IntentResult::failed(Labels::message(error)));
            QStringList formats, tools;
            const QJsonObject wheels = doc.value(QStringLiteral("wheels")).toObject();
            for (const QJsonValue &v : wheels.value(QStringLiteral("convert")).toArray()) {
                if (v.toObject().value(QStringLiteral("available")).toBool())
                    formats << v.toObject().value(QStringLiteral("target")).toString();
            }
            for (const QJsonValue &v : wheels.value(QStringLiteral("tools")).toArray()) {
                const QJsonObject o = v.toObject();
                if (!o.value(QStringLiteral("available")).toBool())
                    continue;
                QString label = Labels::engine(o.value(QStringLiteral("label")).toString());
                const QString sub = Labels::engine(o.value(QStringLiteral("subtitle")).toString());
                tools << (sub.isEmpty() ? label : QStringLiteral("%1 (%2)").arg(label, sub));
            }
            formats.removeAll(QString());
            call.finish(IntentResult::ok({{QStringLiteral("formats"), formats.join(QStringLiteral(", "))},
                                          {QStringLiteral("tools"), tools.join(QStringLiteral(", "))}}));
        });
    });

    intents->publish(); // /Intents on com.lingmo.FileAction, which main() owns
}

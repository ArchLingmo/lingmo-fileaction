/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QStringList>
#include <functional>

// The conversions come from lingmo-fileaction-engine (the FileAction fork, C#):
//   list --json FILE...            what the Convert and Tools wheels offer for these files
//   run --json ID FILE... --param  one JSON object per line: start, progress, done / error
// LINGMO_FILEACTION_ENGINE overrides the command (e.g. "dotnet /path/lingmo-fileaction-engine.dll").
namespace Engine {

QString program();
QStringList arguments(const QStringList &args);
// English messages from the engine (the app translates them by text); C.UTF-8 keeps file names intact
QProcessEnvironment environment();
// Starts the engine with these arguments on a new QProcess owned by parent
QProcess *start(const QStringList &args, QObject *parent);

// list --json, asynchronously; done(result, error) on the receiver's thread
void list(const QStringList &files, QObject *receiver, std::function<void(const QJsonObject &, const QString &)> done);

}

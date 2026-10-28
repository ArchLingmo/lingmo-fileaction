/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

class JobModel;
class QObject;

// Lingmo Intents on com.lingmo.FileAction /Intents (data/intents/lingmo-fileaction.json):
//   files.convert       a file to another format (result next to it, progress in the Island)
//   files.compress      a smaller copy of an image, video, audio or PDF
//   files.actions.list  what the wheels offer for a file
void publishFileActionIntents(JobModel *jobs, QObject *parent);

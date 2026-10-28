/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QCoreApplication>
#include <QString>

// The engine speaks English (upstream FileAction has English and Czech); the
// slice names, subtitles, parameter labels and the usual messages are
// translated here by their English text ("Engine" context in the .ts file).
class Labels
{
    Q_DECLARE_TR_FUNCTIONS(Labels)
public:
    static QString engine(const QString &english);
    static QString message(const QString &english);
    static QString convertingTo(const QString &format);
};

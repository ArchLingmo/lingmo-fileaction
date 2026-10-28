/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "labels.h"

// Every English text of the engine's wheels (FileAction.Core Operations/Catalog.cs),
// listed so lupdate puts them in the .ts file
[[maybe_unused]] static const char *const engineStrings[] = {
    QT_TRANSLATE_NOOP("Engine", "Compress"), QT_TRANSLATE_NOOP("Engine", "Shrink"),
    QT_TRANSLATE_NOOP("Engine", "for web"), QT_TRANSLATE_NOOP("Engine", "Rotate"),
    QT_TRANSLATE_NOOP("Engine", "right"), QT_TRANSLATE_NOOP("Engine", "left"),
    QT_TRANSLATE_NOOP("Engine", "Mirror"), QT_TRANSLATE_NOOP("Engine", "Square"),
    QT_TRANSLATE_NOOP("Engine", "crop"), QT_TRANSLATE_NOOP("Engine", "Grayscale"),
    QT_TRANSLATE_NOOP("Engine", "No metadata"), QT_TRANSLATE_NOOP("Engine", "Merge to PDF"),
    QT_TRANSLATE_NOOP("Engine", "document"), QT_TRANSLATE_NOOP("Engine", "video"),
    QT_TRANSLATE_NOOP("Engine", "Normalize"), QT_TRANSLATE_NOOP("Engine", "loudness"),
    QT_TRANSLATE_NOOP("Engine", "Trim"), QT_TRANSLATE_NOOP("Engine", "start–end"),
    QT_TRANSLATE_NOOP("Engine", "Join"), QT_TRANSLATE_NOOP("Engine", "audio only"),
    QT_TRANSLATE_NOOP("Engine", "animation"), QT_TRANSLATE_NOOP("Engine", "downscale"),
    QT_TRANSLATE_NOOP("Engine", "Mute"), QT_TRANSLATE_NOOP("Engine", "muted"),
    QT_TRANSLATE_NOOP("Engine", "Speed up"), QT_TRANSLATE_NOOP("Engine", "From S-Log"),
    QT_TRANSLATE_NOOP("Engine", "Sony → normal video"), QT_TRANSLATE_NOOP("Engine", "Apply LUT"),
    QT_TRANSLATE_NOOP("Engine", ".cube file"), QT_TRANSLATE_NOOP("Engine", "Thumbnail"),
    QT_TRANSLATE_NOOP("Engine", "PNG frame"), QT_TRANSLATE_NOOP("Engine", "all pages"),
    QT_TRANSLATE_NOOP("Engine", "text"), QT_TRANSLATE_NOOP("Engine", "tables"),
    QT_TRANSLATE_NOOP("Engine", "web"), QT_TRANSLATE_NOOP("Engine", "e-book"),
    QT_TRANSLATE_NOOP("Engine", "Merge PDFs"), QT_TRANSLATE_NOOP("Engine", "Split"),
    QT_TRANSLATE_NOOP("Engine", "into pages"), QT_TRANSLATE_NOOP("Engine", "Optimize"),
    QT_TRANSLATE_NOOP("Engine", "all slides"), QT_TRANSLATE_NOOP("Engine", "text only"),
    QT_TRANSLATE_NOOP("Engine", "Extract"), QT_TRANSLATE_NOOP("Engine", "Archive"),
    QT_TRANSLATE_NOOP("Engine", "Extract text"), QT_TRANSLATE_NOOP("Engine", "Start"),
    QT_TRANSLATE_NOOP("Engine", "End"), QT_TRANSLATE_NOOP("Engine", "e.g. 0:30 or 1:02:03"),
    QT_TRANSLATE_NOOP("Engine", "empty = until the end"), QT_TRANSLATE_NOOP("Engine", "slides"),
    QT_TRANSLATE_NOOP("Engine", "high quality"), QT_TRANSLATE_NOOP("Engine", "medium quality"),
    QT_TRANSLATE_NOOP("Engine", "small file"), QT_TRANSLATE_NOOP("Engine", "original size"),
    QT_TRANSLATE_NOOP("Engine", "same format"), QT_TRANSLATE_NOOP("Engine", "Max 1920 px"),
    QT_TRANSLATE_NOOP("Engine", "Merge"), QT_TRANSLATE_NOOP("Engine", "compressed"),
    // Messages
    QT_TRANSLATE_NOOP("Engine", "Cancelled."),
    QT_TRANSLATE_NOOP("Engine", "The image is already well compressed; no smaller copy was made."),
    QT_TRANSLATE_NOOP("Engine", "The file is already well compressed; no smaller copy was made."),
    QT_TRANSLATE_NOOP("Engine", "The image is already smaller than 1920 px; nothing to change."),
    QT_TRANSLATE_NOOP("Engine", "The audio is already at 128 kbit/s or less; smaller would only lose quality."),
    QT_TRANSLATE_NOOP("Engine", "The video is already 720p or smaller; there is nothing to gain."),
    QT_TRANSLATE_NOOP("Engine", "The end must come after the start."),
    QT_TRANSLATE_NOOP("Engine", "The conversion produced no output."),
};

QString Labels::engine(const QString &english)
{
    if (english.isEmpty())
        return english;
    return QCoreApplication::translate("Engine", english.toUtf8().constData());
}

QString Labels::message(const QString &english)
{
    return engine(english);
}

QString Labels::convertingTo(const QString &format)
{
    return tr("Converting to %1").arg(format);
}

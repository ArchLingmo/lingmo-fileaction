/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Dialogs
import QtQml.Models
import LingmoUI.CompatibleModule 3.0 as LingmoUI

// Everything lives without a main window: the wheel, the parameter and folder
// dialogs, the About box and one Island activity per conversion.
Item {
    id: shell

    // the one-off instance waits for these before quitting
    readonly property bool busy: paramLoader.active || folderDialog.visible || aboutLoader.active || settingsLoader.active

    function showAbout() {
        aboutLoader.active = true
        aboutLoader.item.show()
        aboutLoader.item.raise()
        aboutLoader.item.requestActivate()
    }

    function showSettings() {
        settingsLoader.active = true
        settingsLoader.item.show()
        settingsLoader.item.raise()
        settingsLoader.item.requestActivate()
    }

    Wheel {
        id: wheel
    }

    Loader {
        id: settingsLoader
        active: false
        sourceComponent: Settings {
            onClosing: settingsLoader.active = false
        }
    }

    Connections {
        target: fa
        function onParametersRequested(index, slice) {
            paramLoader.active = true
            paramLoader.item.open(index, slice)
        }
        function onFolderRequested(actionId, move) {
            folderDialog.actionId = actionId
            folderDialog.title = move ? qsTr("Move to") : qsTr("Copy to")
            folderDialog.open()
        }
    }

    Loader {
        id: paramLoader
        active: false
        sourceComponent: ParamDialog {
            onClosing: paramLoader.active = false
        }
    }

    FolderDialog {
        id: folderDialog
        property string actionId
        onAccepted: fa.runWithFolder(actionId, selectedFolder)
    }

    Loader {
        id: aboutLoader
        active: false
        sourceComponent: LingmoUI.AboutDialog {
            name: qsTr("File actions")
            iconSource: "image://icontheme/lingmo-fileaction"
            description: qsTr("Convert files by dropping them on a wheel: drag a file, tap Shift, Ctrl or Alt and drop it on a slice.")
                         + "\n\n" + qsTr("Based on FileAction (MIT) by Metamorfolog, inspired by Tangerine.")
                         + "\n" + qsTr("Version %1").arg(appVersion)
            link: "https://github.com/ArchLingmo/lingmo-fileaction"
            onVisibleChanged: if (!visible) aboutLoader.active = false
        }
    }

    // Progress, result and errors in the status bar's Island (docs/island-api.md)
    Instantiator {
        model: jobs
        delegate: LingmoUI.IslandActivity {
            id: activity
            readonly property bool running: model.state === "running"
            readonly property bool done: model.state === "done"
            appId: "lingmo-fileaction"
            kind: "generic"
            // Settings › General: without the Island only what went wrong is shown
            active: !model.expired && (prefs.showInIsland || model.state === "failed" || model.state === "nothing")
            iconName: model.iconName
            title: model.state === "failed" ? qsTr("Could not finish: %1").arg(model.title)
                 : model.state === "nothing" ? qsTr("Nothing to do")
                 : model.title
            subtitle: (model.state === "failed" || model.state === "nothing" || (done && model.message.length > 0))
                      ? model.message : model.subtitle
            progress: running ? Math.max(0, model.progress) : (done ? 1 : -1)
            hints: ({
                indeterminate: running && model.progress <= 0,
                detail: done && model.outputs.length > 0 ? model.subtitle : "",
                actionIcons: ({ folder: "folder-open" })
            })
            actions: running ? [ { id: "cancel", label: qsTr("Cancel") } ]
                   : (done && model.outputs.length > 0)
                     ? [ { id: "open", label: qsTr("Open") }, { id: "folder", label: qsTr("Show in folder") } ]
                     : []
            finishMessage: model.state === "cancelled" ? qsTr("Cancelled") : ""
            onActionInvoked: (actionId) => {
                if (actionId === "cancel")
                    jobs.cancel(model.jobId)
                else if (actionId === "open")
                    jobs.openResult(model.jobId)
                else if (actionId === "folder")
                    jobs.showInFolder(model.jobId)
            }
        }
    }
}

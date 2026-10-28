/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LingmoUI.CompatibleModule 3.0 as LingmoUI
import "settings"

// lingmo-fileaction --settings (also the gear on the wheel and the app menu):
// the pages of upstream FileAction's settings window, the Lingmo way
LingmoUI.Window {
    id: settingsWindow

    property int currentPage: 0

    title: qsTr("File actions settings")
    width: 940
    height: 660
    minimumWidth: 780
    minimumHeight: 520
    visible: false

    background.opacity: LingmoUI.Theme.darkMode ? 0.88 : 0.97
    header.height: LingmoUI.Units.windowHeaderHeight
    contentTopMargin: 0

    LingmoUI.WindowBlur {
        view: settingsWindow
        geometry: Qt.rect(settingsWindow.x, settingsWindow.y, settingsWindow.width, settingsWindow.height)
        windowRadius: settingsWindow.windowRadius
        enabled: true
    }

    Component.onCompleted: {
        prefs.reloadCatalog()
        prefs.refreshHelpers()
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        LingmoUI.SettingsSideBar {
            id: sideBar
            Layout.fillHeight: true
            title: qsTr("File actions")
            iconName: "lingmo-fileaction"
            currentIndex: settingsWindow.currentPage
            onCurrentIndexChanged: settingsWindow.currentPage = currentIndex

            model: ListModel {
                id: pages
                ListElement { title: qsTr("General"); iconName: "preferences-system"; iconColor: "transparent" }
                ListElement { title: qsTr("Gestures and keys"); iconName: "input-keyboard"; iconColor: "transparent" }
                ListElement { title: qsTr("Wheels"); iconName: "office-chart-pie"; iconColor: "transparent" }
                ListElement { title: qsTr("Quality"); iconName: "image-x-generic"; iconColor: "transparent" }
                ListElement { title: qsTr("Helper programs"); iconName: "package-x-generic"; iconColor: "transparent" }
                ListElement { title: qsTr("About"); iconName: "help-about"; iconColor: "transparent" }
            }

            // ListElement only takes constants: the tile colours follow the theme from here
            readonly property var tileColors: [LingmoUI.Theme.greyColor, LingmoUI.Theme.blueColor, LingmoUI.Theme.orangeColor,
                                               LingmoUI.Theme.greenColor, LingmoUI.Theme.purpleColor, LingmoUI.Theme.greyColor]
            onTileColorsChanged: updateTiles()
            Component.onCompleted: updateTiles()
            function updateTiles() {
                for (let i = 0; i < pages.count && i < tileColors.length; ++i)
                    pages.setProperty(i, "iconColor", tileColors[i].toString())
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: settingsWindow.currentPage

            GeneralPage {}
            GesturesPage {}
            WheelsPage {}
            QualityPage {}
            HelpersPage {}
            AboutPage {}
        }
    }
}

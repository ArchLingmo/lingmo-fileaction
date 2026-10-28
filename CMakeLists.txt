cmake_minimum_required(VERSION 3.16)
project(lingmo-fileaction VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 DBus LinguistTools)
# The drag-and-tap gesture: XInput2 raw keys and buttons, XFixes for the XdndSelection owner
find_package(X11 REQUIRED COMPONENTS Xi Xfixes)
# the apps that open each file type (Actions wheel: "Open with")
find_package(KF6Service REQUIRED)
find_package(KF6KIO REQUIRED)

set(TRANSLATIONS_DIR ${CMAKE_INSTALL_FULL_DATADIR}/lingmo-fileaction/translations)
# The conversions: lingmo-fileaction-engine (the FileAction fork, C#); LINGMO_FILEACTION_ENGINE overrides it
set(ENGINE_COMMAND "lingmo-fileaction-engine" CACHE STRING "FileAction engine command")

add_executable(lingmo-fileaction
    src/main.cpp
    src/engine.cpp src/engine.h
    src/jobs.cpp src/jobs.h
    src/wheel.cpp src/wheel.h
    src/gesture.cpp src/gesture.h
    src/labels.cpp src/labels.h
    src/prefs.cpp src/prefs.h
    qml.qrc
)
target_compile_definitions(lingmo-fileaction PRIVATE
    TRANSLATIONS_DIR="${TRANSLATIONS_DIR}"
    ENGINE_COMMAND="${ENGINE_COMMAND}"
    APP_VERSION="${PROJECT_VERSION}")
target_include_directories(lingmo-fileaction PRIVATE ${X11_INCLUDE_DIR})
target_link_libraries(lingmo-fileaction PRIVATE
    KF6::Service KF6::KIOGui
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::DBus
    ${X11_LIBRARIES} ${X11_Xi_LIB} ${X11_Xfixes_LIB})

# Lingmo Intents: files.convert, files.compress, files.actions.list on com.lingmo.FileAction /Intents
find_package(LingmoIntents 1.0 QUIET)
if(LingmoIntents_FOUND)
    target_sources(lingmo-fileaction PRIVATE src/intents.cpp src/intents.h)
    target_compile_definitions(lingmo-fileaction PRIVATE HAVE_LINGMO_INTENTS)
    target_link_libraries(lingmo-fileaction PRIVATE LingmoIntents::LingmoIntents)
    lingmo_install_intents(MANIFEST data/intents/lingmo-fileaction.json SERVICE data/com.lingmo.FileAction.service)
else()
    message(STATUS "LingmoIntents not found: built without app actions (intents)")
    install(FILES data/com.lingmo.FileAction.service DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/services)
endif()

qt_add_translation(QM_FILES translations/lingmo-fileaction_pt_BR.ts)
add_custom_target(translations ALL DEPENDS ${QM_FILES})

install(TARGETS lingmo-fileaction RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${QM_FILES} DESTINATION ${TRANSLATIONS_DIR})
install(FILES data/lingmo-fileaction.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
# The drag gesture for the whole session (the Lingmo session runs XDG autostart)
install(FILES data/lingmo-fileaction-daemon.desktop DESTINATION /etc/xdg/autostart)
install(FILES icons/lingmo-fileaction.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES LICENSE THIRD-PARTY-NOTICES.md DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/lingmo-fileaction)

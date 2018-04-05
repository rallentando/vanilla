#!/bin/bash

TARGET_DIR="package/vanilla"

SYSTEM_DIR="C:/Windows/System32"

for drive in C D E F G
do
    if   [ -e   $drive:/Qt/Qt5.9.0/5.9/msvc2015 ]; then
        QT5_DIR=$drive:/Qt/Qt5.9.0/5.9/msvc2015
        break
    elif [ -e   $drive:/Qt/Qt5.9.0/5.9/msvc2015_64 ]; then
        QT5_DIR=$drive:/Qt/Qt5.9.0/5.9/msvc2015_64
        break
    elif [ -e   $drive:/Qt/Qt5.9.0-x64/5.9/msvc2015_64 ]; then
        QT5_DIR=$drive:/Qt/Qt5.9.0-x64/5.9/msvc2015_64
        break
    elif [ -e   $drive:/Qt/5.9/msvc2015_64 ]; then
        QT5_DIR=$drive:/Qt/5.9/msvc2015_64
        break
    elif [ -e   $drive:/Qt/5.9.1/msvc2015_64 ]; then
        QT5_DIR=$drive:/Qt/5.9.1/msvc2015_64
        break
    fi
done

for drive in C D E F G
do
    if [[ $QT5_DIR == *"_64" ]]; then
        if [ -e $drive:/OpenSSL-Win64/bin ]; then
            OPENSSL_DIR=$drive:/OpenSSL-Win64/bin
            break
        fi
    else
        if [ -e $drive:/OpenSSL-Win32/bin ]; then
            OPENSSL_DIR=$drive:/OpenSSL-Win32/bin
            break
        fi
    fi
done

mkdir -p $TARGET_DIR/
mkdir -p $TARGET_DIR/translations/
mkdir -p $TARGET_DIR/translations/qtwebengine_locales/

mkdir -p $TARGET_DIR/audio/
mkdir -p $TARGET_DIR/bearer/
mkdir -p $TARGET_DIR/iconengines/
mkdir -p $TARGET_DIR/imageformats/
mkdir -p $TARGET_DIR/mediaservice/
mkdir -p $TARGET_DIR/platforms/
mkdir -p $TARGET_DIR/playlistformats/
mkdir -p $TARGET_DIR/position/
mkdir -p $TARGET_DIR/printsupport/
mkdir -p $TARGET_DIR/qmltooling/
#mkdir -p $TARGET_DIR/qtwebengine/
mkdir -p $TARGET_DIR/resources/
mkdir -p $TARGET_DIR/sensorgestures/
mkdir -p $TARGET_DIR/sensors/
mkdir -p $TARGET_DIR/sqldrivers/

mkdir -p $TARGET_DIR/QtGraphicalEffects/
mkdir -p $TARGET_DIR/QtGraphicalEffects/private/
mkdir -p $TARGET_DIR/QtQml/
mkdir -p $TARGET_DIR/QtQml/Models.2/
mkdir -p $TARGET_DIR/QtQuick/
mkdir -p $TARGET_DIR/QtQuick/Controls/
mkdir -p $TARGET_DIR/QtQuick/Dialogs/
mkdir -p $TARGET_DIR/QtQuick/Dialogs/Private/
mkdir -p $TARGET_DIR/QtQuick/Window.2/
mkdir -p $TARGET_DIR/QtQuick.2/
mkdir -p $TARGET_DIR/QtWebEngine/
mkdir -p $TARGET_DIR/QtWebEngine/experimental/
#mkdir -p $TARGET_DIR/QtWebEngine/UIDelegates/
mkdir -p $TARGET_DIR/QtWebEngine/Controls1Delegates/
mkdir -p $TARGET_DIR/QtWebEngine/Controls2Delegates/

#cp $OPENSSL_DIR/ssleay32.dll $TARGET_DIR/
#cp $OPENSSL_DIR/libeay32.dll $TARGET_DIR/

cp $SYSTEM_DIR/msvcr120.dll $TARGET_DIR/
cp $SYSTEM_DIR/msvcp120.dll $TARGET_DIR/

cp $QT5_DIR/bin/Qt5Concurrent.dll        $TARGET_DIR/
cp $QT5_DIR/bin/Qt5Core.dll              $TARGET_DIR/
cp $QT5_DIR/bin/Qt5Gui.dll               $TARGET_DIR/
cp $QT5_DIR/bin/Qt5Multimedia.dll        $TARGET_DIR/
cp $QT5_DIR/bin/Qt5MultimediaWidgets.dll $TARGET_DIR/
cp $QT5_DIR/bin/Qt5Network.dll           $TARGET_DIR/
cp $QT5_DIR/bin/Qt5OpenGL.dll            $TARGET_DIR/
cp $QT5_DIR/bin/Qt5PrintSupport.dll      $TARGET_DIR/
cp $QT5_DIR/bin/Qt5Positioning.dll       $TARGET_DIR/
cp $QT5_DIR/bin/Qt5Qml.dll               $TARGET_DIR/
cp $QT5_DIR/bin/Qt5Quick.dll             $TARGET_DIR/
cp $QT5_DIR/bin/Qt5QuickControls2.dll    $TARGET_DIR/
cp $QT5_DIR/bin/Qt5QuickWidgets.dll      $TARGET_DIR/
cp $QT5_DIR/bin/Qt5Sensors.dll           $TARGET_DIR/
cp $QT5_DIR/bin/Qt5Sql.dll               $TARGET_DIR/
cp $QT5_DIR/bin/Qt5Svg.dll               $TARGET_DIR/
cp $QT5_DIR/bin/Qt5WebChannel.dll        $TARGET_DIR/
cp $QT5_DIR/bin/Qt5WebEngine.dll         $TARGET_DIR/
cp $QT5_DIR/bin/Qt5WebEngineCore.dll     $TARGET_DIR/
cp $QT5_DIR/bin/Qt5WebEngineWidgets.dll  $TARGET_DIR/
cp $QT5_DIR/bin/Qt5WebView.dll           $TARGET_DIR/
cp $QT5_DIR/bin/Qt5Widgets.dll           $TARGET_DIR/
cp $QT5_DIR/bin/Qt5WinExtras.dll         $TARGET_DIR/
cp $QT5_DIR/bin/Qt5Xml.dll               $TARGET_DIR/
cp $QT5_DIR/bin/d3dcompiler_47.dll       $TARGET_DIR/
#cp $QT5_DIR/bin/icudt54.dll              $TARGET_DIR/
#cp $QT5_DIR/bin/icuin54.dll              $TARGET_DIR/
#cp $QT5_DIR/bin/icuuc54.dll              $TARGET_DIR/
#cp $QT5_DIR/bin/libEGL.dll               $TARGET_DIR/
#cp $QT5_DIR/bin/libGLESv2.dll            $TARGET_DIR/
cp $QT5_DIR/bin/opengl32sw.dll           $TARGET_DIR/
cp $QT5_DIR/bin/QtWebEngineProcess.exe   $TARGET_DIR/

cp $QT5_DIR/plugins/audio/qtaudio_windows.dll              $TARGET_DIR/audio/

cp $QT5_DIR/plugins/bearer/qgenericbearer.dll              $TARGET_DIR/bearer/
cp $QT5_DIR/plugins/bearer/qnativewifibearer.dll           $TARGET_DIR/bearer/

cp $QT5_DIR/plugins/iconengines/qsvgicon.dll               $TARGET_DIR/iconengines/

#cp $QT5_DIR/plugins/imageformats/qdds.dll                  $TARGET_DIR/imageformats/
cp $QT5_DIR/plugins/imageformats/qgif.dll                  $TARGET_DIR/imageformats/
cp $QT5_DIR/plugins/imageformats/qicns.dll                 $TARGET_DIR/imageformats/
cp $QT5_DIR/plugins/imageformats/qico.dll                  $TARGET_DIR/imageformats/
#cp $QT5_DIR/plugins/imageformats/qjp2.dll                  $TARGET_DIR/imageformats/
cp $QT5_DIR/plugins/imageformats/qjpeg.dll                 $TARGET_DIR/imageformats/
#cp $QT5_DIR/plugins/imageformats/qmng.dll                  $TARGET_DIR/imageformats/
cp $QT5_DIR/plugins/imageformats/qsvg.dll                  $TARGET_DIR/imageformats/
cp $QT5_DIR/plugins/imageformats/qtga.dll                  $TARGET_DIR/imageformats/
cp $QT5_DIR/plugins/imageformats/qtiff.dll                 $TARGET_DIR/imageformats/
cp $QT5_DIR/plugins/imageformats/qwbmp.dll                 $TARGET_DIR/imageformats/
cp $QT5_DIR/plugins/imageformats/qwebp.dll                 $TARGET_DIR/imageformats/

cp $QT5_DIR/plugins/mediaservice/dsengine.dll              $TARGET_DIR/mediaservice/
cp $QT5_DIR/plugins/mediaservice/qtmedia_audioengine.dll   $TARGET_DIR/mediaservice/
#cp $QT5_DIR/plugins/mediaservice/wmfengine.dll             $TARGET_DIR/mediaservice/

cp $QT5_DIR/plugins/platforms/qminimal.dll                 $TARGET_DIR/platforms/
cp $QT5_DIR/plugins/platforms/qoffscreen.dll               $TARGET_DIR/platforms/
cp $QT5_DIR/plugins/platforms/qwindows.dll                 $TARGET_DIR/platforms/

cp $QT5_DIR/plugins/playlistformats/qtmultimedia_m3u.dll   $TARGET_DIR/playlistformats/

cp $QT5_DIR/plugins/position/qtposition_positionpoll.dll   $TARGET_DIR/position/

cp $QT5_DIR/plugins/printsupport/windowsprintersupport.dll $TARGET_DIR/printsupport/

#cp $QT5_DIR/plugins/qmltooling/qmldbg_qtquick2.dll         $TARGET_DIR/qmltooling/
cp $QT5_DIR/plugins/qmltooling/qmldbg_debugger.dll         $TARGET_DIR/qmltooling/
cp $QT5_DIR/plugins/qmltooling/qmldbg_inspector.dll        $TARGET_DIR/qmltooling/
cp $QT5_DIR/plugins/qmltooling/qmldbg_local.dll            $TARGET_DIR/qmltooling/
cp $QT5_DIR/plugins/qmltooling/qmldbg_profiler.dll         $TARGET_DIR/qmltooling/
cp $QT5_DIR/plugins/qmltooling/qmldbg_server.dll           $TARGET_DIR/qmltooling/
cp $QT5_DIR/plugins/qmltooling/qmldbg_tcp.dll              $TARGET_DIR/qmltooling/

cp $QT5_DIR/plugins/sensorgestures/qtsensorgestures_plugin.dll $TARGET_DIR/sensorgestures/
cp $QT5_DIR/plugins/sensorgestures/qtsensorgestures_shakeplugin.dll $TARGET_DIR/sensorgestures/

cp $QT5_DIR/plugins/sensors/qtsensors_generic.dll          $TARGET_DIR/sensors/

cp $QT5_DIR/plugins/sqldrivers/qsqlite.dll                 $TARGET_DIR/sqldrivers/
cp $QT5_DIR/plugins/sqldrivers/qsqlmysql.dll               $TARGET_DIR/sqldrivers/
cp $QT5_DIR/plugins/sqldrivers/qsqlodbc.dll                $TARGET_DIR/sqldrivers/
cp $QT5_DIR/plugins/sqldrivers/qsqlpsql.dll                $TARGET_DIR/sqldrivers/

cp $QT5_DIR/qml/QtGraphicalEffects/qtgraphicaleffectsplugin.dll       $TARGET_DIR/QtGraphicalEffects/
cp $QT5_DIR/qml/QtGraphicalEffects/*.qml                              $TARGET_DIR/QtGraphicalEffects/
cp $QT5_DIR/qml/QtGraphicalEffects/*.qmlc                             $TARGET_DIR/QtGraphicalEffects/
cp $QT5_DIR/qml/QtGraphicalEffects/qmldir                             $TARGET_DIR/QtGraphicalEffects/

cp $QT5_DIR/qml/QtGraphicalEffects/private/qtgraphicaleffectsprivate.dll $TARGET_DIR/QtGraphicalEffects/private/
cp $QT5_DIR/qml/QtGraphicalEffects/private/*.qml                      $TARGET_DIR/QtGraphicalEffects/private/
cp $QT5_DIR/qml/QtGraphicalEffects/private/*.qmlc                     $TARGET_DIR/QtGraphicalEffects/private/
cp $QT5_DIR/qml/QtGraphicalEffects/private/qmldir                     $TARGET_DIR/QtGraphicalEffects/private/

cp $QT5_DIR/qml/QtQml/Models.2/qmldir                                 $TARGET_DIR/QtQml/Models.2/
cp $QT5_DIR/qml/QtQml/Models.2/modelsplugin.dll                       $TARGET_DIR/QtQml/Models.2/

cp $QT5_DIR/qml/QtQuick/Controls/qtquickcontrolsplugin.dll            $TARGET_DIR/QtQuick/Controls/
cp $QT5_DIR/qml/QtQuick/Controls/qmldir                               $TARGET_DIR/QtQuick/Controls/
cp $QT5_DIR/qml/QtQuick/Controls/*.qml                                $TARGET_DIR/QtQuick/Controls/
cp $QT5_DIR/qml/QtQuick/Controls/Private                              $TARGET_DIR/QtQuick/Controls/ -R
cp $QT5_DIR/qml/QtQuick/Controls/Styles                               $TARGET_DIR/QtQuick/Controls/ -R

cp $QT5_DIR/qml/QtQuick/Dialogs/dialogplugin.dll                      $TARGET_DIR/QtQuick/Dialogs/
cp $QT5_DIR/qml/QtQuick/Dialogs/qmldir                                $TARGET_DIR/QtQuick/Dialogs/
cp $QT5_DIR/qml/QtQuick/Dialogs/*.qml                                 $TARGET_DIR/QtQuick/Dialogs/
cp $QT5_DIR/qml/QtQuick/Dialogs/images                                $TARGET_DIR/QtQuick/Dialogs/ -R
cp $QT5_DIR/qml/QtQuick/Dialogs/qml                                   $TARGET_DIR/QtQuick/Dialogs/ -R
cp $QT5_DIR/qml/QtQuick/Dialogs/Private/dialogsprivateplugin.dll      $TARGET_DIR/QtQuick/Dialogs/Private/
cp $QT5_DIR/qml/QtQuick/Dialogs/Private/qmldir                        $TARGET_DIR/QtQuick/Dialogs/Private/

cp $QT5_DIR/qml/QtQuick/Window.2/windowplugin.dll                     $TARGET_DIR/QtQuick/Window.2/
cp $QT5_DIR/qml/QtQuick/Window.2/qmldir                               $TARGET_DIR/QtQuick/Window.2/

cp $QT5_DIR/qml/QtQuick.2/qtquick2plugin.dll                          $TARGET_DIR/QtQuick.2/
cp $QT5_DIR/qml/QtQuick.2/qmldir                                      $TARGET_DIR/QtQuick.2/

cp $QT5_DIR/qml/QtWebEngine/qtwebengineplugin.dll                          $TARGET_DIR/QtWebEngine/
cp $QT5_DIR/qml/QtWebEngine/qmldir                                         $TARGET_DIR/QtWebEngine/
#cp $QT5_DIR/qml/QtWebEngine/experimental/qtwebengineexperimentalplugin.dll $TARGET_DIR/QtWebEngine/experimental/
#cp $QT5_DIR/qml/QtWebEngine/experimental/qmldir                            $TARGET_DIR/QtWebEngine/experimental/
#cp $QT5_DIR/qml/QtWebEngine/UIDelegates/*.qml                              $TARGET_DIR/QtWebEngine/UIDelegates/
#cp $QT5_DIR/qml/QtWebEngine/UIDelegates/qmldir                             $TARGET_DIR/QtWebEngine/UIDelegates/
cp $QT5_DIR/qml/QtWebEngine/Controls1Delegates/*.qml                       $TARGET_DIR/QtWebEngine/Controls1Delegates/
cp $QT5_DIR/qml/QtWebEngine/Controls1Delegates/qmldir                      $TARGET_DIR/QtWebEngine/Controls1Delegates/
cp $QT5_DIR/qml/QtWebEngine/Controls2Delegates/*.qml                       $TARGET_DIR/QtWebEngine/Controls2Delegates/
cp $QT5_DIR/qml/QtWebEngine/Controls2Delegates/*.png                       $TARGET_DIR/QtWebEngine/Controls2Delegates/
cp $QT5_DIR/qml/QtWebEngine/Controls2Delegates/qmldir                      $TARGET_DIR/QtWebEngine/Controls2Delegates/

cp $QT5_DIR/translations/qt_*.qm     $TARGET_DIR/translations/
cp $QT5_DIR/translations/qtbase_*.qm $TARGET_DIR/translations/

cp $QT5_DIR/resources/icudtl.dat $TARGET_DIR/resources/
cp $QT5_DIR/resources/qtwebengine_devtools_resources.pak $TARGET_DIR/resources/
cp $QT5_DIR/resources/qtwebengine_resources.pak $TARGET_DIR/resources/
cp $QT5_DIR/resources/qtwebengine_resources_100p.pak $TARGET_DIR/resources/
cp $QT5_DIR/resources/qtwebengine_resources_200p.pak $TARGET_DIR/resources/
cp $QT5_DIR/translations/qtwebengine_locales/*.pak $TARGET_DIR/translations/qtwebengine_locales/

cp translations/*.qm $TARGET_DIR/translations/

cp scripts/edge.bat $TARGET_DIR/

cp LICENSE $TARGET_DIR/

#cp qt.conf $TARGET_DIR/

if [ -e $QT5_DIR/bin/Qt5WebKit.dll ]; then
    mkdir -p $TARGET_DIR/scenegraph/
    mkdir -p $TARGET_DIR/QtWebKit/
    mkdir -p $TARGET_DIR/QtWebKit/experimental/

    #cp $OPENSSL_DIR/ssleay32.dll $TARGET_DIR/
    #cp $OPENSSL_DIR/libeay32.dll $TARGET_DIR/

    cp $QT5_DIR/bin/Qt5SerialPort.dll        $TARGET_DIR/
    cp $QT5_DIR/bin/Qt5WebKit.dll            $TARGET_DIR/
    cp $QT5_DIR/bin/Qt5WebKitWidgets.dll     $TARGET_DIR/

    cp $QT5_DIR/bin/icudt57.dll              $TARGET_DIR/
    cp $QT5_DIR/bin/icuin57.dll              $TARGET_DIR/
    cp $QT5_DIR/bin/icuuc57.dll              $TARGET_DIR/
    cp $QT5_DIR/bin/libEGL.dll               $TARGET_DIR/
    cp $QT5_DIR/bin/libGLESv2.dll            $TARGET_DIR/
    cp $QT5_DIR/bin/libxml2.dll              $TARGET_DIR/
    cp $QT5_DIR/bin/libxslt.dll              $TARGET_DIR/

    cp $QT5_DIR/bin/QtWebDatabaseProcess.exe $TARGET_DIR/
    cp $QT5_DIR/bin/QtWebNetworkProcess.exe  $TARGET_DIR/
    cp $QT5_DIR/bin/QtWebProcess.exe         $TARGET_DIR/

    cp $QT5_DIR/plugins/scenegraph/qsgd3d12backend.dll $TARGET_DIR/scenegraph/

    cp $QT5_DIR/qml/QtWebKit/qmlwebkitplugin.dll $TARGET_DIR/QtWebKit/
    cp $QT5_DIR/qml/QtWebKit/qmldir $TARGET_DIR/QtWebKit/

    cp $QT5_DIR/qml/QtWebKit/experimental/qmlwebkitexperimentalplugin.dll $TARGET_DIR/QtWebKit/experimental/
    cp $QT5_DIR/qml/QtWebKit/experimental/qmldir $TARGET_DIR/QtWebKit/experimental/
fi

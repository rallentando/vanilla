#include "switch.hpp"

#include "application.hpp"

#ifdef WEBENGINEVIEW
#  include <QtWebEngineQuick>
#  include "settingspage.hpp"
#  include "extensionhost.hpp"
#endif

#ifdef NATIVEWEBVIEW
#  include <QtWebView>
#endif

[[ noreturn ]] static void EmitErrorMessage(std::exception &e){
    qFatal("Error %s", e.what());
}

[[ noreturn ]] static void EmitErrorMessage(){
    qFatal("Error <unknown>");
}

#if defined(Q_OS_MAC)
extern void disableWindowTabbing();
#endif

static int _main(int argc, char **argv){

    QGuiApplication::setHighDpiScaleFactorRoundingPolicy
        (Qt::HighDpiScaleFactorRoundingPolicy::Round);

#if defined(Q_OS_MAC)
    disableWindowTabbing();
#endif
#ifdef WEBENGINEVIEW
    SettingsSchemeHandler::RegisterScheme();
    ExtensionHost::RegisterScheme();
    QtWebEngineQuick::initialize();
#endif
#ifdef NATIVEWEBVIEW
    QtWebView::initialize();
#endif
#ifdef EDGEWEBVIEW
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);
#endif
    Application a(argc, argv);
    Application::BootApplication(argc, argv, &a);
    return a.Run();
}

int main(int argc, char **argv){
    try{
        return _main(argc, argv);
    } catch (std::exception &e){
        EmitErrorMessage(e);
    } catch (...){
        EmitErrorMessage();
    }
}

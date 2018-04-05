#include "switch.hpp"

#include "application.hpp"

static void EmitErrorMessage(std::exception &e){
    qFatal("Error %s", e.what());
}

static void EmitErrorMessage(){
    qFatal("Error <unknown>");
}

#if defined(Q_OS_MAC)
extern void disableWindowTabbing();
#endif

static int _main(int argc, char **argv){

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

#if defined(Q_OS_WIN) // for TridentView.
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);
#endif
#ifdef USE_ANGLE
    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES, true);
    QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL, false);
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL, false);
#endif
#if defined(Q_OS_MAC)
    disableWindowTabbing();
#endif
    Application a(argc, argv);
    Application::BootApplication(argc, argv, &a);
    return a.exec();
}

int main(int argc, char **argv){
#if defined(Q_OS_WIN)
    __try{
        return _main(argc, argv);
    } __except(0){
        EmitErrorMessage();
    }
#else
    try{
        return _main(argc, argv);
    } catch (std::exception &e){
        EmitErrorMessage(e);
    } catch (...){
        EmitErrorMessage();
    }
#endif
    return 1;
}

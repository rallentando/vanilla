#include "switch.hpp"
#include "const.hpp"

#include "saver.hpp"

#include "application.hpp"
#include "mainwindow.hpp"
#include "networkcontroller.hpp"
#include "treebank.hpp"
#include "treebar.hpp"
#include "toolbar.hpp"

#if defined(Q_OS_WIN)
#  include <excpt.h>
#endif

AutoSaver::AutoSaver()
    : QObject(nullptr)
{
    m_IsSaving = false;
}

AutoSaver::~AutoSaver(){}

bool AutoSaver::AutoSaveStart(){
    if(m_IsSaving.exchange(true)) return false;
    m_Timer.start();
    emit Started();
    return true;
}

void AutoSaver::AutoSaveFinish(){
    m_IsSaving = false;
    emit Finished(QStringLiteral("%1 ms").arg(m_Timer.elapsed()));
}

void AutoSaver::AutoSaveFail(){
    m_IsSaving = false;
    emit Failed();
}

void AutoSaver::SaveWindowSettings(){
    foreach(MainWindow *win, Application::GetMainWindows()){
        win->SaveSettings();
    }
}

void AutoSaver::SaveAll(){
    if(!AutoSaveStart()) return;

#if defined(Q_OS_WIN)
#  define TRY __try
#  define CATCH __except(EXCEPTION_EXECUTE_HANDLER)
#else
#  define TRY try
#  define CATCH catch(...)
#endif

    TRY{
        SaveWindowSettings();
    } CATCH {
        AutoSaveFail();
        return;
    }

    TRY{
        TreeBar::SaveSettings();
    } CATCH {
        AutoSaveFail();
        return;
    }

    TRY{
        ToolBar::SaveSettings();
    } CATCH {
        AutoSaveFail();
        return;
    }

    TRY{
        TreeBank::SaveSettings();
    } CATCH {
        AutoSaveFail();
        return;
    }

    TRY{
        Application::SaveGlobalSettings();
    } CATCH {
        AutoSaveFail();
        return;
    }

    TRY{
        Application::SaveSettingsFile();
    } CATCH {
        AutoSaveFail();
        return;
    }


    TRY{
        TreeBank::SaveTree();
    } CATCH {
        AutoSaveFail();
        return;
    }

    TRY{
        Application::SaveIconDatabase();
    } CATCH {
        AutoSaveFail();
        return;
    }

    TRY{
        NetworkController::SaveAllCookies();
    } CATCH {
        AutoSaveFail();
        return;
    }

#undef TRY
#undef CATCH

    AutoSaveFinish();
}

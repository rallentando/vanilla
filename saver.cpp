#include "switch.hpp"
#include "const.hpp"

#include "saver.hpp"

#include "application.hpp"
#include "mainwindow.hpp"
#include "networkcontroller.hpp"
#include "treebank.hpp"
#include "treebar.hpp"
#include "toolbar.hpp"

AutoSaver::AutoSaver()
    : QObject(0)
{
    m_IsSaving = false;
}

AutoSaver::~AutoSaver(){}

void AutoSaver::AutoSaveStart(){
    m_IsSaving = true;
    m_Time.start();
    emit Started();
}

void AutoSaver::AutoSaveFinish(){
    m_IsSaving = false;
    emit Finished(QStringLiteral("%1 ms").arg(m_Time.elapsed()));
}

void AutoSaver::SaveWindowSettings(){
    foreach(MainWindow *win, Application::GetMainWindows()){
        win->SaveSettings();
    }
}

void AutoSaver::SaveAll(){
    AutoSaveStart();

#if defined(Q_OS_WIN)
#  define TRY __try
#  define CATCH __except(0)
#else
#  define TRY try
#  define CATCH catch(...)
#endif

    // save window settings.
    TRY{
        SaveWindowSettings();
    } CATCH{
        emit Failed();
        return;
    }

    // save global settings.
    TRY{
        Application::SaveGlobalSettings();
    } CATCH{
        emit Failed();
        return;
    }

    // save treebar settings.
    TRY{
        TreeBar::SaveSettings();
    } CATCH{
        emit Failed();
        return;
    }

    // save toolbar settings.
    TRY{
        ToolBar::SaveSettings();
    } CATCH{
        emit Failed();
        return;
    }

    // save treebank settings.
    TRY{
        TreeBank::SaveSettings();
    } CATCH{
        emit Failed();
        return;
    }

    // save all tree.
    TRY{
        TreeBank::SaveTree();
    } CATCH{
        emit Failed();
        return;
    }

    // save all cookies.
    TRY{
        NetworkController::SaveAllCookies();
    } CATCH{
        emit Failed();
        return;
    }

#undef TRY
#undef CATCH

    AutoSaveFinish();
}

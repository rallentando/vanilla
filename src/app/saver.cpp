#include "switch.hpp"
#include "const.hpp"

#include "saver.hpp"

#include <QCoreApplication>
#include <QPointer>
#include <QThread>
#include <QThreadPool>

#include <utility>

#include "application.hpp"
#include "mainwindow.hpp"
#include "networkcontroller.hpp"
#include "treebank.hpp"
#include "treebar.hpp"
#include "toolbar.hpp"

#if defined(Q_OS_WIN)
#  include <excpt.h>
#endif

AutoSaver::AutoSaver(QObject *parent)
    : AutoSaver(&AutoSaver::CaptureProductionSnapshot,
                &AutoSaver::WriteProductionSnapshot, parent) {}

AutoSaver::AutoSaver(SnapshotMaker capture, SnapshotWriter write, QObject *parent)
    : QObject(parent)
    , m_Capture(std::move(capture))
    , m_Write(std::move(write)) {}

AutoSaver::~AutoSaver(){}

bool AutoSaver::BeginSave(){
    Q_ASSERT(!QCoreApplication::instance() ||
             QThread::currentThread() == QCoreApplication::instance()->thread());
    if(!m_Flow.Request()) return false;
    m_Timer.start();
    emit Started();
    return true;
}

bool AutoSaver::CaptureProductionSnapshot(SaveSnapshot *snapshot){
    if(!snapshot) return false;
    TreeBank::DoDelete();
    TreeBank::UpdateCurrentThumbnails();
    foreach(MainWindow *win, Application::GetMainWindows()){
        win->SaveSettings();
    }
    TreeBar::SaveSettings();
    ToolBar::SaveSettings();
    TreeBank::SaveSettings();
    Application::SaveGlobalSettings();

    snapshot->settings = Application::GlobalSettings();
    snapshot->icons = Application::IconDatabaseSnapshot();
    snapshot->cookies = NetworkController::CookieSnapshot();
    snapshot->windowIndices = TreeBank::WindowIndexSnapshot();
    return true;
}

void AutoSaver::SaveAll(){
    if(!BeginSave()) return;

    SaveSnapshot snapshot;
    bool success = false;
    try {
        success = m_Capture && m_Capture(&snapshot) && m_Write && m_Write(snapshot);
    } catch(...) {
        success = false;
    }
    SettleSave(success);
}

void AutoSaver::SaveAllAsync(){
    if(!BeginSave()) return;
    StartAsyncWrite();
}

void AutoSaver::StartAsyncWrite(){
    SaveSnapshot snapshot;
    bool captured = false;
    try {
        captured = m_Capture && m_Capture(&snapshot);
    } catch(...) {
        captured = false;
    }
    if(!captured){
        SettleSave(false);
        return;
    }

    QPointer<AutoSaver> saver(this);
    const SnapshotWriter write = m_Write;
    QThreadPool::globalInstance()->start([saver, snapshot, write](){
        bool success = false;
        try {
            success = write && write(snapshot);
        } catch(...) {
            success = false;
        }
        if(!saver) return;
        QMetaObject::invokeMethod(saver, [saver, success](){
            if(saver) saver->SettleSave(success);
        }, Qt::QueuedConnection);
    });
}

void AutoSaver::SettleSave(bool success){
    Q_ASSERT(!QCoreApplication::instance() ||
             QThread::currentThread() == QCoreApplication::instance()->thread());
    switch(m_Flow.Settle(success)){
    case SaveFlow::SettleAction::StartPending:
        StartAsyncWrite();
        return;
    case SaveFlow::SettleAction::Finished:
        emit Finished(QStringLiteral("%1 ms").arg(m_Timer.elapsed()));
        return;
    case SaveFlow::SettleAction::Failed:
        emit Failed();
        return;
    }
}

bool AutoSaver::WriteProductionSnapshot(const SaveSnapshot &snapshot){

#if defined(Q_OS_WIN)
#  define TRY __try
#  define CATCH __except(EXCEPTION_EXECUTE_HANDLER)
#else
#  define TRY try
#  define CATCH catch(...)
#endif

    SaveStageFlow stages;

    TRY{
        stages.Run([&](){ return Application::SaveSettingsFile(snapshot.settings);});
    } CATCH {
        stages.Fail();
    }

    TRY{
        stages.Run([&](){ return TreeBank::SaveTree(snapshot.windowIndices);});
    } CATCH {
        stages.Fail();
    }

    TRY{
        stages.Run([&](){ return Application::SaveIconDatabase(snapshot.icons);});
    } CATCH {
        stages.Fail();
    }

    TRY{
        stages.Run([&](){ return NetworkController::SaveCookieSnapshot(snapshot.cookies);});
    } CATCH {
        stages.Fail();
    }

#undef TRY
#undef CATCH

    return stages.Succeeded();
}

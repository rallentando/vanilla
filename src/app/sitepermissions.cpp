#include "switch.hpp"

#include "sitepermissions.hpp"

#include "application.hpp"
#include "networkcontroller.hpp"
#ifdef EDGEWEBVIEW
#  include "edgewebview.hpp"
#endif

#include <QFile>
#include <QSaveFile>

namespace {

    QString TablePath(){
        return Application::StateDirectory() + QStringLiteral("permissions.json");
    }

    PermissionTable &TheTable(){
        static PermissionTable *table = nullptr;
        if(!table){
            QByteArray json;
            QFile file(TablePath());
            if(file.open(QIODevice::ReadOnly)) json = file.readAll();
            file.close();

            bool broken = false;
            table = new PermissionTable(PermissionTable::FromJson(json, &broken));
            if(broken){
                const QString aside = TablePath() + QStringLiteral(".broken");
                QFile::remove(aside);
                QFile::copy(TablePath(), aside);
                qWarning() << "permissions.json did not read; the original is"
                           << aside;
            }
        }
        return *table;
    }

    void Write(){
        QSaveFile file(TablePath());
        if(!file.open(QIODevice::WriteOnly)){
            qWarning() << "permissions.json could not be written:" << file.errorString();
            return;
        }
        file.write(TheTable().ToJson());
        if(!file.commit())
            qWarning() << "permissions.json could not be written:" << file.errorString();
    }
}

PermissionTable::State SitePermissions::Lookup(const QUrl &origin, const QString &kind){
    return TheTable().Lookup(PermissionTable::Origin(origin), kind);
}

void SitePermissions::Remember(const QUrl &origin, const QStringList &kinds,
                               PermissionTable::State state, bool offTheRecord){
    if(offTheRecord) return;
    if(!Application::GlobalSettings()
       .value(QStringLiteral("network/@RememberPermissions"), true).value<bool>()) return;

    const PermissionTable before = TheTable();
    bool changed = false;
    foreach(const QString &kind, kinds)
        changed = TheTable().Set(PermissionTable::Origin(origin), kind, state) || changed;
    if(changed) Changed(before);
}

bool SitePermissions::Set(const QString &origin, const QString &kind, PermissionTable::State state){
    const PermissionTable before = TheTable();
    if(!TheTable().Set(origin, kind, state)) return false;
    Changed(before);
    return true;
}

bool SitePermissions::Clear(){
    const PermissionTable before = TheTable();
    if(!TheTable().Clear()) return false;
    Changed(before);
    return true;
}

const PermissionTable &SitePermissions::Table(){
    return TheTable();
}

void SitePermissions::Changed(const PermissionTable &before){
    Write();
    NetworkController::MirrorPermissions(TheTable().Delta(before));
#ifdef EDGEWEBVIEW
    EdgeWebView::MirrorPermissions();
#endif
}

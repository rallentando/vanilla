#ifndef SAVER_HPP
#define SAVER_HPP

#include "switch.hpp"
#include "application.hpp"
#include "saveflow.hpp"

#include <QObject>
#include <QByteArray>
#include <QElapsedTimer>
#include <QMap>

#include <functional>

class ViewNode;

struct SaveSnapshot {
    Settings settings;
    Settings icons;
    QByteArray cookies;
    QMap<ViewNode*, int> windowIndices;
};

class AutoSaver : public QObject {
    Q_OBJECT

public:
    typedef std::function<bool(SaveSnapshot*)> SnapshotMaker;
    typedef std::function<bool(const SaveSnapshot&)> SnapshotWriter;

    explicit AutoSaver(QObject *parent = nullptr);
    AutoSaver(SnapshotMaker capture, SnapshotWriter write, QObject *parent = nullptr);
    ~AutoSaver();

    bool IsSaving() const { return m_Flow.IsSaving();}

private:
    QElapsedTimer m_Timer;
    SaveFlow m_Flow;
    SnapshotMaker m_Capture;
    SnapshotWriter m_Write;

    bool BeginSave();
    void SettleSave(bool success);
    void StartAsyncWrite();

    static bool CaptureProductionSnapshot(SaveSnapshot *snapshot);
    static bool WriteProductionSnapshot(const SaveSnapshot &snapshot);

signals:
    void Started();
    void Failed();
    void Finished(const QString & = QString());

public slots:
    void SaveAll();
    void SaveAllAsync();
};

#endif

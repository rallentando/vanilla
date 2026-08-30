#ifndef SAVER_HPP
#define SAVER_HPP

#include "switch.hpp"
#include "application.hpp"

#include <QObject>
#include <QElapsedTimer>

#include <atomic>

class AutoSaver : public QObject {
    Q_OBJECT

public:
    AutoSaver();
    ~AutoSaver();

    bool IsSaving() const { return m_IsSaving;}

private:
    QElapsedTimer m_Timer;
    std::atomic_bool m_IsSaving;
    bool AutoSaveStart();
    void AutoSaveFinish();
    void AutoSaveFail();
    void SaveWindowSettings();

signals:
    void Started();
    void Failed();
    void Finished(const QString & = QString());

public slots:
    void SaveAll();
};

#endif

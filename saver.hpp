#ifndef SAVER_HPP
#define SAVER_HPP

#include "switch.hpp"
#include "application.hpp"

#include <QObject>
#include <QTime>

class AutoSaver : public QObject{
    Q_OBJECT

public:
    AutoSaver();
    ~AutoSaver();

    bool IsSaving() const { return m_IsSaving;}

private:
    QTime m_Time;
    bool m_IsSaving;
    void AutoSaveStart();
    void AutoSaveFinish();
    void SaveWindowSettings();

signals:
    void Started();
    void Failed();
    void Finished(const QString & = QString());

public slots:
    void SaveAll();
};

#endif

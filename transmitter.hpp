#ifndef TRANSMITTER_HPP
#define TRANSMITTER_HPP

#include "switch.hpp"

#include <QObject>

class QString;
class QLocalSocket;

class Transmitter : public QObject{
    Q_OBJECT

public:
    Transmitter(QObject *parent = 0);
    ~Transmitter();

    bool ServerAlreadyExists();
    void SendCommand(QString);
    void SendCommandAndQuit(QString);

public slots:
    void Quit();

private:
    QLocalSocket *m_LocalSocket;
};

#endif //ifndef TRANSMITTER_HPP

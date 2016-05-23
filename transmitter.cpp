#include "switch.hpp"
#include "const.hpp"

#include "transmitter.hpp"

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>

#include "application.hpp"
#include "receiver.hpp"

Transmitter::Transmitter(QObject *parent)
    : QObject(parent)
{
    m_LocalSocket = new QLocalSocket();
    connect(m_LocalSocket, &QLocalSocket::disconnected,
            m_LocalSocket, &QLocalSocket::deleteLater);
}

Transmitter::~Transmitter(){}

bool Transmitter::ServerAlreadyExists(){
    m_LocalSocket->connectToServer(Application::LocalServerName());
    return m_LocalSocket->waitForConnected(100);
}

void Transmitter::SendCommand(QString command){
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

#if QT_VERSION >= 0x050700
    out.setVersion(QDataStream::Qt_5_7);
#else
    out.setVersion(QDataStream::Qt_5_6);
#endif

    out << QString(command);
    out.device()->seek(0);
    m_LocalSocket->write(block);
    m_LocalSocket->flush();
}

void Transmitter::SendCommandAndQuit(QString command){
    SendCommand(command);
    QTimer::singleShot(100, this, &Transmitter::Quit);
}

void Transmitter::Quit(){
    disconnect();
    Application::quit();
}

#include "switch.hpp"
#include "const.hpp"

#include "transmitter.hpp"

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>

#include "application.hpp"
#include "receiver.hpp"

#if defined(Q_OS_WIN)
#  include <windows.h>
#elif defined(Q_OS_LINUX)
#  include <sys/socket.h>
#  include <unistd.h>
#elif defined(Q_OS_MACOS)
#  include <unistd.h>
#endif

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
    if(!m_LocalSocket->waitForConnected(100)) return false;
    if(ServerIsSameUser(m_LocalSocket)) return true;
    m_LocalSocket->abort();
    return false;
}

#if defined(Q_OS_WIN)
namespace {
    QByteArray UserOf(HANDLE token){
        DWORD size = 0;
        GetTokenInformation(token, TokenUser, nullptr, 0, &size);
        if(!size) return QByteArray();
        QByteArray buffer(int(size), Qt::Uninitialized);
        if(!GetTokenInformation(token, TokenUser, buffer.data(), size, &size)) return QByteArray();
        const PSID sid = reinterpret_cast<TOKEN_USER*>(buffer.data())->User.Sid;
        if(!IsValidSid(sid)) return QByteArray();
        return QByteArray(reinterpret_cast<const char*>(sid), int(GetLengthSid(sid)));
    }
    QByteArray UserOfProcess(HANDLE process){
        HANDLE token = nullptr;
        if(!OpenProcessToken(process, TOKEN_QUERY, &token)) return QByteArray();
        const QByteArray user = UserOf(token);
        CloseHandle(token);
        return user;
    }
}
#endif

bool Transmitter::ServerIsSameUser(QLocalSocket *socket){
    if(!socket || socket->state() != QLocalSocket::ConnectedState) return false;
#if defined(Q_OS_WIN)
    ULONG pid = 0;
    if(!GetNamedPipeServerProcessId(reinterpret_cast<HANDLE>(socket->socketDescriptor()), &pid) || !pid)
        return false;
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if(!process) return false;
    const QByteArray theirs = UserOfProcess(process);
    CloseHandle(process);
    const QByteArray ours = UserOfProcess(GetCurrentProcess());
    return !ours.isEmpty() && theirs == ours;
#elif defined(Q_OS_LINUX)
    struct ucred credentials;
    socklen_t length = sizeof(credentials);
    if(getsockopt(int(socket->socketDescriptor()), SOL_SOCKET, SO_PEERCRED, &credentials, &length) != 0)
        return false;
    return credentials.uid == geteuid();
#elif defined(Q_OS_MACOS)
    uid_t uid = 0;
    gid_t gid = 0;
    if(getpeereid(int(socket->socketDescriptor()), &uid, &gid) != 0) return false;
    return uid == geteuid();
#else
    return true;
#endif
}

void Transmitter::SendCommand(QString command){
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

    out.setVersion(QDataStream::Qt_DefaultCompiledVersion);

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

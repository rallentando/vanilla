#include "commandframe.hpp"

#include <QDataStream>
#include <QIODevice>

#include <limits>

namespace CommandFrame {

Result Read(QIODevice *device, QString *command, quint32 maximumPayloadBytes){
    if(!device || !command) return Invalid;

    constexpr qint64 HeaderBytes = sizeof(quint32);
    if(device->bytesAvailable() < HeaderBytes) return Incomplete;

    const QByteArray header = device->peek(HeaderBytes);
    QDataStream headerStream(header);
    headerStream.setVersion(QDataStream::Qt_DefaultCompiledVersion);

    quint32 payloadBytes = 0;
    headerStream >> payloadBytes;
    if(headerStream.status() != QDataStream::Ok ||
       payloadBytes == std::numeric_limits<quint32>::max() ||
       payloadBytes % sizeof(char16_t) != 0)
        return Invalid;
    if(payloadBytes > maximumPayloadBytes) return TooLarge;
    if(device->bytesAvailable() < HeaderBytes + payloadBytes) return Incomplete;

    QDataStream stream(device);
    stream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    stream.startTransaction();
    QString value;
    stream >> value;
    if(!stream.commitTransaction()) return Invalid;

    *command = value;
    return Complete;
}

}

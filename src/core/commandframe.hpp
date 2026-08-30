#ifndef COMMANDFRAME_HPP
#define COMMANDFRAME_HPP

#include <QString>

class QIODevice;

namespace CommandFrame {

enum Result {
    Incomplete,
    Complete,
    Invalid,
    TooLarge,
};

static constexpr quint32 MaximumPayloadBytes = 1024U * 1024U;

Result Read(QIODevice *device, QString *command,
            quint32 maximumPayloadBytes = MaximumPayloadBytes);

}

#endif

#ifndef OWNEDRAISE_HPP
#define OWNEDRAISE_HPP

#include "switch.hpp"

#include <QtGlobal>

#if defined(Q_OS_WIN)
struct tagWINDOWPOS;
#endif

namespace OwnedRaise {
#if defined(Q_OS_WIN)
    bool Changing(tagWINDOWPOS *pos);
    using ForegroundSource = quintptr (*)();
    void SetForegroundSource(ForegroundSource source);
#endif

    class OwnerRaise {
    public:
        explicit OwnerRaise(quintptr owner);
        ~OwnerRaise();
        OwnerRaise(const OwnerRaise &) = delete;
        OwnerRaise &operator=(const OwnerRaise &) = delete;
    private:
        quintptr m_Previous;
    };
    void Install();
}

#endif

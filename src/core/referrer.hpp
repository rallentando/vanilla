#ifndef REFERRER_HPP
#define REFERRER_HPP

#include "switch.hpp"

#include <QByteArray>
#include <QUrl>

namespace Referrer {

QByteArray ForNavigation(const QUrl &referrer, const QUrl &target);

}

#endif

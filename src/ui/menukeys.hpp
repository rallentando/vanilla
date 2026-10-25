#ifndef MENUKEYS_HPP
#define MENUKEYS_HPP

#include "switch.hpp"

#include <QHash>
#include <QKeySequence>
#include <QString>

class QMenu;

namespace MenuKeys {

    void Show(QMenu *menu, const QHash<QString, QKeySequence> &keys);
}

#endif

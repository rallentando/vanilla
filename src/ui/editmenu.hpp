#ifndef EDITMENU_HPP
#define EDITMENU_HPP

#include "switch.hpp"

#include <QStringList>

class QLineEdit;
class QMenu;

namespace EditMenu {

    QStringList Names();

    QMenu *Build(QLineEdit *edit, const QStringList &names);

    QStringList NamesFor(QLineEdit *edit);

    void Install(QLineEdit *edit);
}

#endif

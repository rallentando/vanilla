#ifndef EDGEMENUITEM_HPP
#define EDGEMENUITEM_HPP

#include "switch.hpp"

#ifdef EDGEWEBVIEW

#include <QList>
#include <QString>
#include <functional>

struct EdgeMenuItem {
    enum class Kind { Command, CheckBox, Radio, Separator, Submenu };

    QString label;
    int commandId = -1;
    Kind kind = Kind::Command;
    bool enabled = true;
    bool checked = false;
    QList<EdgeMenuItem> children;
};

class QMenu;

void AddEdgeMenuItems(QMenu *menu, const QList<EdgeMenuItem> &items,
                      const std::function<void(int)> &chosen);

#endif
#endif

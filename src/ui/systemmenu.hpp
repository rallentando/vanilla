#ifndef SYSTEMMENU_HPP
#define SYSTEMMENU_HPP

#include "switch.hpp"

#include <QList>
#include <QString>
#include <QtGui/qwindowdefs.h>

class QMenu;
class QAction;

namespace SystemMenu {

constexpr int FirstId = 0x0100;
constexpr int IdStep  = 0x0010;
constexpr int LastId  = 0xF000 - IdStep;
constexpr int MarkId  = FirstId - 1;

int IdOf(int index);
int IndexOf(quintptr command);
bool IsOurs(int id);

QString Label(const QAction *action);

struct Entry {
    enum Kind { Command, Separator, Submenu };
    Kind kind;
    QString label;
    bool enabled;
    bool checked;
    int id;
    QList<Entry> children;
};

QList<Entry> Entries(const QMenu *menu, QList<QAction*> &actions);

void ApplyScheme(bool dark);

void Install(WId window, const QList<Entry> &entries);

}

#endif

#include "switch.hpp"

#include "menukeys.hpp"
#include "inputmap.hpp"

#include <QAction>
#include <QWidgetAction>
#include <QEvent>
#include <QMenu>

namespace MenuKeys {

namespace {

class Mute : public QObject {
public:
    explicit Mute(QAction *action) : QObject(action){
        setObjectName(QStringLiteral("MenuKeys::Mute"));
        action->installEventFilter(this);
    }
    bool eventFilter(QObject *, QEvent *ev) override {
        if(ev->type() != QEvent::Shortcut) return false;
        ev->accept();
        return true;
    }
};

void MuteOnce(QAction *action){
    if(action->findChild<QObject*>(QStringLiteral("MenuKeys::Mute"), Qt::FindDirectChildrenOnly)) return;
    new Mute(action);
}

void ShowOn(QAction *action, const QHash<QString, QKeySequence> &keys){
    if(action->isSeparator()) return;
    const QString name = action->objectName();
    if(name.isEmpty()) return;
    action->setShortcut(InputMap::DisplayKey(keys.value(name)));
    action->setShortcutContext(Qt::WidgetShortcut);
    action->setShortcutVisibleInContextMenu(true);
    MuteOnce(action);
}

}

void Show(QMenu *menu, const QHash<QString, QKeySequence> &keys){
    if(!menu) return;
    foreach(QAction *action, menu->actions()){
        if(QMenu *sub = action->menu()){
            Show(sub, keys);
            continue;
        }
        if(QWidgetAction *row = qobject_cast<QWidgetAction*>(action)){
            if(QWidget *widget = row->defaultWidget())
                foreach(QAction *inner, widget->actions()) ShowOn(inner, keys);
            continue;
        }
        ShowOn(action, keys);
    }
}

}

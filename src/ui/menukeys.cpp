#include "switch.hpp"

#include "menukeys.hpp"
#include "inputmap.hpp"

#include <QAction>
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
    bool eventFilter(QObject *, QEvent *ev) Q_DECL_OVERRIDE {
        if(ev->type() != QEvent::Shortcut) return false;
        ev->accept();
        return true;
    }
};

void MuteOnce(QAction *action){
    if(action->findChild<QObject*>(QStringLiteral("MenuKeys::Mute"), Qt::FindDirectChildrenOnly)) return;
    new Mute(action);
}

}

void Show(QMenu *menu, const QHash<QString, QKeySequence> &keys){
    if(!menu) return;
    foreach(QAction *action, menu->actions()){
        if(QMenu *sub = action->menu()){
            Show(sub, keys);
            continue;
        }
        if(action->isSeparator()) continue;
        const QString name = action->objectName();
        if(name.isEmpty()) continue;
        action->setShortcut(InputMap::DisplayKey(keys.value(name)));
        action->setShortcutContext(Qt::WidgetShortcut);
        action->setShortcutVisibleInContextMenu(true);
        MuteOnce(action);
    }
}

}

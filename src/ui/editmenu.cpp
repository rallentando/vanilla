#include "switch.hpp"

#include "editmenu.hpp"

#include "view.hpp"

#include <QAction>
#include <QClipboard>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QKeySequence>
#include <QLineEdit>
#include <QMenu>
#include <QMimeData>

namespace {

    const QString Separator = QStringLiteral("Separator");

    struct Edit {
        const char *name;
        QKeySequence::StandardKey key;
        bool (*enabled)(QLineEdit*);
        void (*act)(QLineEdit*);
    };

    bool Selected(QLineEdit *edit){ return edit->hasSelectedText();}
    bool Copyable(QLineEdit *edit){ return Selected(edit) && edit->echoMode() == QLineEdit::Normal;}
    bool Writable(QLineEdit *edit){ return !edit->isReadOnly();}
    bool Pastable(QLineEdit *edit){
        const QMimeData *data = QGuiApplication::clipboard()->mimeData();
        return Writable(edit) && data && data->hasText();
    }

    const Edit Edits[] = {
        { "Undo",      QKeySequence::Undo,      [](QLineEdit *e){ return e->isUndoAvailable();}, [](QLineEdit *e){ e->undo();} },
        { "Redo",      QKeySequence::Redo,      [](QLineEdit *e){ return e->isRedoAvailable();}, [](QLineEdit *e){ e->redo();} },
        { "Cut",       QKeySequence::Cut,       [](QLineEdit *e){ return Copyable(e) && Writable(e);}, [](QLineEdit *e){ e->cut();} },
        { "Copy",      QKeySequence::Copy,      Copyable, [](QLineEdit *e){ e->copy();} },
        { "Paste",     QKeySequence::Paste,     Pastable, [](QLineEdit *e){ e->paste();} },
        { "PasteAndMatchStyle", QKeySequence::UnknownKey, Pastable, [](QLineEdit *e){ e->paste();} },
        { "Delete",    QKeySequence::Delete,    [](QLineEdit *e){ return Selected(e) && Writable(e);}, [](QLineEdit *e){ e->del();} },
        { "SelectAll", QKeySequence::SelectAll, [](QLineEdit *e){ return !e->text().isEmpty();}, [](QLineEdit *e){ e->selectAll();} },
    };

    const Edit *Find(const QString &name){
        for(const Edit &edit : Edits)
            if(name == QLatin1String(edit.name)) return &edit;
        return nullptr;
    }
}

namespace EditMenu {

QStringList Names(){
    return View::GetTextMenu().split(QLatin1Char(','));
}

QStringList NamesFor(QLineEdit *edit){
    QStringList names = Names();
    if(edit->hasSelectedText()) names << Separator << QStringLiteral("Copy");
    return names;
}

QMenu *Build(QLineEdit *edit, const QStringList &names){
    QMenu *menu = new QMenu();
    menu->setToolTipsVisible(true);
    foreach(const QString &name, names){
        if(name == Separator){
            if(!menu->isEmpty()) menu->addSeparator();
            continue;
        }
        const Edit *edit_ = Find(name);
        if(!edit_) continue;
        QAction *action = menu->addAction(QCoreApplication::translate("Page", edit_->name));
        action->setObjectName(QLatin1String(edit_->name));
        action->setEnabled(edit_->enabled(edit));
        if(edit_->key != QKeySequence::UnknownKey){
            action->setShortcut(QKeySequence(edit_->key));
            action->setShortcutContext(Qt::WidgetShortcut);
            action->setShortcutVisibleInContextMenu(true);
        }
        void (*act)(QLineEdit*) = edit_->act;
        QObject::connect(action, &QAction::triggered, edit, [edit, act](){ act(edit);});
    }
    return menu;
}

void Install(QLineEdit *edit){
    edit->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(edit, &QWidget::customContextMenuRequested, edit, [edit](const QPoint &pos){
        QMenu *menu = Build(edit, NamesFor(edit));
        if(!menu->isEmpty()) menu->exec(edit->mapToGlobal(pos));
        delete menu;
    });
}

}

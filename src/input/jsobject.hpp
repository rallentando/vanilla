#ifndef JSOBJECT_HPP
#define JSOBJECT_HPP

#include "switch.hpp"

#include <QObject>
#include <QInputDialog>
#include <QTimer>

#include "treebank.hpp"
#include "view.hpp"
#include "dialog.hpp"

class _Vanilla : public QObject {
    Q_OBJECT

private:
    TreeBank *m_TreeBank;

public:
    _Vanilla(TreeBank *tbank) : QObject(nullptr){
        m_TreeBank = tbank;
    }

public slots:
    int     getInt   (QString title, QString label, int    val = 0, int    min = INT_MIN, int    max = INT_MAX, int     step = 1){
        return ModalDialog::GetInt(title, label, val, min, max, step);
    }

    double  getDouble(QString title, QString label, double val = 0, double min = DBL_MIN, double max = DBL_MAX, int decimals = 1){
        return ModalDialog::GetDouble(title, label, val, min, max, decimals);
    }

    QString getItem  (QString title, QString label, QStringList items, bool editable = true){
        return ModalDialog::GetItem(title, label, items, editable);
    }

    QString getText  (QString title, QString label, QString text = QString()){
        return ModalDialog::GetText(title, label, text);
    }

#define VANILLA_JS_METHOD(method, jsName) void jsName(){ m_TreeBank->method(); }
    FOR_EACH_VANILLA_JS_METHOD(VANILLA_JS_METHOD)
#undef VANILLA_JS_METHOD

    void quit(){ QTimer::singleShot(0, m_TreeBank, SLOT(Quit()));}
    void close(){ QTimer::singleShot(0, m_TreeBank, SLOT(Close())); }
    void recreate(){ QTimer::singleShot(0, m_TreeBank, SLOT(Recreate())); }

#define VANILLA_EDIT_ACTION(name, jsName) void jsName(){ m_TreeBank->name();}
    FOR_EACH_EDIT_EVENTS_WITH_JS_NAME(VANILLA_EDIT_ACTION)
#undef VANILLA_EDIT_ACTION
};

class _View : public QObject {
    Q_OBJECT

private:
    View *m_View;

public:
    _View(View *view) : QObject(nullptr){
        m_View = view;
    }

public slots:
#define VANILLA_JS_ACTION(action, jsName) \
    void jsName(){ m_View->TriggerAction(Page::_##action); }
    FOR_EACH_VIEW_JS_ACTION(VANILLA_JS_ACTION)
#undef VANILLA_JS_ACTION

    void quit(){ QTimer::singleShot(0, m_View->GetTreeBank(), SLOT(Quit()));}
    void close(){ QTimer::singleShot(0, m_View->GetTreeBank(), SLOT(Close())); }
    void recreate(){ QTimer::singleShot(0, m_View->GetTreeBank(), SLOT(Recreate())); }
};

#endif

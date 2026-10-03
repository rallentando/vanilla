#ifndef SIDEPANELS_HPP
#define SIDEPANELS_HPP

#include "switch.hpp"

#include <QHash>
#include <QList>
#include <QObject>
#include <QPair>
#include <QPointer>
#include <QString>
#include <QUrl>

class ExtensionController;
class MainWindow;
class QDockWidget;
class QStackedWidget;
class QWidget;
class View;

class SidePanels : public QObject {
    Q_OBJECT

public:
    explicit SidePanels(MainWindow *window);
    ~SidePanels();
    QList<QDockWidget*> Docks() const;

    static QString Open(ExtensionController *controller, const QString &id, qint64 tab = 0);
    static QString Close(ExtensionController *controller, const QString &id, qint64 tab = 0);
    QString Toggle(ExtensionController *controller, const QString &id);
    int Placed() const { return m_Placed;}
    void SetPlaced(int placed){ m_Placed = placed;}
    static void UpdateAll();
    static qint64 ClosedByUserAt(const ExtensionController *controller, const QString &id);

    void Update();
    void ShutdownPages();
    static void ShutdownEverywhere();

protected:
    bool eventFilter(QObject *watched, QEvent *ev) Q_DECL_OVERRIDE;

private:
    struct Panel {
        QPointer<ExtensionController> controller;
        QString id;
        qint64 tab = 0;
        QString version;
        QString path;
        QUrl url;
        QPointer<QWidget> page;
    };
    bool Shows(const ExtensionController *controller) const;
    int IndexOf(const ExtensionController *controller) const;
    int IndexOfTab(qint64 tab) const;
    int IndexOfPage(const QWidget *page) const;
    QString OpenHere(ExtensionController *controller, const QString &id, const QUrl &url, qint64 tab, View *from);
    void Drop(int index, bool byUser);
    void ClosePage(const QWidget *page, bool byUser);
    void Show(QDockWidget *dock, QStackedWidget *stack, const Panel *panel);
    void Prune();
    static QList<SidePanels*> &All();

    MainWindow *m_Window;
    QDockWidget *m_Dock;
    QStackedWidget *m_Stack;
    QDockWidget *m_TabDock;
    QStackedWidget *m_TabStack;
    QPointer<QWidget> m_Raise;
    QList<Panel> m_Panels;
    QHash<QPair<const void*, qint64>, quint64> m_Making;
    quint64 m_Serial = 0;
    int m_Placed = 0;
};

#endif

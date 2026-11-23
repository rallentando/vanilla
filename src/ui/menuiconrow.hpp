#ifndef MENUICONROW_HPP
#define MENUICONROW_HPP

#include "switch.hpp"

#include <QWidget>
#include <QIcon>
#include <QList>
#include <QPointer>
#include <QStringList>

class QMenu;
class QAction;
class QWidgetAction;

class MenuIconRow : public QWidget {
    Q_OBJECT

public:
    explicit MenuIconRow(QMenu *menu);

    QWidgetAction *Install();
    static MenuIconRow *Of(QMenu *menu);

    void AddCell(const QString &name, QAction *action, const QString &icon, bool enabled = true);
    void AddCommandCell(QAction *action);
    void AddDivider();
    int CellCount() const { return m_Cells.length(); }
    bool Replace(QAction *from, QAction *to);
    bool Runs(const QAction *action) const {
        for(const Cell &cell : m_Cells) if(cell.action == action) return true;
        return false;
    }
    bool Holds(const QString &name) const { return m_Names.contains(name); }

    static QIcon Icon(const QString &icon, const QWidget *widget);
    static QString IconOf(const QString &name);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void leaveEvent(QEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void focusInEvent(QFocusEvent *ev) override;
    void focusOutEvent(QFocusEvent *ev) override;
    bool event(QEvent *ev) override;

private:
    struct Cell {
        QPointer<QAction> action;
        QString icon;
        bool enabled;
        bool command;
        bool divided;
    };
    int Scaled(int length) const;
    int Gap() const;
    int CellWidth() const;
    QRect CellRect(int index) const;
    int CellAt(const QPoint &pos) const;
    bool Chosen() const;
    int Next(int from, int step) const;
    void Run(int index);

    QMenu *m_Menu;
    QWidgetAction *m_Action;
    QList<Cell> m_Cells;
    QStringList m_Names;
    bool m_Divide;
    int m_Current;
    int m_Hovered;
};

#endif

#ifndef PASSWORDSUGGESTIONS_HPP
#define PASSWORDSUGGESTIONS_HPP

#include "switch.hpp"

#include <QStringList>
#include <QWidget>

class PasswordSuggestions : public QWidget {
    Q_OBJECT

public:
    PasswordSuggestions(QWidget *window, const QString &manage);

    void SetAccounts(const QStringList &accounts);

    int Rows() const { return m_Accounts.size() + 1;}
    int ManageRow() const { return m_Accounts.size();}
    int Chosen() const { return m_Chosen;}
    void MoveChoice(int step);
    void ClearChoice();

    void ShowAt(const QPoint &at);

signals:
    void chosen(int row);

protected:
    void paintEvent(QPaintEvent *ev) override;
    void showEvent(QShowEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    QSize sizeHint() const override;

private:
    int Padding() const;
    int RowHeight(int row) const;
    int SeparatorHeight() const;
    QRect RowRect(int row) const;
    int RowAt(const QPoint &pos) const;
    QFont NameFont() const;
    void SetChoice(int row);

    QStringList m_Accounts;
    QString m_Manage;
    int m_Chosen = -1;
    bool m_Translucent;
};

#endif

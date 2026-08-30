#ifndef RECEIVER_HPP
#define RECEIVER_HPP

#include "switch.hpp"

#include "devicescale.hpp"

#include <QWidget>
#include <QLineEdit>

#include "view.hpp"

class QHideEvent;
class QShowEvent;
class QKeyEvent;
class QKeySequence;
class QFocusEvent;
class QPaintEvent;
class QTimerEvent;
class QLocalServer;
class QLocalSocket;

class TreeBank;
class View;

class LineEdit : public QLineEdit {
    Q_OBJECT

public:
    LineEdit(QWidget *parent = nullptr);
    ~LineEdit() Q_DECL_OVERRIDE;

    void ApplyTheme();

    template <class T> T ScaleByDevice(T t) const {
        return DeviceScale::FromDpi(t, static_cast<int>(logicalDpiY()));
    }

protected:
    void focusInEvent(QFocusEvent *ev) Q_DECL_OVERRIDE;
    void focusOutEvent(QFocusEvent *ev) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE;
    void inputMethodEvent(QInputMethodEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;

signals:
    void Returned();
    void Aborted();
    void FocusIn(Qt::FocusReason);
    void FocusOut(Qt::FocusReason);
    void SelectNextSuggest();
    void SelectPrevSuggest();
};

class Receiver : public QWidget {
    Q_OBJECT

public:
    Receiver(TreeBank *parent = nullptr, bool purge = false);
    ~Receiver() Q_DECL_OVERRIDE;

    template <class T> T ScaleByDevice(T t) const {
        return DeviceScale::FromDpi(t, static_cast<int>(logicalDpiY()));
    }

    bool IsPurged() const;
    void Purge();
    void Join();
    void ResizeNotify(QSize size);
    void RepaintIfNeed(const QRect &rect);
    void OpenTextSeeker(View* = nullptr);
    void OpenQueryEditor(View* = nullptr);
    void OpenUrlEditor(View* = nullptr);
    void OpenCommand(View* = nullptr);

public slots:
    void OnReturned();
    void OnAborted();
    void ForeignCommandReceived();
    void SetString(QString str);
    void SetSuggest(QStringList list);
    void SuitableAction();
    void EditingFinished();
    void ReceiveCommand(QString cmd);

    void SelectNextSuggest();
    void SelectPrevSuggest();

private:
    void MakeOwnedWindow();
    void TakeWindowOwnerIfNeed();

    enum Mode {
        Command,
        Query,
        UrlEdit,
        Search,
    } m_Mode;

    void InitializeDisplay(Mode mode);
    void ReadForeignCommand(QLocalSocket *socket);

    void DispatchAction(const QString &action);

    TreeBank *m_TreeBank;

    static QLocalServer *m_LocalServer;
    LineEdit *m_LineEdit;
    QString m_LineString;
    QStringList m_SuggestStrings;
    int m_CurrentSuggestIndex;

protected:
    void focusInEvent(QFocusEvent *ev) Q_DECL_OVERRIDE;
    void focusOutEvent(QFocusEvent *ev) Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent *ev) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    void hideEvent(QHideEvent *ev) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *ev) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;

signals:

    void TriggerElementAction(Page::CustomAction);

    void OpenNodeWithCommand(QString name);

    void OpenUrl(QUrl);
    void OpenUrl(QList<QUrl>);
    void OpenQueryUrl(QString);
    void OpenBookmarklet(const QString&);
    void SearchWith(QString, QString);

    void Download(QString, QString);
    void SeekText(const QString&, View::FindFlags);
    void KeyEvent(QString);

public slots:
    void DisplaySuggest(const QByteArray&);
signals:
    void SuggestRequest(const QUrl&);
};
#endif

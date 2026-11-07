#ifndef DIALOG_HPP
#define DIALOG_HPP

#include "switch.hpp"
#include "theme.hpp"
#include "devicescale.hpp"

#include <QWidget>
#include <QIcon>
#include <QPointer>
#include <QUrl>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QInputMethodEvent>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPainter>

#include <float.h>

#include "callback.hpp"
#include "editmenu.hpp"

class MainWindow;
class ModelessDialog;
class QAuthenticator;

class Dialog {
    Q_GADGET

public:
    enum Button {
        NoButton = 0,
        Ok       = 1 << 0,
        Allow    = 1 << 1,
        Block    = 1 << 2,
        Yes      = 1 << 3,
        No       = 1 << 4,
        Cancel   = 1 << 5,
        Open     = 1 << 6,
        Close    = 1 << 7,
        OpenTrash = 1 << 8,
        Restart   = 1 << 9,
        Never     = 1 << 10
    };
    Q_ENUM(Button);
    Q_DECLARE_FLAGS(Buttons, Button);

    struct Row {
        Button button;
        QString text;
        bool returns;
    };

    static QString Text(Button button);

    static QList<Row> Table();

    static bool Returns(Button button);

    static Button Default(Buttons buttons, Button named);

    static Button Refusal(Buttons buttons);
};
Q_DECLARE_OPERATORS_FOR_FLAGS(Dialog::Buttons);

class ModalDialog : public QWidget {
    Q_OBJECT

public:
    ModalDialog();
    ~ModalDialog() override;

    template <class T> T ScaleByDevice(T t) const {
        return DeviceScale::FromDpi(t, static_cast<int>(logicalDpiY()));
    }

    enum Type {
        None,
        Int,
        Double,
        Text,
        Item
    };

public slots:
    bool Execute(QWidget *focusWidget = nullptr);

public:
    static void AbortAll();

    static bool AnyRunning();
    static bool InFileDialog();

    static quint64 Generation();

    class Running {
    public:
        Running(ModalDialog *dialog);
        ~Running();

        Running(const Running &) = delete;
        Running &operator=(const Running &) = delete;
    private:
        ModalDialog *m_Dialog;
    };

    static QString GetItem  (QString title, QString caption, QStringList items, bool editable = true, bool *ok = nullptr);
    static QString GetText  (QString title, QString caption, QString text = QString(), bool *ok = nullptr, const QString &icon = QString());
    static QString GetPass  (QString title, QString caption, QString text = QString(), bool *ok = nullptr);
    static QString GetExistingDirectory(const QString &caption = QString(), const QString &dir = QString(), QFileDialog::Options options = QFileDialog::ShowDirsOnly);
    static QString GetSaveFileName_(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = nullptr, QFileDialog::Options options = QFileDialog::Options());
    static QString GetOpenFileName_(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = nullptr, QFileDialog::Options options = QFileDialog::Options());
    static QStringList GetOpenFileNames(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = nullptr, QFileDialog::Options options = QFileDialog::Options());
    static void Information(const QString &title, const QString &text, const QString &icon = QString());
    static bool Question(const QString &title, const QString &text, const QString &icon = QString());
    static Dialog::Button AllowBlockOrCancel(const QString &title, const QString &question, const QString &detail);

    static QUrl SiteOrigin(const QUrl &securityOrigin, const QUrl &pageUrl);
    static QString SiteTitle(const QUrl &origin);
    static QString SiteIcon();
    static void Authentication(QAuthenticator *authenticator, const QString &caption);
    static QString AuthenticationCaption(const QUrl &url, const QString &proxyHost = QString());
    static QRect Placement(const QRect &room, const QSize &size, int margin);
    static class DialogLabel *AuthenticationLabel(const QString &caption, QWidget *parent = nullptr);

    void ApplyTheme();

    Dialog::Button ClickedButton(){ return m_ClickedButton;}
    void SetTitle(QString title){ m_Title = title;}
    void SetIcon(const QString &icon){ m_Icon = icon;}
    void SetCaption(QString caption){ m_Caption = caption;}
    void SetInformativeText(QString information){ m_InformativeText = information;}
    void SetDetailedText(QString detail){ m_DetailedText = detail;}
    void SetButtons(Dialog::Buttons buttons){ m_Buttons = buttons;}

protected:
    void keyPressEvent(QKeyEvent *ev) override;
    void paintEvent(QPaintEvent *ev) override;
    void showEvent(QShowEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;

signals:
    void Returned();
    void Aborted();

private:
    static QList<ModalDialog*> m_Running;
    static QList<ModalDialog*> m_Unwinding;
    static quint64 m_Generation;

    void RefreshIcon();
    void Answer(Dialog::Button button);

    Type m_Type;
    QPoint m_HotSpot;
    Dialog::Buttons m_Buttons;
    Dialog::Button m_ClickedButton;
    Dialog::Button m_DefaultButton;
    QString m_Title;
    QString m_Icon;
    QPointer<QLabel> m_IconLabel;
    QString m_Caption;
    QString m_InformativeText;
    QString m_DetailedText;
    QWidget *m_InputWidget;
    QPointer<class QParallelAnimationGroup> m_Appear;
};

class ModelessDialogFrame : public QWidget {
    Q_OBJECT

public:
    ModelessDialogFrame();
    ~ModelessDialogFrame();

    void Adjust();
    static void RegisterDialog(ModelessDialog *dialog);
    static void DeregisterDialog(ModelessDialog *dialog);
    QList<ModelessDialog*> Dialogs() const { return m_Dialogs;}

private:
    QList<ModelessDialog*> m_Dialogs;
};

class ModelessDialog : public QWidget {
    Q_OBJECT

public:
    ModelessDialog();
    ~ModelessDialog() override;

    template <class T> T ScaleByDevice(T t) const {
        return DeviceScale::FromDpi(t, static_cast<int>(logicalDpiY()));
    }

    enum Type {
        None,
        Int,
        Double,
        Text,
        Item
    };

public slots:
    void Execute();

public:
    static void Information(const QString &title, const QString &text, QObject *caller = nullptr);
    static ModelessDialog *Question(const QString &title, const QString &text, BoolCallBack callBack, QObject *caller = nullptr);

    void Discard();

    void ApplyTheme();

    Dialog::Button ClickedButton(){ return m_ClickedButton;}
    void SetTitle(QString title){ m_Title = title;}
    void SetCaption(QString caption){ m_Caption = caption;}
    void SetButtons(Dialog::Buttons buttons){ m_Buttons = buttons;}
    void SetDefaultValue(bool value){ m_DefaultValue = value;}
    void SetAutoCancel(bool on){ m_AutoCancel = on;}
    void Renew();
    void SetCallBack(BoolCallBack callBack){ m_CallBack = callBack;}
    void SetContent(QWidget *content);

protected:
    bool eventFilter(QObject *watched, QEvent *ev) override;
    void enterEvent(QEnterEvent *ev) override;
    void leaveEvent(QEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void timerEvent(QTimerEvent *ev) override;
    void paintEvent(QPaintEvent *ev) override;
    void showEvent(QShowEvent *ev) override;

signals:
    void Returned();
    void Aborted();

private:
    void StartTimer();
    void StopTimer();

    void Finish(bool value);
    void TakeAway();

    void Answer(Dialog::Button button);

    Type m_Type;
    Dialog::Buttons m_Buttons;
    Dialog::Button m_ClickedButton;
    Dialog::Button m_DefaultButton;
    bool m_DefaultValue;
    QString m_Title;
    QString m_Caption;
    int m_TimerId;
    bool m_AutoCancel;
    QWidget *m_Content = nullptr;
    BoolCallBack m_CallBack;
    bool m_Finished;
    ModelessDialogFrame *m_Frame;
    friend class ModelessDialogFrame;
};

class DialogLabel : public QLabel {
    Q_OBJECT

public:
    DialogLabel(const QString &text, const QFont &font, QWidget *parent = nullptr)
        : QLabel(parent)
    {
        setTextFormat(Qt::PlainText);
        setText(text);
        setFont(font);
    }
    ~DialogLabel() override {}
};

class DialogLineEdit : public QLineEdit {
    Q_OBJECT

public:
    DialogLineEdit(QWidget *parent = nullptr)
        : QLineEdit(parent)
    {
        EditMenu::Install(this);
    }
    ~DialogLineEdit() override {}

protected:
    void focusInEvent(QFocusEvent *ev) override {
        QLineEdit::focusInEvent(ev);
        emit FocusIn();
        ev->setAccepted(true);
    }
    void focusOutEvent(QFocusEvent *ev) override {
        QLineEdit::focusOutEvent(ev);
        emit FocusOut();
        ev->setAccepted(true);
    }
    void keyPressEvent(QKeyEvent *ev) override {
        QLineEdit::keyPressEvent(ev);
        if(!ev->isAccepted()){
            if(ev->key() == Qt::Key_Return){
                emit Returned();
                ev->setAccepted(true);
                return;
            }
            if(ev->key() == Qt::Key_Escape){
                emit Aborted();
                ev->setAccepted(true);
                return;
            }
        }
    }
    void inputMethodEvent(QInputMethodEvent *ev) override {
        QLineEdit::inputMethodEvent(ev);
        emit textChanged(text() + ev->preeditString());
    }

signals:
    void Returned();
    void Aborted();
    void FocusIn();
    void FocusOut();
};

class DialogComboBox : public QComboBox {
    Q_OBJECT

public:
    DialogComboBox(QWidget *parent = nullptr)
        : QComboBox(parent)
    {
    }
    ~DialogComboBox() override {}

protected:
    void focusInEvent(QFocusEvent *ev) override {
        QComboBox::focusInEvent(ev);
        emit FocusIn();
        ev->setAccepted(true);
    }
    void focusOutEvent(QFocusEvent *ev) override {
        QComboBox::focusOutEvent(ev);
        emit FocusOut();
        ev->setAccepted(true);
    }
    void keyPressEvent(QKeyEvent *ev) override {
        QComboBox::keyPressEvent(ev);
        if(!ev->isAccepted()){
            if(ev->key() == Qt::Key_Return){
                emit Returned();
                ev->setAccepted(true);
                return;
            }
            if(ev->key() == Qt::Key_Escape){
                emit Aborted();
                ev->setAccepted(true);
                return;
            }
        }
    }
    void inputMethodEvent(QInputMethodEvent *ev) override {
        QComboBox::inputMethodEvent(ev);
        emit editTextChanged(currentText() + ev->preeditString());
    }

signals:
    void Returned();
    void Aborted();
    void FocusIn();
    void FocusOut();
};

class DialogSlider : public QSlider {
    Q_OBJECT

public:
    DialogSlider(QWidget *parent = nullptr)
        : QSlider(parent)
    {
    }
    ~DialogSlider() override {}

protected:
    void focusInEvent(QFocusEvent *ev) override {
        QSlider::focusInEvent(ev);
        emit FocusIn();
        ev->setAccepted(true);
    }
    void focusOutEvent(QFocusEvent *ev) override {
        QSlider::focusOutEvent(ev);
        emit FocusOut();
        ev->setAccepted(true);
    }
    void keyPressEvent(QKeyEvent *ev) override {
        QSlider::keyPressEvent(ev);
        if(!ev->isAccepted()){
            if(ev->key() == Qt::Key_Return){
                emit Returned();
                ev->setAccepted(true);
                return;
            }
            if(ev->key() == Qt::Key_Escape){
                emit Aborted();
                ev->setAccepted(true);
                return;
            }
        }
    }

signals:
    void Returned();
    void Aborted();
    void FocusIn();
    void FocusOut();
};

#endif

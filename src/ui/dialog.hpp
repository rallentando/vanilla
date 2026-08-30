#ifndef DIALOG_HPP
#define DIALOG_HPP

#include "switch.hpp"
#include "theme.hpp"
#include "devicescale.hpp"

#include <QWidget>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QInputMethodEvent>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPainter>

#include <float.h>

#include "callback.hpp"

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
        Close    = 1 << 7
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
    ~ModalDialog() Q_DECL_OVERRIDE;

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

    static int     GetInt   (QString title, QString caption, int    val = 0, int    min = INT_MIN, int    max = INT_MAX, int     step = 1, bool *ok = nullptr);
    static double  GetDouble(QString title, QString caption, double val = 0, double min = DBL_MIN, double max = DBL_MAX, int decimals = 1, bool *ok = nullptr);
    static QString GetItem  (QString title, QString caption, QStringList items, bool editable = true, bool *ok = nullptr);
    static QString GetText  (QString title, QString caption, QString text = QString(), bool *ok = nullptr);
    static QString GetPass  (QString title, QString caption, QString text = QString(), bool *ok = nullptr);
    static QString GetExistingDirectory(const QString &caption = QString(), const QString &dir = QString(), QFileDialog::Options options = QFileDialog::ShowDirsOnly);
    static QString GetSaveFileName_(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = nullptr, QFileDialog::Options options = QFileDialog::Options());
    static QString GetOpenFileName_(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = nullptr, QFileDialog::Options options = QFileDialog::Options());
    static QStringList GetOpenFileNames(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = nullptr, QFileDialog::Options options = QFileDialog::Options());
    static void Information(const QString &title, const QString &text);
    static bool Question(const QString &title, const QString &text);
    static void Authentication(QAuthenticator *authenticator);

    Dialog::Button ClickedButton(){ return m_ClickedButton;}
    void SetTitle(QString title){ m_Title = title;}
    void SetCaption(QString caption){ m_Caption = caption;}
    void SetInformativeText(QString information){ m_InformativeText = information;}
    void SetDetailedText(QString detail){ m_DetailedText = detail;}
    void SetButtons(Dialog::Buttons buttons){ m_Buttons = buttons;}
    void SetDefaultButton(Dialog::Button button){ m_DefaultButton = button;}

protected:
    void keyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;

signals:
    void Returned();
    void Aborted();

private:
    static QList<ModalDialog*> m_Running;
    static QList<ModalDialog*> m_Unwinding;
    static quint64 m_Generation;

    void Answer(Dialog::Button button);

    Type m_Type;
    QPoint m_HotSpot;
    Dialog::Buttons m_Buttons;
    Dialog::Button m_ClickedButton;
    Dialog::Button m_DefaultButton;
    QString m_Title;
    QString m_Caption;
    QString m_InformativeText;
    QString m_DetailedText;
    QWidget *m_InputWidget;
};

class ModelessDialogFrame : public QWidget {
    Q_OBJECT

public:
    ModelessDialogFrame();
    ~ModelessDialogFrame();

    void Adjust();
    static void RegisterDialog(ModelessDialog *dialog);
    static void DeregisterDialog(ModelessDialog *dialog);

private:
    QList<ModelessDialog*> m_Dialogs;
};

class ModelessDialog : public QWidget {
    Q_OBJECT

public:
    ModelessDialog();
    ~ModelessDialog() Q_DECL_OVERRIDE;

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
    static void Question(const QString &title, const QString &text, BoolCallBack callBack, QObject *caller = nullptr);

    Dialog::Button ClickedButton(){ return m_ClickedButton;}
    void SetTitle(QString title){ m_Title = title;}
    void SetCaption(QString caption){ m_Caption = caption;}
    void SetButtons(Dialog::Buttons buttons){ m_Buttons = buttons;}
    void SetDefaultButton(Dialog::Button button){ m_DefaultButton = button;}
    void SetDefaultValue(bool value){ m_DefaultValue = value;}
    void SetCallBack(BoolCallBack callBack){ m_CallBack = callBack;}

protected:
    void enterEvent(QEnterEvent *ev) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *ev) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent *ev) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;

signals:
    void Returned();
    void Aborted();

private:
    void StartTimer();
    void StopTimer();

    void Finish(bool value);
    void Discard();
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
    BoolCallBack m_CallBack;
    bool m_Finished;
};

class DialogLabel : public QLabel {
    Q_OBJECT

public:
    DialogLabel(const QString &text, const QFont &font, QWidget *parent = nullptr)
        : QLabel(text, parent)
    {
        setFont(font);
        ApplyTheme();
    }
    ~DialogLabel() Q_DECL_OVERRIDE {}

    void ApplyTheme(){
        setStyleSheet(QStringLiteral("QLabel{ color: %1;}")
                      .arg(Theme::StyleSheetColor(Theme::DialogText)));
    }
};

class DialogLineEdit : public QLineEdit {
    Q_OBJECT

public:
    DialogLineEdit(QWidget *parent = nullptr)
        : QLineEdit(parent)
    {
    }
    ~DialogLineEdit() Q_DECL_OVERRIDE {}

protected:
    void focusInEvent(QFocusEvent *ev) Q_DECL_OVERRIDE {
        QLineEdit::focusInEvent(ev);
        emit FocusIn();
        ev->setAccepted(true);
    }
    void focusOutEvent(QFocusEvent *ev) Q_DECL_OVERRIDE {
        QLineEdit::focusOutEvent(ev);
        emit FocusOut();
        ev->setAccepted(true);
    }
    void keyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE {
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
    void inputMethodEvent(QInputMethodEvent *ev) Q_DECL_OVERRIDE {
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
    ~DialogComboBox() Q_DECL_OVERRIDE {}

protected:
    void focusInEvent(QFocusEvent *ev) Q_DECL_OVERRIDE {
        QComboBox::focusInEvent(ev);
        emit FocusIn();
        ev->setAccepted(true);
    }
    void focusOutEvent(QFocusEvent *ev) Q_DECL_OVERRIDE {
        QComboBox::focusOutEvent(ev);
        emit FocusOut();
        ev->setAccepted(true);
    }
    void keyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE {
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
    void inputMethodEvent(QInputMethodEvent *ev) Q_DECL_OVERRIDE {
        QComboBox::inputMethodEvent(ev);
        emit editTextChanged(currentText() + ev->preeditString());
    }

signals:
    void Returned();
    void Aborted();
    void FocusIn();
    void FocusOut();
};

class DialogIntSpinBox : public QSpinBox {
    Q_OBJECT

public:
    DialogIntSpinBox(QWidget *parent = nullptr)
        : QSpinBox(parent)
    {
    }
    ~DialogIntSpinBox() Q_DECL_OVERRIDE {}

protected:
    void focusInEvent(QFocusEvent *ev) Q_DECL_OVERRIDE {
        QSpinBox::focusInEvent(ev);
        emit FocusIn();
        ev->setAccepted(true);
    }
    void focusOutEvent(QFocusEvent *ev) Q_DECL_OVERRIDE {
        QSpinBox::focusOutEvent(ev);
        emit FocusOut();
        ev->setAccepted(true);
    }
    void keyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE {
        QSpinBox::keyPressEvent(ev);
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

class DialogDoubleSpinBox : public QDoubleSpinBox {
    Q_OBJECT

public:
    DialogDoubleSpinBox(QWidget *parent = nullptr)
        : QDoubleSpinBox(parent)
    {
    }
    ~DialogDoubleSpinBox() Q_DECL_OVERRIDE{}

protected:
    void focusInEvent(QFocusEvent *ev) Q_DECL_OVERRIDE {
        QDoubleSpinBox::focusInEvent(ev);
        emit FocusIn();
        ev->setAccepted(true);
    }
    void focusOutEvent(QFocusEvent *ev) Q_DECL_OVERRIDE {
        QDoubleSpinBox::focusOutEvent(ev);
        emit FocusOut();
        ev->setAccepted(true);
    }
    void keyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE {
        QDoubleSpinBox::keyPressEvent(ev);
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

class DialogSlider : public QSlider {
    Q_OBJECT

public:
    DialogSlider(QWidget *parent = nullptr)
        : QSlider(parent)
    {
    }
    ~DialogSlider() Q_DECL_OVERRIDE {}

protected:
    void focusInEvent(QFocusEvent *ev) Q_DECL_OVERRIDE {
        QSlider::focusInEvent(ev);
        emit FocusIn();
        ev->setAccepted(true);
    }
    void focusOutEvent(QFocusEvent *ev) Q_DECL_OVERRIDE {
        QSlider::focusOutEvent(ev);
        emit FocusOut();
        ev->setAccepted(true);
    }
    void keyPressEvent(QKeyEvent *ev) Q_DECL_OVERRIDE {
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

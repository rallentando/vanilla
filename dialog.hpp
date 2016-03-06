#ifndef DIALOG_HPP
#define DIALOG_HPP

#include "switch.hpp"

#include <QWidget>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QInputMethodEvent>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>

#include "callback.hpp"

// ModalDialog
// ModelessDialog

// GetInt
// GetDouble
// GetText
// GetPass
// GetItem
// GetExistingDirectory
// GetSaveFilename
// GetOpenFilename
// GetOpenFilenames
// Information
// Question
// Authentication

// ModelessInformation
// ModelessQuestion

class MainWindow;
class ModelessDialog;
class QAuthenticator;

class ModalDialog : public QWidget{
    Q_OBJECT

public:
    ModalDialog();
    ~ModalDialog();

    enum Type {
        None,
        Int,
        Double,
        Text,
        Item
    };

public slots:
    bool Execute(QWidget *focusWidget = 0);

public:
    static int     GetInt   (QString title, QString caption, int    val = 0, int    min = INT_MIN, int    max = INT_MAX, int     step = 1, bool *ok = 0);
    static double  GetDouble(QString title, QString caption, double val = 0, double min = DBL_MIN, double max = DBL_MAX, int decimals = 1, bool *ok = 0);
    static QString GetItem  (QString title, QString caption, QStringList items, bool editable = true, bool *ok = 0);
    static QString GetText  (QString title, QString caption, QString text = QString(), bool *ok = 0);
    static QString GetPass  (QString title, QString caption, QString text = QString(), bool *ok = 0);
    static QString GetExistingDirectory(const QString &caption = QString(), const QString &dir = QString(), QFileDialog::Options options = QFileDialog::ShowDirsOnly);
    // for name duplication of windows API...
    static QString GetSaveFileName_(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = 0);
    static QString GetOpenFileName_(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = 0);
    static QStringList GetOpenFileNames(const QString &caption = QString(), const QString &dir = QString(), const QString &filter = QString(), QString *selectedFilter = 0, QFileDialog::Options options = 0);
    static void Information(const QString &title, const QString &text);
    static bool Question(const QString &title, const QString &text);
    static void Authentication(QAuthenticator *authenticator);

    QString ClickedButton(){ return m_ClickedButton;}
    void SetTitle(QString title){ m_Title = title;}
    void SetCaption(QString caption){ m_Caption = caption;}
    void SetInformativeText(QString information){ m_InformativeText = information;}
    void SetDetailedText(QString detail){ m_DetailedText = detail;}
    void SetButtons(QStringList buttons){ m_Buttons = buttons;}

protected:
    void keyPressEvent(QKeyEvent *ev) DECL_OVERRIDE;
    void paintEvent(QPaintEvent *ev) DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *ev) DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) DECL_OVERRIDE;

signals:
    void Returned();
    void Aborted();

private:
    Type m_Type;
    QPoint m_HotSpot;
    QStringList m_Buttons;
    QString m_ClickedButton;
    QString m_Title;
    QString m_Caption;
    QString m_InformativeText;
    QString m_DetailedText;
    QWidget *m_InputWidget;
};

class ModelessDialogFrame : public QWidget{
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

class ModelessDialog : public QWidget{
    Q_OBJECT

public:
    ModelessDialog();
    ~ModelessDialog();

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
    static void Information(const QString &title, const QString &text, QObject *caller = 0);
    static void Question(const QString &title, const QString &text, BoolCallBack callBack, QObject *caller = 0);

    void SetTitle(QString title){ m_Title = title;}
    void SetCaption(QString caption){ m_Caption = caption;}
    void SetButtons(QStringList buttons){ m_Buttons = buttons;}
    void SetDefaultValue(bool value){ m_DefaultValue = value;}
    void SetCallBack(BoolCallBack callBack){ m_CallBack = callBack;}

protected:
    void enterEvent(QEvent *ev) DECL_OVERRIDE;
    void leaveEvent(QEvent *ev) DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *ev) DECL_OVERRIDE;
    void timerEvent(QTimerEvent *ev) DECL_OVERRIDE;
    void paintEvent(QPaintEvent *ev) DECL_OVERRIDE;

signals:
    void Returned();
    void Aborted();

private:
    void StartTimer();
    void StopTimer();

    Type m_Type;
    QStringList m_Buttons;
    bool m_DefaultValue;
    QString m_Title;
    QString m_Caption;
    int m_TimerID;
    BoolCallBack m_CallBack;
};

class DialogLineEdit : public QLineEdit{
    Q_OBJECT

public:
    DialogLineEdit(QWidget *parent = 0)
        : QLineEdit(parent)
    {
    }
    ~DialogLineEdit(){}

protected:
    void focusInEvent(QFocusEvent *ev) DECL_OVERRIDE {
        QLineEdit::focusInEvent(ev);
        emit FocusIn();
        ev->setAccepted(true);
    }
    void focusOutEvent(QFocusEvent *ev) DECL_OVERRIDE {
        QLineEdit::focusOutEvent(ev);
        emit FocusOut();
        ev->setAccepted(true);
    }
    void keyPressEvent(QKeyEvent *ev) DECL_OVERRIDE {
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
    void inputMethodEvent(QInputMethodEvent *ev) DECL_OVERRIDE {
        QLineEdit::inputMethodEvent(ev);
        emit textChanged(text() + ev->preeditString());
    }

signals:
    void Returned();
    void Aborted();
    void FocusIn();
    void FocusOut();
};

class DialogComboBox : public QComboBox{
    Q_OBJECT

public:
    DialogComboBox(QWidget *parent = 0)
        : QComboBox(parent)
    {
    }
    ~DialogComboBox(){}

protected:
    void focusInEvent(QFocusEvent *ev) DECL_OVERRIDE {
        QComboBox::focusInEvent(ev);
        emit FocusIn();
        ev->setAccepted(true);
    }
    void focusOutEvent(QFocusEvent *ev) DECL_OVERRIDE {
        QComboBox::focusOutEvent(ev);
        emit FocusOut();
        ev->setAccepted(true);
    }
    void keyPressEvent(QKeyEvent *ev) DECL_OVERRIDE {
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
    void inputMethodEvent(QInputMethodEvent *ev) DECL_OVERRIDE {
        QComboBox::inputMethodEvent(ev);
        emit editTextChanged(currentText() + ev->preeditString());
    }

signals:
    void Returned();
    void Aborted();
    void FocusIn();
    void FocusOut();
};

class DialogIntSpinBox : public QSpinBox{
    Q_OBJECT

public:
    DialogIntSpinBox(QWidget *parent = 0)
        : QSpinBox(parent)
    {
    }
    ~DialogIntSpinBox(){}

protected:
    void focusInEvent(QFocusEvent *ev) DECL_OVERRIDE {
        QSpinBox::focusInEvent(ev);
        emit FocusIn();
        ev->setAccepted(true);
    }
    void focusOutEvent(QFocusEvent *ev) DECL_OVERRIDE {
        QSpinBox::focusOutEvent(ev);
        emit FocusOut();
        ev->setAccepted(true);
    }
    void keyPressEvent(QKeyEvent *ev) DECL_OVERRIDE {
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

class DialogDoubleSpinBox : public QDoubleSpinBox{
    Q_OBJECT

public:
    DialogDoubleSpinBox(QWidget *parent = 0)
        : QDoubleSpinBox(parent)
    {
    }
    ~DialogDoubleSpinBox(){}

protected:
    void focusInEvent(QFocusEvent *ev) DECL_OVERRIDE {
        QDoubleSpinBox::focusInEvent(ev);
        emit FocusIn();
        ev->setAccepted(true);
    }
    void focusOutEvent(QFocusEvent *ev) DECL_OVERRIDE {
        QDoubleSpinBox::focusOutEvent(ev);
        emit FocusOut();
        ev->setAccepted(true);
    }
    void keyPressEvent(QKeyEvent *ev) DECL_OVERRIDE {
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

#endif

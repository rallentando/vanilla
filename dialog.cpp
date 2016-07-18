#include "switch.hpp"
#include "const.hpp"

#include "dialog.hpp"

#include <QPainter>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QDesktopWidget>
#include <QAuthenticator>

#include "application.hpp"
#include "mainwindow.hpp"
#include "view.hpp"

ModalDialog::ModalDialog()
    : QWidget(0)
    , m_Type(None)
    , m_Buttons(QStringList())
    , m_ClickedButton(QString())
    , m_Title(QString())
    , m_Caption(QString())
    , m_InformativeText(QString())
    , m_DetailedText(QString())
    , m_InputWidget(0)
    , m_HotSpot(QPoint())
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
    setAttribute(Qt::WA_ShowModal);
    setAttribute(Qt::WA_TranslucentBackground);
}

ModalDialog::~ModalDialog(){
    hide();
}

bool ModalDialog::Execute(QWidget *focusWidget){
    bool result;
    QEventLoop loop;

    QVBoxLayout *vlayout = new QVBoxLayout();
    vlayout->setContentsMargins(10, 7, 10, 7);

    QLabel *titleLabel = new QLabel(m_Title, this);
    titleLabel->setStyleSheet(DIALOG_TITLE_STYLE_SHEET);
    titleLabel->setContentsMargins(0, 0, 0, 3);
    vlayout->addWidget(titleLabel);

    if(!m_Caption.isEmpty()){
        QLabel *captionLabel = new QLabel(m_Caption, this);
        captionLabel->setStyleSheet(DIALOG_TEXT_STYLE_SHEET);
        captionLabel->setContentsMargins(5, 0, 0, 0);
        vlayout->addWidget(captionLabel);
    }

    if(!m_InformativeText.isEmpty()){
        QLabel *informationLabel = new QLabel(m_InformativeText, this);
        informationLabel->setStyleSheet(DIALOG_TEXT_STYLE_SHEET);
        informationLabel->setContentsMargins(5, 0, 0, 0);
        vlayout->addWidget(informationLabel);
    }

    if(m_InputWidget){
        vlayout->addWidget(m_InputWidget);
    }

    QHBoxLayout *hlayout = new QHBoxLayout();
    vlayout->addLayout(hlayout);
    hlayout->setAlignment(Qt::AlignRight);

    if(m_Buttons.isEmpty())
        m_Buttons << tr("OK") << tr("Cancel");

    if(m_Buttons.contains(tr("OK"))){
        QPushButton *returnButton = new QPushButton(tr("OK"), this);
        connect(returnButton, &QPushButton::clicked, [this](){ m_ClickedButton = tr("OK"); });
        connect(returnButton, &QPushButton::clicked, this, &ModalDialog::Returned);
        hlayout->addWidget(returnButton);
        returnButton->setDefault(true);
    }

    if(m_Buttons.contains(tr("Allow"))){
        QPushButton *allowButton = new QPushButton(tr("Allow"), this);
        connect(allowButton, &QPushButton::clicked, [this](){ m_ClickedButton = tr("Allow"); });
        connect(allowButton, &QPushButton::clicked, this, &ModalDialog::Returned);
        hlayout->addWidget(allowButton);
    }

    if(m_Buttons.contains(tr("Block"))){
        QPushButton *blockButton = new QPushButton(tr("Block"), this);
        connect(blockButton, &QPushButton::clicked, [this](){ m_ClickedButton = tr("Block"); });
        connect(blockButton, &QPushButton::clicked, this, &ModalDialog::Returned);
        hlayout->addWidget(blockButton);
    }

    if(m_Buttons.contains(tr("Yes"))){
        QPushButton *yesButton = new QPushButton(tr("Yes"), this);
        connect(yesButton, &QPushButton::clicked, [this](){ m_ClickedButton = tr("Yes"); });
        connect(yesButton, &QPushButton::clicked, this, &ModalDialog::Returned);
        hlayout->addWidget(yesButton);
    }

    if(m_Buttons.contains(tr("No"))){
        QPushButton *noButton = new QPushButton(tr("No"), this);
        connect(noButton, &QPushButton::clicked, [this](){ m_ClickedButton = tr("No"); });
        connect(noButton, &QPushButton::clicked, this, &ModalDialog::Aborted);
        hlayout->addWidget(noButton);
    }

    QTextEdit *textEdit = 0;
    if(!m_DetailedText.isEmpty()){
        textEdit = new QTextEdit(this);
        textEdit->setPlainText(m_DetailedText);
        textEdit->hide();

        QPushButton *toggleDetail = new QPushButton(tr("Show Details"));
        connect(toggleDetail, &QPushButton::clicked, [textEdit](){
                if(!textEdit->isVisible()){
                    textEdit->show();
                }
            });
        hlayout->addWidget(toggleDetail);
    }

    if(m_Buttons.contains(tr("Cancel"))){
        QPushButton *cancelButton = new QPushButton(tr("Cancel"), this);
        connect(cancelButton, &QPushButton::clicked, [this](){ m_ClickedButton = tr("Cancel"); });
        connect(cancelButton, &QPushButton::clicked, this, &ModalDialog::Aborted);
        hlayout->addWidget(cancelButton);
    }

    if(textEdit){
        vlayout->addWidget(textEdit);
    }

    setLayout(vlayout);

    connect(this, &ModalDialog::Returned, [&](){ result = true;  loop.quit(); });
    connect(this, &ModalDialog::Aborted,  [&](){ result = false; loop.quit(); });

    show();

    QDesktopWidget desktop;
    QPoint p;
    QSize s;

    if(QWidget *w = Application::CurrentWidget()){
        p = w->mapToGlobal(w->pos());
        s = w->size();
    } else if(desktop.screenCount()){
        QRect rect = desktop.screenGeometry(0);
        p = rect.topLeft();
        s = rect.size();
    } else {
        return false;
    }

    int w = qMin(s.width(), qMax(width(), MINIMUL_DIALOG_WIDTH));
    int h = height();
    int x = p.x() + (s.width()  - w) / 2;
    int y = p.y() + (s.height() - h) / 2;

    bool contains = false;
    for(int i = 0; i < desktop.screenCount(); i++){
        if(desktop.screenGeometry(i).intersects(QRect(x, y, w, h))){
            contains = true;
            break;
        }
    }

    if(!contains){
        if(desktop.screenCount()){
            QRect rect = desktop.screenGeometry(0);
            p = rect.topLeft();
            s = rect.size();
            w = qMin(s.width(), qMax(width(), MINIMUL_DIALOG_WIDTH));
            h = height();
            x = p.x() + (s.width()  - w) / 2;
            y = p.y() + (s.height() - h) / 2;
        } else {
            return false;
        }
    }

    setGeometry(x, y, w, h);
    raise();
    if(focusWidget) focusWidget->setFocus();
    loop.exec();
    deleteLater();
    return result;
}

int ModalDialog::GetInt(QString title, QString caption, int val, int min, int max, int step, bool *ok){
    ModalDialog *dialog = new ModalDialog();
    DialogIntSpinBox *intSpinBox = new DialogIntSpinBox(dialog);
    intSpinBox->setValue(val);
    intSpinBox->setRange(min, max);
    intSpinBox->setSingleStep(step);
    dialog->m_InputWidget = intSpinBox;
    dialog->m_Title = title;
    dialog->m_Caption = caption;
    connect(intSpinBox, &DialogIntSpinBox::Returned, dialog, &ModalDialog::Returned);
    connect(intSpinBox, &DialogIntSpinBox::Aborted,  dialog, &ModalDialog::Aborted);
    bool result = dialog->Execute(intSpinBox);
    if(ok) *ok = result;
    if(result) return intSpinBox->value();
    return 0;
}

double ModalDialog::GetDouble(QString title, QString caption, double val, double min, double max, int decimals, bool *ok){
    ModalDialog *dialog = new ModalDialog();
    DialogDoubleSpinBox *doubleSpinBox = new DialogDoubleSpinBox(dialog);
    doubleSpinBox->setValue(val);
    doubleSpinBox->setRange(min, max);
    doubleSpinBox->setDecimals(decimals);
    dialog->m_InputWidget = doubleSpinBox;
    dialog->m_Title = title;
    dialog->m_Caption = caption;
    connect(doubleSpinBox, &DialogDoubleSpinBox::Returned, dialog, &ModalDialog::Returned);
    connect(doubleSpinBox, &DialogDoubleSpinBox::Aborted,  dialog, &ModalDialog::Aborted);
    bool result = dialog->Execute(doubleSpinBox);
    if(ok) *ok = result;
    if(result) return doubleSpinBox->value();
    return 0;
}

QString ModalDialog::GetItem(QString title, QString caption, QStringList items, bool editable, bool *ok){
    ModalDialog *dialog = new ModalDialog();
    DialogComboBox *comboBox = new DialogComboBox(dialog);
    comboBox->setEditable(editable);
    comboBox->addItems(items);
    dialog->m_InputWidget = comboBox;
    dialog->m_Title = title;
    dialog->m_Caption = caption;
    connect(comboBox, &DialogComboBox::Returned, dialog, &ModalDialog::Returned);
    connect(comboBox, &DialogComboBox::Aborted,  dialog, &ModalDialog::Aborted);
    bool result = dialog->Execute(comboBox);
    if(ok) *ok = result;
    if(result) return comboBox->currentText();
    return QString();
}

QString ModalDialog::GetText(QString title, QString caption, QString text, bool *ok){
    ModalDialog *dialog = new ModalDialog();
    DialogLineEdit *lineEdit = new DialogLineEdit(dialog);
    lineEdit->setText(text);
    lineEdit->selectAll();
    dialog->m_InputWidget = lineEdit;
    dialog->m_Title = title;
    dialog->m_Caption = caption;
    connect(lineEdit, &DialogLineEdit::Returned, dialog, &ModalDialog::Returned);
    connect(lineEdit, &DialogLineEdit::Aborted,  dialog, &ModalDialog::Aborted);
    bool result = dialog->Execute(lineEdit);
    if(ok) *ok = result;
    if(result) return lineEdit->text();
    return QString();
}

QString ModalDialog::GetPass(QString title, QString caption, QString text, bool *ok){
    ModalDialog *dialog = new ModalDialog();
    DialogLineEdit *lineEdit = new DialogLineEdit(dialog);
    lineEdit->setText(text);
    lineEdit->selectAll();
    lineEdit->setEchoMode(QLineEdit::Password);
    dialog->m_InputWidget = lineEdit;
    dialog->m_Title = title;
    dialog->m_Caption = caption;
    connect(lineEdit, &DialogLineEdit::Returned, dialog, &ModalDialog::Returned);
    connect(lineEdit, &DialogLineEdit::Aborted,  dialog, &ModalDialog::Aborted);
    bool result = dialog->Execute(lineEdit);
    if(ok) *ok = result;
    if(result) return lineEdit->text();
    return QString();
}

QString ModalDialog::GetExistingDirectory(const QString &caption, const QString &dir, QFileDialog::Options options){
    return QFileDialog::getExistingDirectory(Application::CurrentWidget(), caption, dir, options);
}

QString ModalDialog::GetSaveFileName_(const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options){
    return QFileDialog::getSaveFileName(Application::CurrentWidget(), caption, dir, filter, selectedFilter, options);
}

QString ModalDialog::GetOpenFileName_(const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options){
    return QFileDialog::getOpenFileName(Application::CurrentWidget(), caption, dir, filter, selectedFilter, options);
}

QStringList ModalDialog::GetOpenFileNames(const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options){
    return QFileDialog::getOpenFileNames(Application::CurrentWidget(), caption, dir, filter, selectedFilter, options);
}

void ModalDialog::Information(const QString &title, const QString &text){
    ModalDialog *dialog = new ModalDialog();
    dialog->m_Title = title;
    dialog->m_Caption = text;
    dialog->m_Buttons << tr("OK");
    dialog->Execute();
}

bool ModalDialog::Question(const QString &title, const QString &text){
    ModalDialog *dialog = new ModalDialog();
    dialog->m_Title = title;
    dialog->m_Caption = text;
    dialog->m_Buttons << tr("Yes") << tr("No");
    return dialog->Execute();
}

void ModalDialog::Authentication(QAuthenticator *authenticator){
    ModalDialog *dialog = new ModalDialog();
    QWidget *widget = new QWidget();
    QGridLayout *layout = new QGridLayout();
    layout->setContentsMargins(5, 3, 5, 3);
    QLabel *userNameLabel = new QLabel(tr("UserName:"), dialog);
    userNameLabel->setStyleSheet(DIALOG_TEXT_STYLE_SHEET);
    layout->addWidget(userNameLabel, 0, 0);
    QLabel *passwordLabel = new QLabel(tr("Password:"), dialog);
    passwordLabel->setStyleSheet(DIALOG_TEXT_STYLE_SHEET);
    layout->addWidget(passwordLabel, 1, 0);
    DialogLineEdit *userNameEdit = new DialogLineEdit(dialog);
    layout->addWidget(userNameEdit, 0, 1);
    DialogLineEdit *passwordEdit = new DialogLineEdit(dialog);
    passwordEdit->setEchoMode(QLineEdit::Password);
    layout->addWidget(passwordEdit, 1, 1);

    widget->setLayout(layout);
    dialog->m_InputWidget = widget;
    dialog->m_Title = tr("User authentication.");
    connect(userNameEdit, &DialogLineEdit::Returned, dialog, &ModalDialog::Returned);
    connect(userNameEdit, &DialogLineEdit::Aborted,  dialog, &ModalDialog::Aborted);
    connect(passwordEdit, &DialogLineEdit::Returned, dialog, &ModalDialog::Returned);
    connect(passwordEdit, &DialogLineEdit::Aborted,  dialog, &ModalDialog::Aborted);

    if(dialog->Execute(userNameEdit)){
        authenticator->setUser(userNameEdit->text());
        authenticator->setPassword(passwordEdit->text());
    }
}

void ModalDialog::keyPressEvent(QKeyEvent *ev){
    QWidget::keyPressEvent(ev);
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

void ModalDialog::paintEvent(QPaintEvent *ev){
    Q_UNUSED(ev);

    QColor black = QColor(0, 0, 0, 150);

    QPainter painter(this);

    {   // background.
        painter.setPen(Qt::NoPen);
        painter.setBrush(black);
        painter.drawRect(-1, -1, width()+1, height()+1);
    }
}

void ModalDialog::mousePressEvent(QMouseEvent *ev){
    if(ev->button() == Qt::LeftButton){
        m_HotSpot = ev->pos();
        ev->setAccepted(true);
    }
}

void ModalDialog::mouseMoveEvent(QMouseEvent *ev){
    if(ev->buttons() & Qt::LeftButton){
        setGeometry(QRect(mapToGlobal(ev->pos()) - m_HotSpot, size()));
        ev->setAccepted(true);
    }
}

void ModalDialog::mouseReleaseEvent(QMouseEvent *ev){
    m_HotSpot = QPoint();
    ev->setAccepted(true);
}

// when Execute returns, it can return value but cannot access member of 'this',
// because sometime 'this' is deleted yet. So accessor is not implemented.

ModelessDialogFrame::ModelessDialogFrame()
    : QWidget(0)
    , m_Dialogs(QList<ModelessDialog*>())
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
    setAttribute(Qt::WA_TranslucentBackground);
}

ModelessDialogFrame::~ModelessDialogFrame(){
}

void ModelessDialogFrame::Adjust(){
    int offset = 0;
    foreach(ModelessDialog *dialog, m_Dialogs){
        dialog->show();
        dialog->raise();
        dialog->setGeometry(0, offset, width(), dialog->height());
        offset += dialog->height();
    }
    resize(width(), offset);
}

void ModelessDialogFrame::RegisterDialog(ModelessDialog *dialog){
    ModelessDialogFrame *frame = 0;

    if(MainWindow *win = Application::GetCurrentWindow()){
        frame = win->DialogFrame();
    } else if(Application::GetTemporaryDialogFrame()){
        frame = Application::GetTemporaryDialogFrame();
    } else {
        frame = Application::MakeTemporaryDialogFrame();
    }

    dialog->setParent(frame);
    frame->m_Dialogs.append(dialog);
    frame->Adjust();
}

void ModelessDialogFrame::DeregisterDialog(ModelessDialog *dialog){
    ModelessDialogFrame *frame = static_cast<ModelessDialogFrame*>(dialog->parent());

    frame->m_Dialogs.removeOne(dialog);
    frame->Adjust();
}

ModelessDialog::ModelessDialog()
    : QWidget(0)
    , m_Type(None)
    , m_Buttons(QStringList())
    , m_DefaultValue(false)
    , m_Title(QString(tr("Cancel")))
    , m_Caption(QString())
    , m_TimerId(0)
    , m_CallBack([](bool){})
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
    setAttribute(Qt::WA_TranslucentBackground);
}

ModelessDialog::~ModelessDialog(){
    hide();
}

void ModelessDialog::Execute(){
    QHBoxLayout *hlayout1 = new QHBoxLayout();

    QVBoxLayout *vlayout = new QVBoxLayout();
    hlayout1->addLayout(vlayout);
    QHBoxLayout *hlayout2 = new QHBoxLayout();
    hlayout1->addLayout(hlayout2);
    hlayout2->setAlignment(Qt::AlignRight);

    QLabel *titleLabel = new QLabel(m_Title, this);
    titleLabel->setStyleSheet(DIALOG_TITLE_STYLE_SHEET);
    titleLabel->setContentsMargins(0, 0, 0, 3);
    vlayout->addWidget(titleLabel);

    if(!m_Caption.isEmpty()){
        QLabel *captionLabel = new QLabel(m_Caption, this);
        captionLabel->setStyleSheet(DIALOG_TEXT_STYLE_SHEET);
        captionLabel->setContentsMargins(5, 0, 0, 0);
        vlayout->addWidget(captionLabel);
    }

    if(m_Buttons.isEmpty())
        m_Buttons << tr("OK") << tr("Cancel");

    if(m_Buttons.contains(tr("OK"))){
        QPushButton *returnButton = new QPushButton(tr("OK"), this);
        connect(returnButton, &QPushButton::clicked, this, &ModelessDialog::Returned);
        hlayout2->addWidget(returnButton);
    }

    if(m_Buttons.contains(tr("Allow"))){
        QPushButton *allowButton = new QPushButton(tr("Allow"), this);
        connect(allowButton, &QPushButton::clicked, this, &ModelessDialog::Returned);
        hlayout2->addWidget(allowButton);
    }

    if(m_Buttons.contains(tr("Block"))){
        QPushButton *blockButton = new QPushButton(tr("Block"), this);
        connect(blockButton, &QPushButton::clicked, this, &ModelessDialog::Returned);
        hlayout2->addWidget(blockButton);
    }

    if(m_Buttons.contains(tr("Yes"))){
        QPushButton *yesButton = new QPushButton(tr("Yes"), this);
        connect(yesButton, &QPushButton::clicked, this, &ModelessDialog::Returned);
        hlayout2->addWidget(yesButton);
    }

    if(m_Buttons.contains(tr("No"))){
        QPushButton *noButton = new QPushButton(tr("No"), this);
        connect(noButton, &QPushButton::clicked, this, &ModelessDialog::Aborted);
        hlayout2->addWidget(noButton);
    }

    if(m_Buttons.contains(tr("Cancel"))){
        QPushButton *cancelButton = new QPushButton(tr("Cancel"), this);
        connect(cancelButton, &QPushButton::clicked, this, &ModelessDialog::Aborted);
        hlayout2->addWidget(cancelButton);
    }

    setLayout(hlayout1);

    ModelessDialogFrame::RegisterDialog(this);

    connect(this, &ModelessDialog::Returned, [this](){
            ModelessDialogFrame::DeregisterDialog(this);
            m_CallBack(true);
            disconnect();
            deleteLater();
        });

    connect(this, &ModelessDialog::Aborted, [this](){
            ModelessDialogFrame::DeregisterDialog(this);
            m_CallBack(false);
            disconnect();
            deleteLater();
        });

    StartTimer();
}

void ModelessDialog::Information(const QString &title, const QString &text, QObject *caller){
    ModelessDialog *dialog = new ModelessDialog();
    if(caller) connect(caller, &QObject::destroyed, dialog, &ModelessDialog::Aborted);
    dialog->m_Title = title;
    dialog->m_Caption = text;
    dialog->m_Buttons << tr("OK");
    QTimer::singleShot(0, dialog, &ModelessDialog::Execute);
}

void ModelessDialog::Question(const QString &title, const QString &text, BoolCallBack callBack, QObject *caller){
    ModelessDialog *dialog = new ModelessDialog();
    if(caller) connect(caller, &QObject::destroyed, dialog, &ModelessDialog::Aborted);
    dialog->m_Title = title;
    dialog->m_Caption = text;
    dialog->m_Buttons << tr("Yes") << tr("No");
    dialog->m_CallBack = callBack;
    QTimer::singleShot(0, dialog, &ModelessDialog::Execute);
}

void ModelessDialog::enterEvent(QEvent *ev){
    QWidget::enterEvent(ev);
    StopTimer();
}

void ModelessDialog::leaveEvent(QEvent *ev){
    QWidget::leaveEvent(ev);
    StartTimer();
}

void ModelessDialog::keyPressEvent(QKeyEvent *ev){
    QWidget::keyPressEvent(ev);
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

void ModelessDialog::timerEvent(QTimerEvent *ev){
    QWidget::timerEvent(ev);
    if(ev->timerId() == m_TimerId){
        if(m_DefaultValue)
            emit Returned();
        else
            emit Aborted();
    }
}

void ModelessDialog::paintEvent(QPaintEvent *ev){
    Q_UNUSED(ev);

    QColor black = QColor(0, 0, 0, 150);

    QPainter painter(this);

    {   // background.
        painter.setPen(Qt::NoPen);
        painter.setBrush(black);
        painter.drawRect(-1, -1, width()+1, height()+1);
    }
}

void ModelessDialog::StartTimer(){
    if(m_TimerId){
        killTimer(m_TimerId);
    }
    m_TimerId = startTimer(AUTOCANCEL_DISTANCE);
}

void ModelessDialog::StopTimer(){
    if(m_TimerId){
        killTimer(m_TimerId);
        m_TimerId = 0;
    }
}


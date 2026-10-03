#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"

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
#include <QAuthenticator>
#include <QScreen>
#include <QCoreApplication>

#include "application.hpp"
#include "mainwindow.hpp"
#include "view.hpp"

namespace {

struct ButtonSpec {
    Dialog::Button button;
    const char *text;
    bool returns;
};

const ButtonSpec ButtonSpecs[] = {
    { Dialog::Ok,     QT_TRANSLATE_NOOP("Dialog", "OK"),     true  },
    { Dialog::Allow,  QT_TRANSLATE_NOOP("Dialog", "Allow"),  true  },
    { Dialog::Block,  QT_TRANSLATE_NOOP("Dialog", "Block"),  true  },
    { Dialog::Yes,    QT_TRANSLATE_NOOP("Dialog", "Yes"),    true  },
    { Dialog::No,     QT_TRANSLATE_NOOP("Dialog", "No"),     false },
    { Dialog::Cancel, QT_TRANSLATE_NOOP("Dialog", "Cancel"), false },
    { Dialog::Open,   QT_TRANSLATE_NOOP("Dialog", "Open"),   true  },
    { Dialog::Close,  QT_TRANSLATE_NOOP("Dialog", "Close"),  false }
};

}

QString Dialog::Text(Button button){
    for(const ButtonSpec &spec : ButtonSpecs)
        if(spec.button == button)
            return QCoreApplication::translate("Dialog", spec.text);
    return QString();
}

bool Dialog::Returns(Button button){
    for(const ButtonSpec &spec : ButtonSpecs)
        if(spec.button == button)
            return spec.returns;
    return false;
}

Dialog::Button Dialog::Default(Buttons buttons, Button named){
    if(named != NoButton && buttons.testFlag(named)) return named;
    if(buttons.testFlag(Ok)) return Ok;
    return NoButton;
}

Dialog::Button Dialog::Refusal(Buttons buttons){
    const Button order[] = { Cancel, Close, No };
    for(const Button button : order)
        if(buttons.testFlag(button)) return button;
    return NoButton;
}

QList<Dialog::Row> Dialog::Table(){
    QList<Row> rows;
    for(const ButtonSpec &spec : ButtonSpecs)
        rows << Row{spec.button,
                    QCoreApplication::translate("Dialog", spec.text),
                    spec.returns};
    return rows;
}

QList<ModalDialog*> ModalDialog::m_Running = QList<ModalDialog*>();
QList<ModalDialog*> ModalDialog::m_Unwinding = QList<ModalDialog*>();
quint64 ModalDialog::m_Generation = 0;

void ModalDialog::Answer(Dialog::Button button){
    m_ClickedButton = button;
    if(Dialog::Returns(button)) emit Returned();
    else                        emit Aborted();
}

void ModalDialog::AbortAll(){
    foreach(ModalDialog *dialog, QList<ModalDialog*>(m_Running))
        emit dialog->Aborted();
}

bool ModalDialog::AnyRunning(){
    return !m_Running.isEmpty() || !m_Unwinding.isEmpty();
}

quint64 ModalDialog::Generation(){
    return m_Generation;
}

ModalDialog::Running::Running(ModalDialog *dialog)
    : m_Dialog(dialog)
{
    m_Running.append(m_Dialog);
    m_Generation++;
}

ModalDialog::Running::~Running(){
    m_Running.removeOne(m_Dialog);
    m_Generation++;
    m_Unwinding.append(m_Dialog);
}

ModalDialog::ModalDialog()
    : QWidget(nullptr)
    , m_Type(None)
    , m_HotSpot(QPoint())
    , m_Buttons(Dialog::NoButton)
    , m_ClickedButton(Dialog::NoButton)
    , m_DefaultButton(Dialog::NoButton)
    , m_Title(QString())
    , m_Caption(QString())
    , m_InformativeText(QString())
    , m_DetailedText(QString())
    , m_InputWidget(nullptr)
{
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_ShowModal);
    setAttribute(Qt::WA_TranslucentBackground);
}

ModalDialog::~ModalDialog(){
    bool listed = m_Running.removeOne(this);
    if(m_Unwinding.removeOne(this)) listed = true;
    if(listed) m_Generation++;
    hide();
}

bool ModalDialog::Execute(QWidget *focusWidget){
    bool result = false;
    QEventLoop loop;

    QVBoxLayout *vlayout = new QVBoxLayout();
    vlayout->setContentsMargins(ScaleByDevice(10), ScaleByDevice(7),
                                ScaleByDevice(10), ScaleByDevice(7));

    DialogLabel *titleLabel = new DialogLabel(m_Title, DialogTitleFont(), this);
    titleLabel->setContentsMargins(0, 0, 0, ScaleByDevice(3));
    vlayout->addWidget(titleLabel);

    if(!m_Caption.isEmpty()){
        DialogLabel *captionLabel = new DialogLabel(m_Caption, DialogTextFont(), this);
        captionLabel->setContentsMargins(ScaleByDevice(5), 0, 0, 0);
        vlayout->addWidget(captionLabel);
    }

    if(!m_InformativeText.isEmpty()){
        DialogLabel *informationLabel = new DialogLabel(m_InformativeText, DialogTextFont(), this);
        informationLabel->setContentsMargins(ScaleByDevice(5), 0, 0, 0);
        vlayout->addWidget(informationLabel);
    }

    if(m_InputWidget){
        vlayout->addWidget(m_InputWidget);
    }

    QHBoxLayout *hlayout = new QHBoxLayout();
    vlayout->addLayout(hlayout);
    hlayout->setAlignment(Qt::AlignRight);

    if(m_Buttons == Dialog::NoButton)
        m_Buttons = Dialog::Ok | Dialog::Cancel;

    m_DefaultButton = Dialog::Default(m_Buttons, m_DefaultButton);

    QTextEdit *textEdit = nullptr;
    QPushButton *toggleDetail = nullptr;
    if(!m_DetailedText.isEmpty()){
        textEdit = new QTextEdit(this);
        textEdit->setPlainText(m_DetailedText);
        textEdit->hide();

        toggleDetail = new QPushButton(tr("Show Details"));
        connect(toggleDetail, &QPushButton::clicked, [textEdit](){
            if(!textEdit->isVisible()){
                textEdit->show();
            }
        });
    }

    for(const ButtonSpec &spec : ButtonSpecs){
        if(toggleDetail && spec.button == Dialog::Cancel &&
           m_Buttons.testFlag(Dialog::Cancel)){
            hlayout->addWidget(toggleDetail);
            toggleDetail = nullptr;
        }
        if(!m_Buttons.testFlag(spec.button)) continue;

        QPushButton *button = new QPushButton(Dialog::Text(spec.button), this);
        const Dialog::Button which = spec.button;
        connect(button, &QPushButton::clicked, this, [this, which](){ Answer(which);});
        hlayout->addWidget(button);

        if(spec.button == m_DefaultButton) button->setDefault(true);
    }

    if(toggleDetail) hlayout->addWidget(toggleDetail);

    if(textEdit){
        vlayout->addWidget(textEdit);
    }

    setLayout(vlayout);

    connect(this, &ModalDialog::Returned, [&](){ result = true;  loop.quit(); });
    connect(this, &ModalDialog::Aborted,  [&](){ result = false; loop.quit(); });

    show();

    Application *instance = Application::GetInstance();
    QPoint p;
    QSize s;

    if(QWidget *w = Application::CurrentWidget()){
        p = w->mapToGlobal(QPoint(0, 0));
        s = w->size();
    } else if(instance->screens().length()){
        QRect rect = instance->screens()[0]->geometry();
        p = rect.topLeft();
        s = rect.size();
    } else {
        return false;
    }

    int w = qMin(s.width(), qMax(width(), ScaleByDevice(MINIMUL_DIALOG_WIDTH)));
    int h = height();
    int x = p.x() + (s.width()  - w) / 2;
    int y = p.y() + (s.height() - h) / 2;

    bool contains = false;
    for(int i = 0; i < instance->screens().length(); i++){
        if(instance->screens()[i]->geometry().intersects(QRect(x, y, w, h))){
            contains = true;
            break;
        }
    }

    if(!contains){
        if(instance->screens().length()){
            QRect rect = instance->screens()[0]->geometry();
            p = rect.topLeft();
            s = rect.size();
            w = qMin(s.width(), qMax(width(), ScaleByDevice(MINIMUL_DIALOG_WIDTH)));
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

    {
        Running running(this);
        loop.exec();
    }

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

namespace {
    int fileDialogs = 0;
    struct FileDialogScope {
        FileDialogScope(){ fileDialogs++;}
        ~FileDialogScope(){ fileDialogs--;}
    };
}

bool ModalDialog::InFileDialog(){
    return fileDialogs > 0;
}

QString ModalDialog::GetExistingDirectory(const QString &caption, const QString &dir, QFileDialog::Options options){
    FileDialogScope shown;
    return QFileDialog::getExistingDirectory(Application::CurrentWidget(), caption, dir, options);
}

QString ModalDialog::GetSaveFileName_(const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options){
    FileDialogScope shown;
    return QFileDialog::getSaveFileName(Application::CurrentWidget(), caption, dir, filter, selectedFilter, options);
}

QString ModalDialog::GetOpenFileName_(const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options){
    FileDialogScope shown;
    return QFileDialog::getOpenFileName(Application::CurrentWidget(), caption, dir, filter, selectedFilter, options);
}

QStringList ModalDialog::GetOpenFileNames(const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options){
    FileDialogScope shown;
    return QFileDialog::getOpenFileNames(Application::CurrentWidget(), caption, dir, filter, selectedFilter, options);
}

void ModalDialog::Information(const QString &title, const QString &text){
    ModalDialog *dialog = new ModalDialog();
    dialog->m_Title = title;
    dialog->m_Caption = text;
    dialog->m_Buttons = Dialog::Ok;
    dialog->Execute();
}

bool ModalDialog::Question(const QString &title, const QString &text){
    ModalDialog *dialog = new ModalDialog();
    dialog->m_Title = title;
    dialog->m_Caption = text;
    dialog->m_Buttons = Dialog::Yes | Dialog::No;
    dialog->m_DefaultButton = Dialog::Yes;
    return dialog->Execute();
}

void ModalDialog::Authentication(QAuthenticator *authenticator){
    ModalDialog *dialog = new ModalDialog();
    QWidget *widget = new QWidget();
    QGridLayout *layout = new QGridLayout();
    layout->setContentsMargins(dialog->ScaleByDevice(5), dialog->ScaleByDevice(3),
                               dialog->ScaleByDevice(5), dialog->ScaleByDevice(3));
    DialogLabel *userNameLabel = new DialogLabel(tr("UserName:"), DialogTextFont(), dialog);
    layout->addWidget(userNameLabel, 0, 0);
    DialogLabel *passwordLabel = new DialogLabel(tr("Password:"), DialogTextFont(), dialog);
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
    } else {
        *authenticator = QAuthenticator();
    }
}

void ModalDialog::keyPressEvent(QKeyEvent *ev){
    QWidget::keyPressEvent(ev);
    if(ev->isAccepted()) return;

    if(ev->key() == Qt::Key_Return){
        if(m_DefaultButton == Dialog::NoButton) return;
        Answer(m_DefaultButton);
        ev->setAccepted(true);
        return;
    }
    if(ev->key() == Qt::Key_Escape){
        const Dialog::Button refusal = Dialog::Refusal(m_Buttons);
        if(refusal != Dialog::NoButton) Answer(refusal);
        else emit Aborted();
        ev->setAccepted(true);
        return;
    }
}

void ModalDialog::paintEvent(QPaintEvent *ev){
    Q_UNUSED(ev)

    QPainter painter(this);

    {
        const QRectF card = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Theme::Pen(Theme::DialogBorder));
        painter.setBrush(Theme::Brush(Theme::DialogShadow));
        painter.drawRoundedRect(card, ScaleByDevice(DIALOG_CORNER_RADIUS),
                                ScaleByDevice(DIALOG_CORNER_RADIUS));
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

ModelessDialogFrame::ModelessDialogFrame()
    : QWidget(nullptr)
    , m_Dialogs(QList<ModelessDialog*>())
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
    setAttribute(Qt::WA_TranslucentBackground);
}

ModelessDialogFrame::~ModelessDialogFrame(){
    foreach(ModelessDialog *dialog, m_Dialogs) dialog->m_Frame = nullptr;
    m_Dialogs.clear();
}

void ModelessDialogFrame::Adjust(){
    int offset = 0;
    foreach(ModelessDialog *dialog, m_Dialogs){
        dialog->show();
        dialog->raise();
        const int height = dialog->hasHeightForWidth()
            ? qMax(dialog->heightForWidth(width()), dialog->minimumSizeHint().height())
            : dialog->height();
        dialog->setGeometry(0, offset, width(), height);
        offset += height;
    }
    resize(width(), offset);
}

void ModelessDialogFrame::RegisterDialog(ModelessDialog *dialog){
    ModelessDialogFrame *frame = nullptr;

    if(MainWindow *win = Application::GetCurrentWindow()){
        frame = win->DialogFrame();
    } else if(Application::GetTemporaryDialogFrame()){
        frame = Application::GetTemporaryDialogFrame();
    } else {
        frame = Application::MakeTemporaryDialogFrame();
    }

    dialog->setParent(frame);
    dialog->m_Frame = frame;
    frame->m_Dialogs.append(dialog);
    frame->Adjust();
}

void ModelessDialogFrame::DeregisterDialog(ModelessDialog *dialog){
    ModelessDialogFrame *frame = dialog->m_Frame;
    if(!frame) return;
    dialog->m_Frame = nullptr;

    frame->m_Dialogs.removeOne(dialog);
    frame->Adjust();
}

ModelessDialog::ModelessDialog()
    : QWidget(nullptr)
    , m_Type(None)
    , m_Buttons(Dialog::NoButton)
    , m_ClickedButton(Dialog::NoButton)
    , m_DefaultButton(Dialog::NoButton)
    , m_DefaultValue(false)
    , m_Title(QString())
    , m_Caption(QString())
    , m_TimerId(0)
    , m_CallBack([](bool){})
    , m_Finished(false)
    , m_Frame(nullptr)
{
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    connect(this, &ModelessDialog::Returned, this, [this](){ Finish(true);});
    connect(this, &ModelessDialog::Aborted,  this, [this](){ Finish(false);});
}

void ModelessDialog::Answer(Dialog::Button button){
    m_ClickedButton = button;
    if(Dialog::Returns(button)) emit Returned();
    else                        emit Aborted();
}

void ModelessDialog::Finish(bool value){
    if(m_Finished) return;
    m_Finished = true;
    m_CallBack(value);
    TakeAway();
}

void ModelessDialog::Discard(){
    if(m_Finished) return;
    m_Finished = true;
    TakeAway();
}

void ModelessDialog::TakeAway(){
    ModelessDialogFrame::DeregisterDialog(this);

    disconnect();
    deleteLater();
}

ModelessDialog::~ModelessDialog(){
    ModelessDialogFrame::DeregisterDialog(this);
    hide();
}

void ModelessDialog::Execute(){
    if(m_Finished) return;

    QVBoxLayout *vlayout = new QVBoxLayout();
    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setAlignment(Qt::AlignRight);

    DialogLabel *titleLabel = new DialogLabel(m_Title, DialogTitleFont(), this);
    titleLabel->setContentsMargins(0, 0, 0, ScaleByDevice(3));
    vlayout->addWidget(titleLabel);

    if(!m_Caption.isEmpty()){
        DialogLabel *captionLabel = new DialogLabel(m_Caption, DialogTextFont(), this);
        captionLabel->setContentsMargins(ScaleByDevice(5), 0, 0, 0);
        vlayout->addWidget(captionLabel);
    }

    if(m_Buttons == Dialog::NoButton)
        m_Buttons = Dialog::Ok | Dialog::Cancel;

    m_DefaultButton = Dialog::Default(m_Buttons, m_DefaultButton);

    foreach(DialogLabel *label, findChildren<DialogLabel*>())
        label->setWordWrap(true);

    for(const ButtonSpec &spec : ButtonSpecs){
        if(!m_Buttons.testFlag(spec.button)) continue;

        QPushButton *button = new QPushButton(Dialog::Text(spec.button), this);
        const Dialog::Button which = spec.button;
        connect(button, &QPushButton::clicked, this, [this, which](){ Answer(which);});
        hlayout->addWidget(button);

        if(spec.button == m_DefaultButton) button->setDefault(true);
    }

    vlayout->addLayout(hlayout);
    setLayout(vlayout);

    ModelessDialogFrame::RegisterDialog(this);

    StartTimer();
}

void ModelessDialog::Information(const QString &title, const QString &text, QObject *caller){
    ModelessDialog *dialog = new ModelessDialog();
    if(caller) connect(caller, &QObject::destroyed, dialog, &ModelessDialog::Discard);
    dialog->m_Title = title;
    dialog->m_Caption = text;
    dialog->m_Buttons = Dialog::Ok;
    QTimer::singleShot(0, dialog, &ModelessDialog::Execute);
}

void ModelessDialog::Question(const QString &title, const QString &text, BoolCallBack callBack, QObject *caller){
    ModelessDialog *dialog = new ModelessDialog();
    if(caller) connect(caller, &QObject::destroyed, dialog, &ModelessDialog::Discard);
    dialog->m_Title = title;
    dialog->m_Caption = text;
    dialog->m_Buttons = Dialog::Yes | Dialog::No;
    dialog->m_DefaultButton = Dialog::Yes;
    dialog->m_CallBack = callBack;
    QTimer::singleShot(0, dialog, &ModelessDialog::Execute);
}

void ModelessDialog::enterEvent(QEnterEvent *ev)
{
    QWidget::enterEvent(ev);
    StopTimer();
}

void ModelessDialog::leaveEvent(QEvent *ev){
    QWidget::leaveEvent(ev);
    StartTimer();
}

void ModelessDialog::keyPressEvent(QKeyEvent *ev){
    QWidget::keyPressEvent(ev);
    if(ev->isAccepted()) return;

    if(ev->key() == Qt::Key_Return){
        if(m_DefaultButton == Dialog::NoButton) return;
        Answer(m_DefaultButton);
        ev->setAccepted(true);
        return;
    }
    if(ev->key() == Qt::Key_Escape){
        const Dialog::Button refusal = Dialog::Refusal(m_Buttons);
        if(refusal != Dialog::NoButton) Answer(refusal);
        else emit Aborted();
        ev->setAccepted(true);
        return;
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
    Q_UNUSED(ev)

    QPainter painter(this);

    {
        const QRectF card = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Theme::Pen(Theme::DialogBorder));
        painter.setBrush(Theme::Brush(Theme::DialogShadow));
        painter.drawRoundedRect(card, ScaleByDevice(DIALOG_CORNER_RADIUS),
                                ScaleByDevice(DIALOG_CORNER_RADIUS));
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

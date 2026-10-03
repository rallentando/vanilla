#ifndef EDGEHOSTWINDOW_HPP
#define EDGEHOSTWINDOW_HPP

#include <QWindow>
#include <QPointer>

class QWidget;

class EdgeHostWindow : public QWindow {
    Q_OBJECT

public:
    explicit EdgeHostWindow(QWindow *parent = nullptr);

    static bool AnswerMouseActivate(unsigned int message, qintptr *result);

    void GiveQtFocusOnPress(QWidget *widget);
    bool TakeQtFocus();

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    QPointer<QWidget> m_FocusOnPress;
};

#endif

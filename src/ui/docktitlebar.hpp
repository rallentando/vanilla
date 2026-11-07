#ifndef DOCKTITLEBAR_HPP
#define DOCKTITLEBAR_HPP

#include "switch.hpp"
#include "devicescale.hpp"
#include "theme.hpp"

#include <QWidget>
#include <QPixmap>

class QDockWidget;

class DockTitleBar : public QWidget {
    Q_OBJECT

public:
    explicit DockTitleBar(QDockWidget *dock);

    template <class T> T ScaleByDevice(T t) const {
        return DeviceScale::FromDpi(t, static_cast<int>(logicalDpiY()));
    }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    QRect CloseRect() const;
    QRect FloatRect() const;
    QRect TitleRect() const;

    bool OnTranslucentSurface() const;

protected:
    void paintEvent(QPaintEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void mouseDoubleClickEvent(QMouseEvent *ev) override;
    void leaveEvent(QEvent *ev) override;

private:
    enum Button { NoButton, FloatButton, CloseButton };

    Button ButtonAt(const QPoint &pos) const;
    QRect ButtonRect(int fromRight) const;
    void SetHovered(Button button);
    void LoadIcons();

    QDockWidget *m_Dock;
    QPixmap m_Float;
    QPixmap m_Close;
    Theme::Scheme m_IconScheme;
    Button m_Hovered;
    Button m_Pressed;
};

#endif

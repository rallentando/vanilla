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

    QSize sizeHint() const Q_DECL_OVERRIDE;
    QSize minimumSizeHint() const Q_DECL_OVERRIDE;

    QRect CloseRect() const;
    QRect FloatRect() const;
    QRect TitleRect() const;

    bool OnTranslucentSurface() const;

protected:
    void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *ev) Q_DECL_OVERRIDE;

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

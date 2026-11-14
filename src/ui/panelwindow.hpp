#ifndef PANELWINDOW_HPP
#define PANELWINDOW_HPP

#include "switch.hpp"
#include "const.hpp"
#include "theme.hpp"
#include "blurbehind.hpp"
#include "devicescale.hpp"

#include <QWidget>
#include <QLabel>
#include <QIcon>
#include <QPainter>
#include <QShowEvent>

class PanelWindow : public QWidget {
public:
    PanelWindow(QWidget *parent, Qt::WindowFlags flags, bool translucent = false)
        : QWidget(parent, flags)
        , m_Translucent(translucent)
    {
        setAttribute(Qt::WA_DeleteOnClose);
        if(m_Translucent) setAttribute(Qt::WA_TranslucentBackground);
        ApplyTheme();
    }
    ~PanelWindow() override {}

    void ApplyTheme(){
        setPalette(Theme::PanelPalette(palette()));
        update();
    }

protected:
    void paintEvent(QPaintEvent *ev) override {
        Q_UNUSED(ev)
        QPainter painter(this);
        painter.setPen(Theme::Pen(Theme::PreviewBorder));
        if(m_Translucent){
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setBrush(Theme::Brush(Theme::PreviewBackgroundTranslucent));
            const int radius = DeviceScale::FromDpi(DIALOG_CORNER_RADIUS, static_cast<int>(logicalDpiY()));
            const QRectF card = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
            painter.drawRoundedRect(card, radius, radius);
            return;
        }
        painter.setBrush(Theme::Brush(Theme::PreviewBackground));
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
    }
    void showEvent(QShowEvent *ev) override {
        QWidget::showEvent(ev);
        if(m_Translucent) BlurBehind::Apply(this);
    }

private:
    bool m_Translucent;
};

inline void MakePanelQuiet(QWidget *widget){
    QPalette palette = widget->palette();
    QColor quiet = Theme::Color(Theme::PreviewText);
    quiet.setAlphaF(DOWNLOAD_PANEL_QUIET_ALPHA);
    palette.setColor(QPalette::WindowText, quiet);
    widget->setPalette(palette);
}

inline void SpreadPanelAccent(QWidget *panel){
    const QPalette source = Theme::PanelPalette(panel->palette());
    for(QWidget *widget : panel->findChildren<QWidget*>()){
        QPalette palette = widget->palette();
        for(QPalette::ColorRole role : { QPalette::Accent, QPalette::Highlight, QPalette::HighlightedText })
            palette.setColor(role, source.color(role));
        widget->setPalette(palette);
    }
}

inline QIcon PanelCloseIcon(){
    return QIcon(Theme::Pixmap(QStringLiteral(":/resources/treebar/close.png"),
                               Theme::PreviewText));
}

class PanelRule : public QWidget {
public:
    explicit PanelRule(QWidget *parent) : QWidget(parent){
        setFixedHeight(1);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

protected:
    void paintEvent(QPaintEvent *ev) override {
        Q_UNUSED(ev)
        QPainter painter(this);
        painter.fillRect(rect(), Theme::Color(Theme::MenuSeparator));
    }
};

#endif

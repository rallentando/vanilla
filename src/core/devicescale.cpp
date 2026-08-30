#include "switch.hpp"

#include "devicescale.hpp"

#include <QGuiApplication>
#include <QScreen>

namespace DeviceScale {

int PrimaryDpi(){
    const QScreen *screen = QGuiApplication::primaryScreen();
    if(!screen) return BaseDpi;
    return static_cast<int>(screen->logicalDotsPerInchY());
}

qreal PrimaryFactor(){
    const QScreen *screen = QGuiApplication::primaryScreen();
    if(!screen) return 1.0;
    const qreal dpi = screen->logicalDotsPerInchY();
    if(dpi > BaseDpi) return dpi / BaseDpi;
    return 1.0;
}

QSize PrimarySize(const QSize &size){
    const int dpi = PrimaryDpi();
    return QSize(FromDpi(size.width(), dpi), FromDpi(size.height(), dpi));
}

}

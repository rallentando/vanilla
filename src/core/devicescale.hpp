#ifndef DEVICESCALE_HPP
#define DEVICESCALE_HPP

#include "switch.hpp"

#include <QSize>

namespace DeviceScale {

    const int BaseDpi = 96;

    template <class T> T FromDpi(T t, int dpi){
        if(dpi > BaseDpi) return t * dpi / BaseDpi;
        return t;
    }

    int PrimaryDpi();

    template <class T> T Primary(T t){ return FromDpi(t, PrimaryDpi());}

    QSize PrimarySize(const QSize &size);

    qreal PrimaryFactor();
}

#endif

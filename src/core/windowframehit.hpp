#ifndef WINDOWFRAMEHIT_HPP
#define WINDOWFRAMEHIT_HPP

#include <QList>
#include <QRect>

namespace WindowFrameHit {

enum Part { Client, Caption, MaximizeButton, Top, TopLeft, TopRight };

struct Layout {
    int width = 0;
    int border = 0;
    bool resizable = true;
    QRect bar;
    QRect maximize;
    QList<QRect> buttons;
};

inline Part At(const Layout &layout, const QPoint &pos){
    if(layout.resizable && pos.y() >= 0 && pos.y() < layout.border){
        if(pos.x() < layout.border*2) return TopLeft;
        if(pos.x() >= layout.width - layout.border*2) return TopRight;
        return Top;
    }
    if(!layout.bar.contains(pos)) return Client;
    if(layout.maximize.contains(pos)) return MaximizeButton;
    for(const QRect &button : layout.buttons)
        if(button.contains(pos)) return Client;
    return Caption;
}

inline long SystemAnswer(long code){
    switch(code){
    case 3: case 8: case 9: case 20: case 21:
        return 2;
    default:
        return code;
    }
}

}

namespace WindowFrameMargin {

struct State {
    bool qtMaximized = false;
    bool fullScreen = false;
    bool nativeKnown = false;
    bool nativeZoomed = false;
    bool nativeIconic = false;
    bool current = false;
};

inline bool Inner(const State &state){
    if(state.fullScreen) return false;
    if(state.nativeKnown){
        if(state.nativeIconic) return state.current;
        return !state.nativeZoomed;
    }
    return !state.qtMaximized;
}

}

#endif

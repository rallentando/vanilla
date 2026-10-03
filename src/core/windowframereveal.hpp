#ifndef WINDOWFRAMEREVEAL_HPP
#define WINDOWFRAMEREVEAL_HPP

#include <QRect>

class WindowFrameReveal {
public:
    bool Update(bool enabled, bool active, const QRect &strip,
                const QPoint &cursor, bool buttonsDown){
        if(!enabled || !active){
            Reset();
        } else if(!(m_Revealed && buttonsDown)){
            m_Revealed = strip.contains(cursor) &&
                         (m_Revealed || cursor.y() == strip.top());
        }
        return m_Revealed;
    }

    void Reset(){ m_Revealed = false; }

private:
    bool m_Revealed = false;
};

#endif

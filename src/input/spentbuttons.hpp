#ifndef SPENTBUTTONS_HPP
#define SPENTBUTTONS_HPP

#include <Qt>

class SpentButtons {
public:
    SpentButtons() : m_Spent(Qt::NoButton) {}

    void Press(Qt::MouseButton button, bool spent){
        if(spent) m_Spent |= button;
        else      m_Spent &= ~button;
    }

    bool Settle(Qt::MouseButton button){
        if(!(m_Spent & button)) return false;
        m_Spent &= ~button;
        return true;
    }

    Qt::MouseButtons Spent() const { return m_Spent;}

private:
    Qt::MouseButtons m_Spent;
};

#endif

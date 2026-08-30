#ifndef WINDOWLEDGER_HPP
#define WINDOWLEDGER_HPP

#include "switch.hpp"

#include <QMap>
#include <QList>

#include <functional>
#include <cstdlib>

template <class W>
class WindowLedger {

public:
    typedef QMap<int, W*> Map;

    WindowLedger() : m_Current(nullptr) {}

    std::function<void(W*)> Focus;
    std::function<bool()>   IsBusy;
    std::function<int()>    NewId;

    const Map &All() const { return m_Map;}
    QList<int> Ids() const { return m_Map.keys();}
    int Count() const { return static_cast<int>(m_Map.count());}
    bool IsEmpty() const { return m_Map.isEmpty();}

    W *At(int id) const { return m_Map.value(id, nullptr);}
    int IdOf(W *win) const { return win ? m_Map.key(win, 0) : 0;}

    W *Current() const { return m_Current;}
    int CurrentId() const { return IdOf(m_Current);}
    void SetCurrent(W *win){ m_Current = win;}
    void SetCurrent(int id){ m_Current = At(id);}

    int UnusedId() const {
        for(int i = 0; i < 1000; i++){
            const int id = NewId ? NewId() : (rand() + 1);
            if(id > 0 && !m_Map.contains(id)) return id;
        }
        return m_Map.isEmpty() ? 1 : qMax(1, m_Map.lastKey() + 1);
    }

    void Insert(int id, W *win){
        if(win) m_Map[id] = win;
    }

    void Remove(W *win){
        Remove(IdOf(win));
    }

    void Remove(int id){
        W *win = At(id);
        if(!win) return;

        m_Map.remove(id);

        if(m_Current == win) TakeNewCurrent();
    }

    W *Switch(bool next){
        if(m_Map.isEmpty()) return m_Current = nullptr;

        typename Map::const_iterator i = m_Map.constFind(CurrentId());

        if(i == m_Map.constEnd()){
            i = next ? m_Map.constBegin() : --m_Map.constEnd();

        } else if(next){
            ++i;
            if(i == m_Map.constEnd()) i = m_Map.constBegin();

        } else {
            if(i == m_Map.constBegin()) i = m_Map.constEnd();
            --i;
        }

        m_Current = i.value();
        if(m_Current && Focus) Focus(m_Current);
        return m_Current;
    }

private:
    void TakeNewCurrent(){
        m_Current = m_Map.isEmpty() ? nullptr : m_Map.first();
        if(m_Current && Focus && !(IsBusy && IsBusy())) Focus(m_Current);
    }

    Map m_Map;
    W *m_Current;
};

#endif

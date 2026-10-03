#ifndef TRANSLATIONORDER_HPP
#define TRANSLATIONORDER_HPP

#include "switch.hpp"

#include <QLocale>
#include <QStringList>

namespace TranslationOrder {

    inline QStringList Locales(const QStringList &uiLanguages){
        QStringList preferred;
        for(const QString &language : uiLanguages){
            const QString name = QLocale(language).name();
            if(!preferred.contains(name)) preferred << name;
        }
        QStringList order;
        for(auto it = preferred.crbegin(); it != preferred.crend(); ++it)
            order << *it;
        return order;
    }

}

#endif

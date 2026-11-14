#ifndef SITEDATALINK_HPP
#define SITEDATALINK_HPP

#include "switch.hpp"

#include <QJsonObject>
#include <QObject>
#include <QString>

#include <functional>

class SiteDataLink : public QObject {
public:
    using Answer = std::function<void(bool ok, const QJsonObject &result)>;
    explicit SiteDataLink(QObject *parent = nullptr) : QObject(parent){}
    static const int TIMEOUT_MSEC = 5000;
    virtual void Call(const QString &method, const QJsonObject &params, Answer done) = 0;
};

#endif

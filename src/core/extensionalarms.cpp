#include "switch.hpp"

#include "extensionalarms.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <cmath>

#include "extensionmainscripts.hpp"

namespace ExtensionAlarms {

namespace {

const double LEAST_STEP_MS = 1000;

QString BadArgument(int nth, const QString &what){
    return QStringLiteral("Invalid value for argument %1. %2").arg(nth).arg(what);
}

bool NameOf(const QJsonArray &args, QString *name, QString *error){
    const QJsonValue first = args.isEmpty() ? QJsonValue() : args.at(0);
    if(first.isUndefined() || first.isNull()){ name->clear(); return true; }
    if(!first.isString()){ *error = BadArgument(1, QStringLiteral("Expected a string.")); return false; }
    *name = first.toString();
    return true;
}

}

void Book::Create(const QString &id, const QJsonArray &args, double now, QString *error){
    QString name;
    QJsonValue info = args.isEmpty() ? QJsonValue() : args.at(0);
    int nth = 1;
    if(!info.isObject()){
        if(!NameOf(args, &name, error)) return;
        info = args.size() > 1 ? args.at(1) : QJsonValue();
        nth = 2;
    }
    if(!info.isObject()){ *error = BadArgument(nth, QStringLiteral("Expected an object.")); return; }
    const QJsonObject given = info.toObject();
    const QJsonValue when = given.value(QStringLiteral("when")), delay = given.value(QStringLiteral("delayInMinutes")),
        period = given.value(QStringLiteral("periodInMinutes"));
    const auto number = [](const QJsonValue &one){ return one.isUndefined() || one.isDouble(); };
    if(!number(when) || !number(delay) || !number(period)){
        *error = BadArgument(nth, QStringLiteral("Expected numbers."));
        return;
    }
    if(!delay.isUndefined() && !when.isUndefined()){
        *error = QStringLiteral("Cannot set both when and delayInMinutes.");
        return;
    }
    if(delay.isUndefined() && when.isUndefined() && period.isUndefined()){
        *error = QStringLiteral("Must set at least one of when, delayInMinutes, or periodInMinutes.");
        return;
    }
    Of &of = m_Of[id];
    if(of.alarms.size() >= MOST){
        *error = QStringLiteral("An extension cannot have more than %1 active alarms.").arg(MOST);
        return;
    }
    Alarm alarm;
    alarm.name = name;
    alarm.scheduledTime = !when.isUndefined() ? when.toDouble()
        : now + (delay.isUndefined() ? period : delay).toDouble() * 60000;
    if(!period.isUndefined()) alarm.periodInMinutes = std::max(0.0, period.toDouble());
    const auto named = [&](const Alarm &other){ return other.name == name; };
    of.alarms.removeIf(named);
    of.due.removeIf(named);
    of.alarms.append(alarm);
}

namespace {
QJsonObject Written(const QString &name, double scheduledTime, double periodInMinutes){
    QJsonObject out{ { QStringLiteral("name"), name }, { QStringLiteral("scheduledTime"), scheduledTime } };
    if(periodInMinutes >= 0) out.insert(QStringLiteral("periodInMinutes"), periodInMinutes);
    return out;
}
}

QJsonValue Book::Get(const QString &id, const QJsonArray &args, QString *error) const {
    QString name;
    if(!NameOf(args, &name, error)) return QJsonValue(QJsonValue::Undefined);
    const auto found = m_Of.constFind(id);
    if(found == m_Of.constEnd()) return QJsonValue(QJsonValue::Undefined);
    for(const Alarm &alarm : found->alarms)
        if(alarm.name == name) return Written(alarm.name, alarm.scheduledTime, alarm.periodInMinutes);
    return QJsonValue(QJsonValue::Undefined);
}

QJsonArray Book::GetAll(const QString &id) const {
    QJsonArray out;
    const auto found = m_Of.constFind(id);
    if(found == m_Of.constEnd()) return out;
    for(const Alarm &alarm : found->alarms) out.append(Written(alarm.name, alarm.scheduledTime, alarm.periodInMinutes));
    return out;
}

bool Book::Clear(const QString &id, const QJsonArray &args, QString *error){
    QString name;
    if(!NameOf(args, &name, error)) return false;
    const auto found = m_Of.find(id);
    if(found == m_Of.end()) return false;
    const auto named = [&](const Alarm &other){ return other.name == name; };
    const bool was = found->alarms.removeIf(named) > 0;
    found->due.removeIf(named);
    DropIfEmpty(id);
    return was;
}

bool Book::ClearAll(const QString &id){
    const auto found = m_Of.find(id);
    if(found == m_Of.end()) return true;
    found->alarms.clear();
    found->due.clear();
    DropIfEmpty(id);
    return true;
}

bool Book::Advance(double now, const std::function<bool(const QString &)> &mayRing){
    bool changed = false;
    for(auto it = m_Of.begin(); it != m_Of.end(); ++it){
        Of &of = it.value();
        changed = of.due.removeIf([&](const Alarm &one){ return now - one.since > DUE_KEPT_MS; }) > 0 || changed;
        if(!mayRing(it.key())) continue;
        for(int i = 0; i < of.alarms.size();){
            Alarm &alarm = of.alarms[i];
            if(alarm.scheduledTime > now){ i++; continue; }
            changed = true;
            Alarm rung = alarm;
            rung.since = now;
            of.due.removeIf([&](const Alarm &other){ return other.name == rung.name; });
            of.due.append(rung);
            if(alarm.periodInMinutes < 0){ of.alarms.removeAt(i); continue; }
            const double step = std::max(alarm.periodInMinutes * 60000, LEAST_STEP_MS);
            alarm.scheduledTime += step * (std::floor((now - alarm.scheduledTime) / step) + 1);
            i++;
        }
    }
    return changed;
}

QJsonArray Book::Take(const QString &id){
    QJsonArray out;
    const auto found = m_Of.find(id);
    if(found == m_Of.end()) return out;
    for(const Alarm &alarm : std::as_const(found->due)) out.append(Written(alarm.name, alarm.scheduledTime, alarm.periodInMinutes));
    found->due.clear();
    DropIfEmpty(id);
    return out;
}

bool Book::HasAlarms(const QString &id) const {
    const auto found = m_Of.constFind(id);
    return found != m_Of.constEnd() && (!found->alarms.isEmpty() || !found->due.isEmpty());
}

QStringList Book::WithDue() const {
    QStringList out;
    for(auto it = m_Of.constBegin(); it != m_Of.constEnd(); ++it) if(!it->due.isEmpty()) out << it.key();
    return out;
}

double Book::NextAt(const std::function<bool(const QString &)> &mayRing) const {
    double next = -1;
    for(auto it = m_Of.constBegin(); it != m_Of.constEnd(); ++it){
        if(!mayRing(it.key())) continue;
        for(const Alarm &alarm : it->alarms)
            if(next < 0 || alarm.scheduledTime < next) next = std::max(0.0, alarm.scheduledTime);
    }
    return next;
}

void Book::Drop(const QString &id){
    m_Of.remove(id);
}

void Book::DropIfEmpty(const QString &id){
    const auto found = m_Of.find(id);
    if(found != m_Of.end() && found->alarms.isEmpty() && found->due.isEmpty()) m_Of.erase(found);
}

namespace {
QJsonArray ListOf(const QList<QJsonObject> &list){
    QJsonArray out;
    for(const QJsonObject &one : list) out.append(one);
    return out;
}
}

QByteArray Book::ToJson() const {
    QJsonObject all;
    for(auto it = m_Of.constBegin(); it != m_Of.constEnd(); ++it){
        QList<QJsonObject> alarms, due;
        for(const Alarm &alarm : it->alarms) alarms << Written(alarm.name, alarm.scheduledTime, alarm.periodInMinutes);
        for(const Alarm &alarm : it->due){
            QJsonObject one = Written(alarm.name, alarm.scheduledTime, alarm.periodInMinutes);
            one.insert(QStringLiteral("since"), alarm.since);
            due << one;
        }
        all.insert(it.key(), QJsonObject{ { QStringLiteral("alarms"), ListOf(alarms) },
                                          { QStringLiteral("due"), ListOf(due) } });
    }
    return QJsonDocument(all).toJson(QJsonDocument::Compact);
}

Book Book::FromJson(const QByteArray &bytes){
    Book book;
    const QJsonObject all = QJsonDocument::fromJson(bytes).object();
    for(auto it = all.constBegin(); it != all.constEnd(); ++it){
        const QJsonObject kept = it.value().toObject();
        Of of;
        const auto read = [](const QJsonArray &list, QList<Alarm> *to){
            for(const QJsonValue &value : list){
                const QJsonObject one = value.toObject();
                const QJsonValue name = one.value(QStringLiteral("name")), at = one.value(QStringLiteral("scheduledTime"));
                if(!name.isString() || !at.isDouble() || to->size() >= MOST) continue;
                Alarm alarm;
                alarm.name = name.toString();
                alarm.scheduledTime = at.toDouble();
                alarm.periodInMinutes = one.value(QStringLiteral("periodInMinutes")).toDouble(-1);
                alarm.since = one.value(QStringLiteral("since")).toDouble();
                to->append(alarm);
            }
        };
        read(kept.value(QStringLiteral("alarms")).toArray(), &of.alarms);
        read(kept.value(QStringLiteral("due")).toArray(), &of.due);
        if(!of.alarms.isEmpty() || !of.due.isEmpty()) book.m_Of.insert(it.key(), of);
    }
    return book;
}

QString StoreFileName(const QString &profileKey){
    return ExtensionMainScripts::ProfileFileName(QStringLiteral("extension-alarms-"), profileKey);
}

}

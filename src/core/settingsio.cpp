#include "switch.hpp"
#include "const.hpp"

#include "settingsio.hpp"
#include "fileexchange.hpp"

#include <QFile>
#include <QDir>
#include <QVariant>
#include <QUrl>
#include <QSize>
#include <QPoint>
#include <QRect>
#include <QColor>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonParseError>

namespace SettingsIO {

namespace {

QJsonValue TaggedJson(QString tag, const QJsonValue &body){
    QJsonObject obj;
    obj[tag] = body;
    return QJsonValue(obj);
}

QByteArray ReadWholeFile(const QString &path, bool *ok){
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)){
        if(ok) *ok = false;
        return QByteArray();
    }
    QByteArray data = file.readAll();
    file.close();
    if(ok) *ok = true;
    return data;
}

}

QJsonValue VariantToJson(const QVariant &var){
    switch(var.typeId()){
    case QMetaType::QString:
        return QJsonValue(var.toString());
    case QMetaType::Bool:
        return QJsonValue(var.toBool());
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        return QJsonValue(var.toLongLong());
    case QMetaType::Double:
        return QJsonValue(var.toDouble());
    case QMetaType::QStringList:
        return QJsonValue(QJsonArray::fromStringList(var.toStringList()));
    case QMetaType::QUrl:
        return TaggedJson(QStringLiteral("@url"), QJsonValue(var.toUrl().toString()));
    case QMetaType::QSize:
        return TaggedJson(QStringLiteral("@size"),
                          QJsonArray() << var.value<QSize>().width()
                                       << var.value<QSize>().height());
    case QMetaType::QSizeF:
        return TaggedJson(QStringLiteral("@sizef"),
                          QJsonArray() << var.value<QSizeF>().width()
                                       << var.value<QSizeF>().height());
    case QMetaType::QPoint:
        return TaggedJson(QStringLiteral("@point"),
                          QJsonArray() << var.value<QPoint>().x()
                                       << var.value<QPoint>().y());
    case QMetaType::QPointF:
        return TaggedJson(QStringLiteral("@pointf"),
                          QJsonArray() << var.value<QPointF>().x()
                                       << var.value<QPointF>().y());
    case QMetaType::QRect:
        return TaggedJson(QStringLiteral("@rect"),
                          QJsonArray() << var.value<QRect>().x()
                                       << var.value<QRect>().y()
                                       << var.value<QRect>().width()
                                       << var.value<QRect>().height());
    case QMetaType::QRectF:
        return TaggedJson(QStringLiteral("@rectf"),
                          QJsonArray() << var.value<QRectF>().x()
                                       << var.value<QRectF>().y()
                                       << var.value<QRectF>().width()
                                       << var.value<QRectF>().height());
    case QMetaType::QColor:
        return TaggedJson(QStringLiteral("@color"),
                          QJsonArray() << var.value<QColor>().red()
                                       << var.value<QColor>().green()
                                       << var.value<QColor>().blue()
                                       << var.value<QColor>().alpha());
    default:{
        QByteArray ba;
        QDataStream stream(&ba, QIODevice::WriteOnly);
        stream << var;
        return TaggedJson(QStringLiteral("@variant"),
                          QJsonValue(QString::fromLatin1(ba.toBase64())));
    }
    }
}

QVariant JsonToVariant(const QJsonValue &val){
    if(val.isBool())   return QVariant(val.toBool());
    if(val.isDouble()) return val.toVariant();
    if(val.isString()) return QVariant(val.toString());

    if(val.isArray()){
        QStringList list;
        foreach(QJsonValue elem, val.toArray()) list << elem.toString();
        return QVariant(list);
    }
    if(val.isObject()){
        QJsonObject obj = val.toObject();
        if(obj.isEmpty()) return QVariant();
        QString tag = obj.keys().first();
        QJsonValue body = obj[tag];
        QJsonArray a = body.toArray();

        if(tag == QStringLiteral("@url"))
            return QVariant(QUrl(body.toString()));
        if(tag == QStringLiteral("@size"))
            return QVariant(QSize(a.at(0).toInt(), a.at(1).toInt()));
        if(tag == QStringLiteral("@sizef"))
            return QVariant(QSizeF(a.at(0).toDouble(), a.at(1).toDouble()));
        if(tag == QStringLiteral("@point"))
            return QVariant(QPoint(a.at(0).toInt(), a.at(1).toInt()));
        if(tag == QStringLiteral("@pointf"))
            return QVariant(QPointF(a.at(0).toDouble(), a.at(1).toDouble()));
        if(tag == QStringLiteral("@rect"))
            return QVariant(QRect(a.at(0).toInt(), a.at(1).toInt(),
                                  a.at(2).toInt(), a.at(3).toInt()));
        if(tag == QStringLiteral("@rectf"))
            return QVariant(QRectF(a.at(0).toDouble(), a.at(1).toDouble(),
                                   a.at(2).toDouble(), a.at(3).toDouble()));
        if(tag == QStringLiteral("@color"))
            return QVariant(QColor(a.at(0).toInt(), a.at(1).toInt(),
                                   a.at(2).toInt(), a.at(3).toInt()));
        if(tag == QStringLiteral("@variant")){
            QByteArray ba = QByteArray::fromBase64(body.toString().toLatin1());
            QDataStream stream(&ba, QIODevice::ReadOnly);
            QVariant var;
            stream >> var;
            return var;
        }
    }
    return QVariant();
}

bool ReadJson(const QByteArray &json, Map &map){
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json, &error);
    if(error.error != QJsonParseError::NoError || !doc.isObject()) return false;

    QJsonObject obj = doc.object();
    for(QJsonObject::const_iterator it = obj.constBegin(); it != obj.constEnd(); it++)
        map.insert(it.key(), JsonToVariant(it.value()));
    return true;
}

QByteArray WriteJson(const Map &map){
    QJsonObject obj;
    foreach(QString key, map.keys())
        obj[key] = VariantToJson(map.value(key));
    return QJsonDocument(obj).toJson(QJsonDocument::Indented);
}

bool ReadJsonFile(const QString &path, Map &map){
    bool ok = false;
    QByteArray data = ReadWholeFile(path, &ok);
    return ok && ReadJson(data, map);
}

bool WriteJsonFile(const QString &path, const Map &map){
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly)) return false;
    QByteArray data = WriteJson(map);
    bool written = file.write(data) == data.length();
    written = file.flush() && written && file.error() == QFileDevice::NoError;
    file.close();
    if(!written) file.remove();
    return written;
}

void Load(const QString &directory, const QString &filename, Map &map, const Hooks &hooks){
    if(ReadJsonFile(directory + filename, map)) return;

    const QString previous = directory + filename + QStringLiteral(".prev");
    if(QFile::exists(previous) && ReadJsonFile(previous, map)){
        hooks.ReportRestore(filename + QStringLiteral(".prev"));
        return;
    }

    QDir dir = QDir(directory);
    QStringList list =
        dir.entryList(hooks.BackUpFilters(), QDir::NoFilter, QDir::Name | QDir::Reversed);

    foreach(QString backup, list){

        if(!backup.endsWith(filename)) continue;
        if(!ReadJsonFile(directory + backup, map)) continue;

        hooks.ReportRestore(backup);
        break;
    }
}

bool Save(const QString &directory, const QString &filename, const Map &map, const Hooks &hooks){
    const QString target = directory + filename;
    const QString temporary = directory + hooks.BackUpPreposition() + filename;
    if(QFile::exists(temporary)) QFile::remove(temporary);

    if(!WriteJsonFile(temporary, map)) return false;
    return FileExchange::Replace(temporary, target);
}

}

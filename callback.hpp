#ifndef CALLBACK_HPP
#define CALLBACK_HPP

#include <QUrl>
#include <QString>
#include <QUrl>
#include <QString>
#include <QVariant>
#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QSizeF>
#include <QRect>
#include <QRectF>

#include <functional>

typedef std::function<void(bool)> BoolCallBack;
typedef std::function<void(const QUrl&)> UrlCallBack;
typedef std::function<void(const QString&)> StringCallBack;
typedef std::function<void(const QVariant&)> VariantCallBack;
typedef std::function<void(const QPoint&)> PointCallBack;
typedef std::function<void(const QPointF&)> PointFCallBack;
typedef std::function<void(const QSize&)> SizeCallBack;
typedef std::function<void(const QSizeF&)> SizeFCallBack;
typedef std::function<void(const QRect&)> RectCallBack;
typedef std::function<void(const QRectF&)> RectFCallBack;

#endif

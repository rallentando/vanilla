#ifndef WEBENGINEEXTENSIONS_HPP
#define WEBENGINEEXTENSIONS_HPP

#include "switch.hpp"

#ifdef WEBENGINEVIEW

#include "networkcontroller.hpp"

#include <QVariantMap>

#include <memory>

class QQuickWebEngineProfile;
class QWidget;
class View;
typedef std::weak_ptr<View> WeakView;

namespace WebEngineExtensions {

    void Install(QWebEngineProfile *profile);
    void Install(QQuickWebEngineProfile *profile);

    QQuickWebEngineProfile *CreateQuickProfile(QObject *owner, const QVariantMap &initial);

    QWidget *CreatePopup(const SharedProfile &profile, const QUrl &url, bool fits, const WeakView &opener, QWidget *parent,
                         const std::function<void()> &closed = std::function<void()>(), bool docked = false);
    QWidget *CreatePopup(QQuickWebEngineProfile *profile, const QUrl &url, bool fits, const WeakView &opener, QWidget *parent);
}

#endif

#endif

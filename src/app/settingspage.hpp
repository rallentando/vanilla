#ifndef SETTINGSPAGE_HPP
#define SETTINGSPAGE_HPP

#include "switch.hpp"

#include <QByteArray>
#include <QString>
#include <QUrl>

struct VanillaPageResponse {
    enum class UserInteraction {
        None,
        PickChromeExtensionDirectory
    };

    int m_Status;
    QByteArray m_ContentType;
    QByteArray m_Body;
    UserInteraction m_UserInteraction;

    VanillaPageResponse()
        : m_Status(404), m_ContentType(QByteArrayLiteral("text/plain"))
        , m_Body(QByteArray()), m_UserInteraction(UserInteraction::None) {}

    bool NeedsUserInteraction() const {
        return m_UserInteraction != UserInteraction::None;
    }
};

namespace VanillaPage {

    bool IsPageUrl(const QUrl &url);

    QByteArray WithInlinePalette(const QByteArray &html);

    VanillaPageResponse Answer(const QUrl &url, const QByteArray &method,
                               const QByteArray &body, const QUrl &initiator);

    bool ReadsBody(const QUrl &url, const QByteArray &method, const QUrl &initiator);
    constexpr qint64 BodyLimit = 1024 * 1024;

    VanillaPageResponse CompleteUserInteraction
        (const VanillaPageResponse &pending);

    QUrl InitiatorFromHeaders(const QString &origin, const QString &referer,
                              const QUrl &viewUrl, bool documentPost = false);

    QUrl SettingsUrl();
    bool IsSettingsUrl(const QUrl &url);
    bool SameDocument(const QUrl &a, const QUrl &b);
}

#ifdef WEBENGINEVIEW

#include <QWebEngineUrlSchemeHandler>

class QWebEngineProfile;
class QQuickWebEngineProfile;

class SettingsSchemeHandler : public QWebEngineUrlSchemeHandler {
    Q_OBJECT

public:
    SettingsSchemeHandler(QObject *parent = nullptr);
    ~SettingsSchemeHandler() Q_DECL_OVERRIDE;

    void requestStarted(QWebEngineUrlRequestJob *job) Q_DECL_OVERRIDE;

    static void RegisterScheme();
    static void Install(QWebEngineProfile *profile);
    static void Install(QQuickWebEngineProfile *profile);

    static QUrl SettingsUrl();
    static bool IsSettingsUrl(const QUrl &url);
};

#endif

#endif

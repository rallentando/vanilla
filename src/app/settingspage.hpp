#ifndef SETTINGSPAGE_HPP
#define SETTINGSPAGE_HPP

#include "switch.hpp"

#include <QByteArray>
#include <QString>
#include <QUrl>

#include <QList>
#include <functional>

class TreeBank;
class ViewNode;

struct VanillaPageResponse {
    enum class UserInteraction {
        None,
        PickChromeExtensionDirectory,
        Deferred
    };

    int m_Status;
    QByteArray m_ContentType;
    QByteArray m_Body;
    UserInteraction m_UserInteraction;
    std::function<QByteArray()> m_Deferred;

    VanillaPageResponse()
        : m_Status(404), m_ContentType(QByteArrayLiteral("text/plain"))
        , m_Body(QByteArray()), m_UserInteraction(UserInteraction::None) {}

    bool NeedsUserInteraction() const {
        return m_UserInteraction != UserInteraction::None;
    }
};

namespace VanillaPage {

    bool IsPageUrl(const QUrl &url);

    QString PermissionKindLabel(const QString &kind);
    QString PermissionKindScope(const QString &kind);

    QByteArray WithInlinePalette(const QByteArray &html);

    QByteArray WithEmbeddedSchema(const QByteArray &html, const QByteArray &schemaJson);

    QByteArray WithInlineResources(const QByteArray &html, const QByteArray &css,
                                   const QByteArray &js);

    VanillaPageResponse Answer(const QUrl &url, const QByteArray &method,
                               const QByteArray &body, const QUrl &initiator);

    bool ReadsBody(const QUrl &url, const QByteArray &method, const QUrl &initiator);
    constexpr qint64 BodyLimit = 1024 * 1024;

    VanillaPageResponse CompleteUserInteraction
        (const VanillaPageResponse &pending);

    QUrl InitiatorFromHeaders(const QString &origin, const QString &referer,
                              const QUrl &viewUrl, bool documentPost = false);

    QUrl StartUrl();
    bool IsStartUrl(const QUrl &url);

    QList<ViewNode*> FindTabs(const QString &query, TreeBank *asking, int limit);
    QList<ViewNode*> FindTrashedTabs(const QString &query, TreeBank *asking, int limit);
    ViewNode *TabOfSerial(quint64 serial);

    QUrl SettingsUrl();
    bool IsSettingsUrl(const QUrl &url);

    bool IsRecordingKey();

    bool IsTranslucentPage(const QUrl &url);
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
    ~SettingsSchemeHandler() override;

    void requestStarted(QWebEngineUrlRequestJob *job) override;

    static void RegisterScheme();
    static void Install(QWebEngineProfile *profile);
    static void Install(QQuickWebEngineProfile *profile);

    static QUrl SettingsUrl();
    static bool IsSettingsUrl(const QUrl &url);
};

#endif

#endif

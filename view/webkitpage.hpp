#ifndef WEBKITPAGE_HPP
#define WEBKITPAGE_HPP

#include "switch.hpp"

#ifdef WEBKITVIEW

#include <QWebPage>
#include <QWebSettings>

#include <QWebFrame>

#include <memory>

#include "page.hpp"
#include "application.hpp"
#include "dialog.hpp"

class QMenu;
class QObject;
class QAction;
class QMenu;
class View;
class TreeBank;
class MainWindow;

class ViewNode;
class HistNode;
class WebElement;

typedef std::shared_ptr<WebElement> SharedWebElement;
typedef QList<std::shared_ptr<WebElement>> SharedWebElementList;

class WebKitPage : public QWebPage{
    Q_OBJECT

public:
    WebKitPage(NetworkAccessManager *nam, QObject *parent = 0);
    ~WebKitPage();
    View* GetView();
    WebKitPage* createWindow(WebWindowType type) Q_DECL_OVERRIDE;
    void triggerAction(WebAction action, bool checked = false) Q_DECL_OVERRIDE;

    bool acceptNavigationRequest(QWebFrame *frame,
                                 const QNetworkRequest &request,
                                 NavigationType type) Q_DECL_OVERRIDE;

    QObject* createPlugin(const QString &id, const QUrl &url,
                          const QStringList &names,
                          const QStringList &values) Q_DECL_OVERRIDE;

    QString userAgentForUrl(const QUrl &url) const Q_DECL_OVERRIDE;
    QString chooseFile(QWebFrame *parentframe, const QString &suggested) Q_DECL_OVERRIDE;
    bool extension(Extension extension, const ExtensionOption *option = 0, ExtensionReturn *output = 0) Q_DECL_OVERRIDE;
    bool supportsExtension(Extension extension) const Q_DECL_OVERRIDE;

    void javaScriptAlert(QWebFrame *frame, const QString &msg) Q_DECL_OVERRIDE {
        //QWebPage::javaScriptAlert(frame, msg);
        Q_UNUSED(frame);
        ModalDialog::Information(QStringLiteral("Javascript Alert"), msg);
    }
    bool javaScriptConfirm(QWebFrame *frame, const QString & msg) Q_DECL_OVERRIDE {
        //return QWebPage::javaScriptConfirm(frame, msg);
        Q_UNUSED(frame);
        return ModalDialog::Question(QStringLiteral("Javascript Confirm"), msg);
    }
    bool javaScriptPrompt(QWebFrame *frame, const QString &msg, const QString &defaultValue, QString *result) Q_DECL_OVERRIDE {
        //return QWebPage::javaScriptPrompt(frame, msg, defaultValue, result);
        Q_UNUSED(frame);
        bool ok = true;
        if(result) *result = ModalDialog::GetText(QStringLiteral("Javascript Prompt"), msg, defaultValue, &ok);
        return ok;
    }
    void javaScriptConsoleMessage(const QString &msg, int lineNumber, const QString &sourceID) Q_DECL_OVERRIDE {
        QWebPage::javaScriptConsoleMessage(msg, lineNumber, sourceID);
    }

    void DisplayContextMenu(QWidget *parent, SharedWebElement elem,
                            QPoint localPos, QPoint globalPos,
                            Page::MediaType type = Page::MediaTypeNone);

    void TriggerAction(Page::CustomAction, QVariant = QVariant());
    QAction *Action(Page::CustomAction, QVariant = QVariant());

public slots:
    void Download(const QNetworkRequest &req,
                  const QString &file = QString()){
        m_Page->Download(req, file);
    }
    void Download(const QUrl &target,
                  const QUrl &referer,
                  const QString &file = QString()){
        m_Page->Download(target, referer, file);
    }
    void Download(const QString &url,
                  const QString &file = QString()){
        m_Page->Download(url, file);
    }

    void HandleUnsupportedContent(QNetworkReply *reply);

    void AddJsObject();

    View *OpenInNew(QUrl url){ return m_Page->OpenInNew(url);}
    View *OpenInNew(QList<QUrl> urls){ return m_Page->OpenInNew(urls);}
    View *OpenInNew(QString query){ return m_Page->OpenInNew(query);}
    View *OpenInNew(QString key, QString query){ return m_Page->OpenInNew(key, query);}
    View *OpenInNewBackground(QUrl url){ return m_Page->OpenInNewViewNodeBackground(url);}
    View *OpenInNewBackground(QList<QUrl> urls){ return m_Page->OpenInNewViewNodeBackground(urls);}
    View *OpenInNewBackground(QString query){ return m_Page->OpenInNewViewNodeBackground(query);}
    View *OpenInNewBackground(QString key, QString query){ return m_Page->OpenInNewViewNodeBackground(key, query);}

    void SetSource(const QUrl &url){ m_Page->SetSource(url);}
    void SetSource(const QByteArray &html){ m_Page->SetSource(html);}
    void SetSource(const QString &html){ m_Page->SetSource(html);}

    void Print();

    void InspectElement();
    void AddSearchEngine(QPoint);
    void AddBookmarklet(QPoint);

signals:
    void statusBarMessage2(const QString&, const QString&);
    void ViewChanged();
    void ScrollChanged(QPointF);
    void ButtonCleared();
    void RenderFinished();
    void urlChanged(const QUrl&);
    void titleChanged(const QString&);

public slots:
    void DownloadSuggest(const QUrl&);
signals:
    void SuggestResult(const QByteArray&);

private:
    View *m_View;
    Page *m_Page;
};

#endif //ifdef WEBKITVIEW
#endif //ifndef WEBKITPAGE_HPP

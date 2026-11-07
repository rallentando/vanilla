#ifndef WEBELEMENT_HPP
#define WEBELEMENT_HPP

#include "switch.hpp"

#include <QPixmap>

#include <memory>

class View;

class WebElement {
public:
    explicit WebElement(){
    }
    virtual ~WebElement(){
    }
    enum Action {
        Click,
        Focus,
        Hover,
        None,
    };
    virtual bool SetFocus(){ return false;}
    virtual bool ClickEvent(){ return false;}
    virtual QString TagName() const { return QString();}
    virtual QString InnerText() const { return QString();}
    virtual QUrl BaseUrl() const { return QUrl();}
    virtual QUrl LinkUrl() const { return QUrl();}
    virtual QUrl ImageUrl() const { return QUrl();}
    virtual QString LinkHtml() const { return QString();}
    virtual QString ImageHtml() const { return QString();}
    virtual QPoint Position() const { return QPoint();}
    virtual QRect Rectangle() const { return QRect();}
    virtual QRegion Region() const { return QRegion();}
    virtual void SetRectangle(QRect){}
    virtual void SetText(QString){}
    virtual QPixmap Pixmap(){ return QPixmap();}
    virtual bool IsNull() const { return true;}
    virtual bool IsEditableElement() const { return false;}
    virtual bool IsTextInputElement() const { return false;}
    virtual bool IsQueryInputElement() const { return false;}
    virtual bool IsFrameElement() const { return false;}
    virtual bool IsLooped() const { return false;}
    virtual bool IsPaused() const { return false;}
    virtual bool IsMuted() const { return false;}
    virtual Action GetAction() const { return None;}
};

typedef std::shared_ptr<WebElement> SharedWebElement;
typedef QList<std::shared_ptr<WebElement>> SharedWebElementList;
typedef std::function<void(SharedWebElement)> WebElementCallBack;
typedef std::function<void(SharedWebElementList)> WebElementListCallBack;

Q_DECLARE_METATYPE(SharedWebElement);

class JsWebElement : public WebElement {
public:
    explicit JsWebElement();
    explicit JsWebElement(View *provider, QVariant var);
    explicit JsWebElement(View *provider, const QPoint &pos, const QUrl &linkUrl, const QUrl &imageUrl, bool isEditable);
    virtual ~JsWebElement();

    virtual bool SetFocus() override;
    virtual bool ClickEvent() override;
    virtual QString TagName() const override;
    virtual QString InnerText() const override;
    virtual QUrl BaseUrl() const override;
    virtual QUrl LinkUrl() const override;
    virtual QUrl ImageUrl() const override;
    virtual QString LinkHtml() const override;
    virtual QString ImageHtml() const override;
    virtual QPoint Position() const override;
    virtual QRect Rectangle() const override;
    virtual QRegion Region() const override;
    virtual void SetRectangle(QRect) override;
    virtual void SetText(QString) override;
    virtual QPixmap Pixmap() override;
    virtual bool IsNull() const override;
    virtual bool IsEditableElement() const override;
    virtual bool IsTextInputElement() const override;
    virtual bool IsQueryInputElement() const override;
    virtual bool IsFrameElement() const override;
    virtual bool IsLooped() const override;
    virtual bool IsPaused() const override;
    virtual bool IsMuted() const override;
    virtual Action GetAction() const override;

protected:
    View *m_Provider;
    QString m_TagName;
    QString m_InnerText;
    QUrl m_BaseUrl;
    QUrl m_LinkUrl;
    QUrl m_ImageUrl;
    QString m_LinkHtml;
    QString m_ImageHtml;
    QRect m_Rectangle;
    QRegion m_Region;
    bool m_IsTextInput;
    bool m_IsQueryInput;
    bool m_IsEditable;
    bool m_IsFrame;
    bool m_IsLooped;
    bool m_IsPaused;
    bool m_IsMuted;
    QString m_XPath;
    QString m_Action;
};

#endif

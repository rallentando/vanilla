#include "switch.hpp"
#include "const.hpp"

#include "webelement.hpp"

#include <QPixmap>
#include <QPainter>

#include "view.hpp"

JsWebElement::JsWebElement()
    : WebElement()
{
    m_Provider     = 0;
    m_TagName      = QString();
    m_InnerText    = QString();
    m_BaseUrl      = QUrl();
    m_LinkUrl      = QUrl();
    m_ImageUrl     = QUrl();
    m_LinkHtml     = QString();
    m_ImageHtml    = QString();
    m_Rectangle    = QRect();
    m_IsEditable   = false;
    m_IsJsCommand  = false;
    m_IsTextInput  = false;
    m_IsQueryInput = false;
    m_IsFrame      = false;
    m_IsLooped     = false;
    m_IsPaused     = false;
    m_IsMuted      = false;
    m_XPath        = QString();
    m_Action       = QStringLiteral("None");
}

JsWebElement::JsWebElement(View *provider, QVariant var)
    : WebElement()
{
    QVariantMap map = var.toMap();
    m_Provider  = provider;
    m_TagName   = map[QStringLiteral("tagName")].toString();
    m_InnerText = map[QStringLiteral("innerText")].toString();
    m_BaseUrl   = Page::StringToUrl(map[QStringLiteral("baseUrl")].toString());
    m_LinkUrl   = Page::StringToUrl(map[QStringLiteral("linkUrl")].toString(), m_BaseUrl);
    m_ImageUrl  = Page::StringToUrl(map[QStringLiteral("imageUrl")].toString(), m_BaseUrl);
    m_LinkHtml  = map[QStringLiteral("linkHtml")].toString();
    m_ImageHtml = map[QStringLiteral("imageHtml")].toString();
    m_Rectangle = QRect(map[QStringLiteral("x")].toInt(),     map[QStringLiteral("y")].toInt(),
                        map[QStringLiteral("width")].toInt(), map[QStringLiteral("height")].toInt());
    m_IsEditable   = map[QStringLiteral("isEditable")].toBool();
    m_IsJsCommand  = map[QStringLiteral("isJsCommand")].toBool();
    m_IsTextInput  = map[QStringLiteral("isTextInput")].toBool();
    m_IsQueryInput = map[QStringLiteral("isQueryInput")].toBool();
    m_IsFrame      = map[QStringLiteral("isFrame")].toBool();
    m_IsLooped     = map[QStringLiteral("isLooped")].toBool();
    m_IsPaused     = map[QStringLiteral("isPaused")].toBool();
    m_IsMuted      = map[QStringLiteral("isMuted")].toBool();
    m_XPath        = map[QStringLiteral("xPath")].toString();
    m_Action       = map[QStringLiteral("action")].toString();

    m_Region = QRegion();
    QVariantMap regionMap = map[QStringLiteral("region")].toMap();
    QRect viewport = QRect(QPoint(), m_Provider->size());
    foreach(QString key, regionMap.keys()){
        QVariantMap m = regionMap[key].toMap();
        m_Region |= QRect(m["x"].toInt(),
                          m["y"].toInt(),
                          m["width"].toInt(),
                          m["height"].toInt()).intersected(viewport);
    }
}

JsWebElement::JsWebElement(View *provider, const QPoint &pos, const QUrl &linkUrl, const QUrl &imageUrl, bool isEditable)
    : JsWebElement()
{
    m_Provider = provider;
    m_LinkUrl = linkUrl;
    m_ImageUrl = imageUrl;
    m_IsEditable = isEditable;
    m_Rectangle = QRect(pos, QSize(1, 1));
}

JsWebElement::~JsWebElement(){
}

bool JsWebElement::SetFocus(){
    if(m_Provider){
        QMetaObject::invokeMethod(m_Provider->base(), "SetFocusToElement",
                                  Q_ARG(QString, m_XPath));
        return true;
    }
    return false;
}

bool JsWebElement::ClickEvent(){
    if(m_Provider){
        QMetaObject::invokeMethod(m_Provider->base(), "FireClickEvent",
                                  Q_ARG(QString, m_XPath),
                                  Q_ARG(QPoint, Position()));
        return true;
    }
    return false;
}

QString JsWebElement::TagName() const {
    return m_TagName;
}

QString JsWebElement::InnerText() const {
    return m_InnerText;
}

QUrl JsWebElement::BaseUrl() const {
    return m_BaseUrl;
}

QUrl JsWebElement::LinkUrl() const {
    return m_LinkUrl;
}

QUrl JsWebElement::ImageUrl() const {
    return m_ImageUrl;
}

QString JsWebElement::LinkHtml() const {
    if(!m_LinkHtml.isEmpty()) return m_LinkHtml;
    return QStringLiteral("<a href=\"") + m_LinkUrl.toString() + QStringLiteral("\"></a>");
}

QString JsWebElement::ImageHtml() const {
    if(!m_ImageHtml.isEmpty()) return m_ImageHtml;
    return QStringLiteral("<img src=\"") + m_ImageUrl.toString() + QStringLiteral("\">");
}

QPoint JsWebElement::Position() const {
    return m_Rectangle.center();
}

QRect JsWebElement::Rectangle() const {
    return m_Rectangle;
}

QRegion JsWebElement::Region() const {
    return m_Region;
}

void JsWebElement::SetPosition(QPoint pos){
    m_Rectangle.moveCenter(pos);
}

void JsWebElement::SetRectangle(QRect rect){
    m_Rectangle = rect;
}

void JsWebElement::SetText(QString text){
    if(m_Provider){
        QMetaObject::invokeMethod(m_Provider->base(), "SetTextValue",
                                  Q_ARG(QString, m_XPath),
                                  Q_ARG(QString, text));
    }
}

QPixmap JsWebElement::Pixmap(){
    if(!m_Provider || IsNull()) return QPixmap();
    QPixmap pixmap(m_Provider->size());
    QPainter painter(&pixmap);
    QRect r;
    if(!Region().isNull()){
        m_Provider->Render(&painter, Region());
        r = Region().boundingRect();
    } else if(!Rectangle().isNull()){
        m_Provider->Render(&painter, Rectangle());
        r = Rectangle();
    }
    painter.end();
    return pixmap.copy(r);
}

bool JsWebElement::IsNull() const {
    return Position().isNull() || Rectangle().isNull();
}

bool JsWebElement::IsEditableElement() const {
    return m_IsEditable;
}

bool JsWebElement::IsJsCommandElement() const {
    return m_IsJsCommand;
}

bool JsWebElement::IsTextInputElement() const {
    return m_IsTextInput;
}

bool JsWebElement::IsQueryInputElement() const {
    return m_IsQueryInput;
}

bool JsWebElement::IsFrameElement() const {
    return m_IsFrame;
}

bool JsWebElement::IsLooped() const {
    return m_IsLooped;
}

bool JsWebElement::IsPaused() const {
    return m_IsPaused;
}

bool JsWebElement::IsMuted() const {
    return m_IsMuted;
}

WebElement::Action JsWebElement::GetAction() const {
    if(m_Action == QStringLiteral("Focus")) return Focus;
    if(m_Action == QStringLiteral("Click")) return Click;
    if(m_Action == QStringLiteral("Hover")) return Hover;
    return None;
}

bool JsWebElement::Equals(const WebElement &other) const {
    return m_XPath == static_cast<const JsWebElement*>(&other)->m_XPath;
}

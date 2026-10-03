#include "switch.hpp"
#include "const.hpp"

#include "accessiblewebelement.hpp"

#include <QGraphicsItem>
#include <QUrl>
#include <QPainter>
#include <QFontMetrics>
#include <QKeySequence>
#include <QMap>
#include <QGuiApplication>
#include <QScreen>

#include "view.hpp"
#include "gadgets.hpp"
#include "gadgetsstyle.hpp"
#include "devicescale.hpp"

namespace {

    int ScaleByDevice(int t){
        return DeviceScale::Primary(t);
    }

    QSize ScaleByDevice(const QSize &size){
        return DeviceScale::PrimarySize(size);
    }

}

AccessibleWebElement::AccessibleWebElement(QGraphicsItem *parent)
    : QGraphicsItem(parent)
{
    m_Gadgets = static_cast<Gadgets*>(parent);
    m_Index = -1;
    m_Element = SharedWebElement();
    m_Pos = QPoint();

    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setZValue(MAIN_CONTENTS_LAYER);
}

AccessibleWebElement::~AccessibleWebElement(){}

QRectF AccessibleWebElement::boundingRect() const {
    if(m_Pos.isNull() || !m_Element || m_Element->IsNull() || m_Index == -1)
        return QRectF();

    QUrl url = m_Element->LinkUrl();
    QString str = url.isEmpty() ? QStringLiteral("Blank Entry") : url.toString();

    if(!IsSelected()){
        return QRectF(CharChipRect());
    } else {
        int width = GetInfoMetrics().boundingRect(str).width();
        width = static_cast<int>(width + ScaleByDevice(15) - str.length()*0.4);

        if(width > ScaleByDevice(ACCESSKEY_INFO_MAX_WIDTH))
            width = ScaleByDevice(ACCESSKEY_INFO_MAX_WIDTH);

        const int infoHeight = ScaleByDevice(ACCESSKEY_INFO_HEIGHT);
        QPoint base(m_Pos - QPoint(width/2, infoHeight/2));
        QRect rect = QRect(base, QSize(width, infoHeight));

        QMap<QString, QRect> keyrectmap = KeyRects();
        QMap<QString, QRect> exprectmap = ExpRects();

        QStringList actions = m_Gadgets->GetAccessKeyKeyMap().values();
        actions.removeDuplicates();
        foreach(QString action, actions){
            rect = rect.united(keyrectmap[action]);
            rect = rect.united(exprectmap[action]);
        }
        return QRectF(rect);
    }
}

QPainterPath AccessibleWebElement::shape() const {
    QPainterPath path;
    if(m_Pos.isNull() || !m_Element || m_Element->IsNull()) return path;

    QUrl url = m_Element->LinkUrl();
    QString str = url.isEmpty() ? QStringLiteral("Blank Entry") : url.toString();

    if(!IsSelected()){
        path.addRect(CharChipRect());
    } else {
        int width = GetInfoMetrics().boundingRect(str).width();
        width = static_cast<int>(width + ScaleByDevice(15) - str.length()*0.4);

        if(width > ScaleByDevice(ACCESSKEY_INFO_MAX_WIDTH))
            width = ScaleByDevice(ACCESSKEY_INFO_MAX_WIDTH);

        const int infoHeight = ScaleByDevice(ACCESSKEY_INFO_HEIGHT);
        QPoint base(m_Pos - QPoint(width/2, infoHeight/2));
        path.addRect(QRect(base, QSize(width, infoHeight)));

        QMap<QString, QRect> keyrectmap = KeyRects();
        QMap<QString, QRect> exprectmap = ExpRects();

        QStringList actions = m_Gadgets->GetAccessKeyKeyMap().values();
        actions.removeDuplicates();
        foreach(QString action, actions){
            path.addRect(keyrectmap[action]);
            path.addRect(exprectmap[action]);
        }
    }
    return path;
}

void AccessibleWebElement::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option) Q_UNUSED(widget)
    m_Gadgets->GetStyle()->Render(this, painter);
}

void AccessibleWebElement::SetIndex(int index){
    m_Index = index;
}

void AccessibleWebElement::SetBoundingPos(QPoint pos){
    m_Pos = pos;
}

void AccessibleWebElement::SetElement(SharedWebElement elem){
    m_Element = elem;
    m_Gadgets->GetStyle()->OnSetElement(this, elem);
}

int AccessibleWebElement::GetIndex() const {
    return m_Index;
}

QPoint AccessibleWebElement::GetBoundingPos() const {
    return m_Pos;
}

SharedWebElement AccessibleWebElement::GetElement() const {
    return m_Element;
}

QPoint AccessibleWebElement::KeyExplanationBasePos() const {
    return QPoint(m_Pos - QPoint(ScaleByDevice(40),
                                 ScaleByDevice(ACCESSKEY_INFO_HEIGHT)/2));
}

QMap<QString, QRect> AccessibleWebElement::KeyRects() const {
    QMap<QString, QRect> map;

    int i = 0;
    QStringList actions = m_Gadgets->GetAccessKeyKeyMap().values();
    actions.removeDuplicates();
    foreach(QString action, actions){

        QStringList list;

        foreach(QKeySequence seq, m_Gadgets->GetAccessKeyKeyMap().keys(action)){
            list << seq.toString();
        }

        QString key = list.join(QStringLiteral(" or "));

        int width = GetSmallMetrics().boundingRect(key).width();

        const int chipHeight = ScaleByDevice(ACCESSKEY_CHAR_CHIP_S_SIZE.height());
        const int infoHeight = ScaleByDevice(ACCESSKEY_INFO_HEIGHT);
        QRect keyrect = QRect(KeyExplanationBasePos()
                              + QPoint(-ScaleByDevice(20),
                                       i * (chipHeight + infoHeight + ScaleByDevice(6))
                                       + infoHeight + ScaleByDevice(3)),
                              QSize(static_cast<int>(width + ScaleByDevice(15) - key.length()*0.4),
                                    chipHeight));
        map[action] = keyrect;
        i++;
    }
    return map;
}

QMap<QString, QRect> AccessibleWebElement::ExpRects() const {
    QMap<QString, QRect> map;

    int i = 0;
    QStringList actions = m_Gadgets->GetAccessKeyKeyMap().values();
    actions.removeDuplicates();
    foreach(QString action, actions){

        QString exp = action;

        int width = GetInfoMetrics().boundingRect(exp).width();

        const int chipHeight = ScaleByDevice(ACCESSKEY_CHAR_CHIP_S_SIZE.height());
        const int infoHeight = ScaleByDevice(ACCESSKEY_INFO_HEIGHT);
        QRect exprect = QRect(KeyExplanationBasePos()
                              + QPoint(0,
                                       i * (chipHeight + infoHeight + ScaleByDevice(6))
                                       + infoHeight
                                       + chipHeight + ScaleByDevice(6)),
                              QSize(static_cast<int>(width + ScaleByDevice(15) - exp.length()*0.4),
                                    infoHeight));
        map[action] = exprect;
        i++;
    }
    return map;
}

bool AccessibleWebElement::IsCurrentBlock() const {
    return m_Gadgets->IsCurrentBlock(this);
}

bool AccessibleWebElement::IsSelected() const {
    return isSelected();
}

QRect AccessibleWebElement::CharChipRect() const {
    QSize size;
    int len = m_Gadgets->IndexToString(m_Index).length();

    if(m_Element->IsFrameElement()){
        if(len>1) size = ACCESSKEY_CHAR_CHIP_LS_SIZE;
        else      size = ACCESSKEY_CHAR_CHIP_L_SIZE;
    } else if(m_Element->GetAction() != WebElement::None){
        if(len>1) size = ACCESSKEY_CHAR_CHIP_MS_SIZE;
        else      size = ACCESSKEY_CHAR_CHIP_M_SIZE;
    } else {
        if(len>1) size = ACCESSKEY_CHAR_CHIP_SS_SIZE;
        else      size = ACCESSKEY_CHAR_CHIP_S_SIZE;
    }
    size = ScaleByDevice(size);
    size.setWidth(size.width()*len);
    return QRect(m_Pos - QPoint(size.width()/2, size.height()/2), size);
}

QFont AccessibleWebElement::CharChipFont() const {
    int len = m_Gadgets->IndexToString(m_Index).length();

    if(m_Element->IsFrameElement()){
        if(len>1) return AccessKeyChipLSFont();
        else      return AccessKeyChipLFont();
    } else if(m_Element->GetAction() != WebElement::None){
        if(len>1) return AccessKeyChipMSFont();
        else      return AccessKeyChipMFont();
    } else {
        if(len>1) return AccessKeyChipSSFont();
        else      return AccessKeyChipSFont();
    }
}

void AccessibleWebElement::UpdateMinimal(){
    if(m_Pos.isNull() || !m_Element || m_Element->IsNull()) return;

    QUrl url = m_Element->LinkUrl();
    QString str = url.isEmpty() ? QStringLiteral("Blank Entry") : url.toString();

    if(!IsSelected()){
        m_Gadgets->update(CharChipRect());
        return;
    } else {
        int width = GetInfoMetrics().boundingRect(str).width();
        width = static_cast<int>(width + ScaleByDevice(15) - str.length()*0.4);

        if(width > ScaleByDevice(ACCESSKEY_INFO_MAX_WIDTH))
            width = ScaleByDevice(ACCESSKEY_INFO_MAX_WIDTH);

        const int infoHeight = ScaleByDevice(ACCESSKEY_INFO_HEIGHT);
        QPoint base(m_Pos - QPoint(width/2, infoHeight/2));
        m_Gadgets->update(QRect(base, QSize(width, infoHeight)));

        QMap<QString, QRect> keyrectmap = KeyRects();
        QMap<QString, QRect> exprectmap = ExpRects();

        QStringList actions = m_Gadgets->GetAccessKeyKeyMap().values();
        actions.removeDuplicates();
        foreach(QString action, actions){
            m_Gadgets->update(keyrectmap[action]);
            m_Gadgets->update(exprectmap[action]);
        }
    }
}

QVariant AccessibleWebElement::itemChange(GraphicsItemChange change, const QVariant &value){
    if(change == ItemSelectedChange && scene()){

        if(value.toBool()){
            setZValue(DRAGGING_CONTENTS_LAYER);
        } else {
            setZValue(MAIN_CONTENTS_LAYER);
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

void AccessibleWebElement::mousePressEvent   (QGraphicsSceneMouseEvent *ev){ ev->setAccepted(false);}
void AccessibleWebElement::mouseReleaseEvent (QGraphicsSceneMouseEvent *ev){ ev->setAccepted(false);}
void AccessibleWebElement::mouseMoveEvent    (QGraphicsSceneMouseEvent *ev){ ev->setAccepted(false);}
void AccessibleWebElement::hoverEnterEvent   (QGraphicsSceneHoverEvent *ev){ ev->setAccepted(false);}
void AccessibleWebElement::hoverLeaveEvent   (QGraphicsSceneHoverEvent *ev){ ev->setAccepted(false);}
void AccessibleWebElement::hoverMoveEvent    (QGraphicsSceneHoverEvent *ev){ ev->setAccepted(false);}

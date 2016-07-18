#ifndef GADGETSSTYLE_HPP
#define GADGETSSTYLE_HPP

#include "switch.hpp"

#include "view.hpp" // for SharedWebElement.

#include "graphicstableview.hpp"

class AccessibleWebElement;

class GadgetsStyle{
public:
    explicit GadgetsStyle(){}
    virtual ~GadgetsStyle(){}

    virtual QString StyleName() const = 0;

    virtual void ComputeContentsLayout(GraphicsTableView *gtv, int &col, int &line, int &thumbWidth, int &thumbHeight) const = 0;
    virtual void RenderBackground(GraphicsTableView *gtv, QPainter *painter) const = 0;
    virtual void Render(Thumbnail *thumb, QPainter *painter) const = 0;
    virtual void Render(NodeTitle *title, QPainter *painter) const = 0;
    virtual void Render(SpotLight *light, QPainter *painter) const = 0;
    virtual void Render(InPlaceNotifier *notifier, QPainter *painter) const = 0;
    virtual void Render(CloseButton *button, QPainter *painter) const = 0;
    virtual void Render(CloneButton *button, QPainter *painter) const = 0;
#if QT_VERSION >= 0x050700
    virtual void Render(SoundButton *button, QPainter *painter) const = 0;
#endif
    virtual void Render(UpDirectoryButton *button, QPainter *painter) const = 0;
    virtual void Render(ToggleTrashButton *button, QPainter *painter) const = 0;
    virtual void Render(AccessibleWebElement *awe, QPainter *painter) const = 0;
    virtual void OnSetNest(Thumbnail *thumb, int nest) const = 0;
    virtual void OnSetNest(NodeTitle *title, int nest) const = 0;
    virtual void OnSetPrimary(Thumbnail *thumb, bool primary) const = 0;
    virtual void OnSetPrimary(NodeTitle *title, bool primary) const = 0;
    virtual void OnSetHovered(Thumbnail *thumb, bool hovered) const = 0;
    virtual void OnSetHovered(NodeTitle *thumb, bool hovered) const = 0;
    virtual void OnSetState(GraphicsButton *button, GraphicsButton::ButtonState) const = 0;
    virtual void OnSetElement(AccessibleWebElement *awe, SharedWebElement elem) const = 0;
    virtual void OnReshow(QGraphicsRectItem *gri) const = 0;
    virtual QRectF ThumbnailAreaRect(GraphicsTableView *gtv) const = 0;
    virtual QRectF NodeTitleAreaRect(GraphicsTableView *gtv) const = 0;
    virtual QRectF ScrollBarAreaRect(GraphicsTableView *gtv) const = 0;
    virtual QGraphicsRectItem *CreateSelectRect(GraphicsTableView *gtv, QPointF pos) const = 0;

    virtual bool ScrollToChangeDirectory(bool value) const = 0;
    virtual bool RightClickToRenameNode(bool value) const = 0;
    virtual bool UseGraphicsItemUpdate() const = 0;

    virtual bool NodeTitleDrawBorder() const = 0;
    virtual int NodeTitleHeight(GraphicsTableView *gtv) const = 0;
    virtual int InPlaceNotifierWidth() const = 0;
    virtual int InPlaceNotifierHeight() const = 0;
};

class GlassStyle : public GadgetsStyle{
public:
    explicit GlassStyle(){}
    virtual ~GlassStyle(){}

    QString StyleName() const Q_DECL_OVERRIDE { return QStringLiteral("GlassStyle");}

    static const QFont m_ThumbnailTitleFont;

    static const int m_ThumbnailPaddingX;
    static const int m_ThumbnailPaddingY;
    static const int m_ThumbnailTitleHeight;
    static const int m_ThumbnailWidthPercentage;
    static const int m_ThumbnailDefaultColumnCount;
    static const int m_ThumbnailAreaWidthPercentage;

    static const bool m_ThumbnailDrawBorder;

    static const QFont m_NodeTitleFont;

    static const int m_NodeTitleHeight;
    static const bool m_NodeTitleDrawBorder;

    static const int m_InPlaceNotifierWidth;
    static const int m_InPlaceNotifierHeight;
    static const bool m_InPlaceNotifierDrawBorder;

    void ComputeContentsLayout(GraphicsTableView *gtv, int &col, int &line, int &thumbWidth, int &thumbHeight) const Q_DECL_OVERRIDE;
    void RenderBackground(GraphicsTableView *gtv, QPainter *painter) const Q_DECL_OVERRIDE;
    void Render(Thumbnail *thumb, QPainter *painter) const Q_DECL_OVERRIDE;
    void Render(NodeTitle *title, QPainter *painter) const Q_DECL_OVERRIDE;
    void Render(SpotLight *light, QPainter *painter) const Q_DECL_OVERRIDE;
    void Render(InPlaceNotifier *notifier, QPainter *painter) const Q_DECL_OVERRIDE;
    void Render(CloseButton *button, QPainter *painter) const Q_DECL_OVERRIDE;
    void Render(CloneButton *button, QPainter *painter) const Q_DECL_OVERRIDE;
#if QT_VERSION >= 0x050700
    void Render(SoundButton *button, QPainter *painter) const Q_DECL_OVERRIDE;
#endif
    void Render(UpDirectoryButton *button, QPainter *painter) const Q_DECL_OVERRIDE;
    void Render(ToggleTrashButton *button, QPainter *painter) const Q_DECL_OVERRIDE;
    void Render(AccessibleWebElement *awe, QPainter *painter) const Q_DECL_OVERRIDE;
    void OnSetNest(Thumbnail *thumb, int nest) const Q_DECL_OVERRIDE;
    void OnSetNest(NodeTitle *title, int nest) const Q_DECL_OVERRIDE;
    void OnSetPrimary(Thumbnail *thumb, bool primary) const Q_DECL_OVERRIDE;
    void OnSetPrimary(NodeTitle *title, bool primary) const Q_DECL_OVERRIDE;
    void OnSetHovered(Thumbnail *thumb, bool hovered) const Q_DECL_OVERRIDE;
    void OnSetHovered(NodeTitle *title, bool hovered) const Q_DECL_OVERRIDE;
    void OnSetState(GraphicsButton *button, GraphicsButton::ButtonState) const Q_DECL_OVERRIDE;
    void OnSetElement(AccessibleWebElement *awe, SharedWebElement elem) const Q_DECL_OVERRIDE;
    void OnReshow(QGraphicsRectItem *gri) const Q_DECL_OVERRIDE;
    QRectF ThumbnailAreaRect(GraphicsTableView *gtv) const Q_DECL_OVERRIDE;
    QRectF NodeTitleAreaRect(GraphicsTableView *gtv) const Q_DECL_OVERRIDE;
    QRectF ScrollBarAreaRect(GraphicsTableView *gtv) const Q_DECL_OVERRIDE;
    QGraphicsRectItem *CreateSelectRect(GraphicsTableView *gtv, QPointF pos) const Q_DECL_OVERRIDE;

    bool ScrollToChangeDirectory(bool value) const Q_DECL_OVERRIDE { return value;}
    bool RightClickToRenameNode(bool value) const Q_DECL_OVERRIDE { return value;}
    bool UseGraphicsItemUpdate() const Q_DECL_OVERRIDE { return false;}

    bool NodeTitleDrawBorder() const Q_DECL_OVERRIDE { return m_NodeTitleDrawBorder;}
    int NodeTitleHeight(GraphicsTableView *gtv) const Q_DECL_OVERRIDE;
    int InPlaceNotifierWidth() const Q_DECL_OVERRIDE { return m_InPlaceNotifierWidth;}
    int InPlaceNotifierHeight() const Q_DECL_OVERRIDE { return m_InPlaceNotifierHeight;}
};

class FlatStyle : public GadgetsStyle{
public:
    explicit FlatStyle(){}
    virtual ~FlatStyle(){}

    QString StyleName() const Q_DECL_OVERRIDE { return QStringLiteral("FlatStyle");}

    static const QFont m_ThumbnailTitleFont;

    static const int m_ThumbnailPaddingX;
    static const int m_ThumbnailPaddingY;
    static const int m_ThumbnailTitleHeight;
    static const int m_ThumbnailWidthPercentage;
    static const int m_ThumbnailDefaultColumnCount;
    static const int m_ThumbnailAreaWidthPercentage;

    static const bool m_ThumbnailDrawBorder;

    void ComputeContentsLayout(GraphicsTableView *gtv, int &col, int &line, int &thumbWidth, int &thumbHeight) const Q_DECL_OVERRIDE;
    void RenderBackground(GraphicsTableView *gtv, QPainter *painter) const Q_DECL_OVERRIDE;
    void Render(Thumbnail *thumb, QPainter *painter) const Q_DECL_OVERRIDE;
    void Render(NodeTitle *title, QPainter *painter) const Q_DECL_OVERRIDE;
    void Render(SpotLight *light, QPainter *painter) const Q_DECL_OVERRIDE;
    void Render(InPlaceNotifier *notifier, QPainter *painter) const Q_DECL_OVERRIDE;
    void Render(CloseButton *button, QPainter *painter) const Q_DECL_OVERRIDE;
    void Render(CloneButton *button, QPainter *painter) const Q_DECL_OVERRIDE;
#if QT_VERSION >= 0x050700
    void Render(SoundButton *button, QPainter *painter) const Q_DECL_OVERRIDE;
#endif
    void Render(UpDirectoryButton *button, QPainter *painter) const Q_DECL_OVERRIDE;
    void Render(ToggleTrashButton *button, QPainter *painter) const Q_DECL_OVERRIDE;
    void Render(AccessibleWebElement *awe, QPainter *painter) const Q_DECL_OVERRIDE;
    void OnSetNest(Thumbnail *thumb, int nest) const Q_DECL_OVERRIDE;
    void OnSetNest(NodeTitle *title, int nest) const Q_DECL_OVERRIDE;
    void OnSetPrimary(Thumbnail *thumb, bool primary) const Q_DECL_OVERRIDE;
    void OnSetPrimary(NodeTitle *title, bool primary) const Q_DECL_OVERRIDE;
    void OnSetHovered(Thumbnail *thumb, bool hovered) const Q_DECL_OVERRIDE;
    void OnSetHovered(NodeTitle *title, bool hovered) const Q_DECL_OVERRIDE;
    void OnSetState(GraphicsButton *button, GraphicsButton::ButtonState) const Q_DECL_OVERRIDE;
    void OnSetElement(AccessibleWebElement *awe, SharedWebElement elem) const Q_DECL_OVERRIDE;
    void OnReshow(QGraphicsRectItem *gri) const Q_DECL_OVERRIDE;
    QRectF ThumbnailAreaRect(GraphicsTableView *gtv) const Q_DECL_OVERRIDE;
    QRectF NodeTitleAreaRect(GraphicsTableView *gtv) const Q_DECL_OVERRIDE;
    QRectF ScrollBarAreaRect(GraphicsTableView *gtv) const Q_DECL_OVERRIDE;
    QGraphicsRectItem *CreateSelectRect(GraphicsTableView *gtv, QPointF pos) const Q_DECL_OVERRIDE;

    bool ScrollToChangeDirectory(bool) const Q_DECL_OVERRIDE { return false;}
    bool RightClickToRenameNode(bool) const Q_DECL_OVERRIDE { return false;}
    bool UseGraphicsItemUpdate() const Q_DECL_OVERRIDE { return true;}

    bool NodeTitleDrawBorder() const Q_DECL_OVERRIDE { return false;}
    int NodeTitleHeight(GraphicsTableView*) const Q_DECL_OVERRIDE { return 1;} // dummy value.
    int InPlaceNotifierWidth() const Q_DECL_OVERRIDE { return 706;} // dummy value.
    int InPlaceNotifierHeight() const Q_DECL_OVERRIDE { return 154;} // dummy value.
};

#endif //ifndef GADGETSSTYLE_HPP

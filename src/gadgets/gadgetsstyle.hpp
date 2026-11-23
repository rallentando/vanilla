#ifndef GADGETSSTYLE_HPP
#define GADGETSSTYLE_HPP

#include "switch.hpp"

#include "view.hpp"

#include "graphicstableview.hpp"

class AccessibleWebElement;

class GadgetsStyle {
public:
    explicit GadgetsStyle(){}
    virtual ~GadgetsStyle(){}

    virtual QString StyleName() const = 0;

    virtual void ComputeContentsLayout(GraphicsTableView *gtv, int &col, int &line, int &thumbWidth, int &thumbHeight) const = 0;
    virtual void RenderBackground(GraphicsTableView *gtv, QPainter *painter) const = 0;
    virtual QColor BackgroundVeil(GraphicsTableView *gtv) const = 0;
    virtual void RenderBackgroundOverVeil(GraphicsTableView *gtv, QPainter *painter) const = 0;
    virtual void Render(Thumbnail *thumb, QPainter *painter) const = 0;
    virtual void Render(NodeTitle *title, QPainter *painter) const = 0;
    virtual void Render(SpotLight *light, QPainter *painter) const = 0;
    virtual void Render(InPlaceNotifier *notifier, QPainter *painter) const = 0;
    virtual void Render(CloseButton *button, QPainter *painter) const = 0;
    virtual void Render(CloneButton *button, QPainter *painter) const = 0;
    virtual void Render(SoundButton *button, QPainter *painter) const = 0;
    virtual void Render(UpDirectoryButton *button, QPainter *painter) const = 0;
    virtual void Render(ToggleTrashButton *button, QPainter *painter) const = 0;
    virtual void Render(CloseTableButton *button, QPainter *painter) const = 0;
    virtual void Render(TableHeader *header, QPainter *painter) const = 0;
    virtual void Render(HeaderFilter *filter, QPainter *painter) const = 0;
    virtual void Render(AccessibleWebElement *awe, QPainter *painter) const = 0;
    virtual void Render(ScrollIndicator *indicator, QPainter *painter) const;
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
    virtual QRectF ThumbnailHitRect(const Thumbnail *thumb) const = 0;

    virtual bool ScrollToChangeDirectory(bool value) const = 0;
    virtual bool RightClickToRenameNode(bool value) const = 0;
    virtual bool UseGraphicsItemUpdate() const = 0;
    virtual bool DrawsScrollBarTrack() const { return true;}
    virtual bool CoversViewBehind(GraphicsTableView*) const { return false;}

    virtual bool NodeTitleDrawBorder() const = 0;
    virtual int NodeTitleHeight(GraphicsTableView *gtv) const = 0;
    virtual int InPlaceNotifierWidth() const = 0;
    virtual int InPlaceNotifierHeight() const = 0;
};

class GlassStyle : public GadgetsStyle {
public:
    explicit GlassStyle(){}
    virtual ~GlassStyle() override {}

    QString StyleName() const override { return QStringLiteral("GlassStyle");}

    static const int m_ThumbnailPaddingX;
    static const int m_ThumbnailPaddingY;
    static const int m_ThumbnailTitleHeight;
    static const int m_ThumbnailWidthPercentage;
    static const int m_ThumbnailDefaultColumnCount;
    static const int m_ThumbnailAreaWidthPercentage;

    static const bool m_ThumbnailDrawBorder;

    static const int m_NodeTitleHeight;
    static const bool m_NodeTitleDrawBorder;

    static const int m_InPlaceNotifierWidth;
    static const int m_InPlaceNotifierHeight;
    static const bool m_InPlaceNotifierDrawBorder;

    void ComputeContentsLayout(GraphicsTableView *gtv, int &col, int &line, int &thumbWidth, int &thumbHeight) const override;
    void RenderBackground(GraphicsTableView *gtv, QPainter *painter) const override;
    QColor BackgroundVeil(GraphicsTableView *gtv) const override;
    void RenderBackgroundOverVeil(GraphicsTableView *gtv, QPainter *painter) const override;
    void Render(Thumbnail *thumb, QPainter *painter) const override;
    void Render(NodeTitle *title, QPainter *painter) const override;
    void Render(SpotLight *light, QPainter *painter) const override;
    void Render(InPlaceNotifier *notifier, QPainter *painter) const override;
    void Render(CloseButton *button, QPainter *painter) const override;
    void Render(CloneButton *button, QPainter *painter) const override;
    void Render(SoundButton *button, QPainter *painter) const override;
    void Render(UpDirectoryButton *button, QPainter *painter) const override;
    void Render(ToggleTrashButton *button, QPainter *painter) const override;
    void Render(CloseTableButton *button, QPainter *painter) const override;
    void Render(TableHeader *header, QPainter *painter) const override;
    void Render(HeaderFilter *filter, QPainter *painter) const override;
    void Render(AccessibleWebElement *awe, QPainter *painter) const override;
    void OnSetNest(Thumbnail *thumb, int nest) const override;
    void OnSetNest(NodeTitle *title, int nest) const override;
    void OnSetPrimary(Thumbnail *thumb, bool primary) const override;
    void OnSetPrimary(NodeTitle *title, bool primary) const override;
    void OnSetHovered(Thumbnail *thumb, bool hovered) const override;
    void OnSetHovered(NodeTitle *title, bool hovered) const override;
    void OnSetState(GraphicsButton *button, GraphicsButton::ButtonState) const override;
    void OnSetElement(AccessibleWebElement *awe, SharedWebElement elem) const override;
    void OnReshow(QGraphicsRectItem *gri) const override;
    QRectF ThumbnailAreaRect(GraphicsTableView *gtv) const override;
    QRectF NodeTitleAreaRect(GraphicsTableView *gtv) const override;
    QRectF ScrollBarAreaRect(GraphicsTableView *gtv) const override;
    QGraphicsRectItem *CreateSelectRect(GraphicsTableView *gtv, QPointF pos) const override;
    QRectF ThumbnailHitRect(const Thumbnail *thumb) const override;

    bool ScrollToChangeDirectory(bool value) const override { return value;}
    bool RightClickToRenameNode(bool value) const override { return value;}
    bool UseGraphicsItemUpdate() const override { return false;}

    bool NodeTitleDrawBorder() const override { return m_NodeTitleDrawBorder;}
    int NodeTitleHeight(GraphicsTableView *gtv) const override;
    int InPlaceNotifierWidth() const override { return m_InPlaceNotifierWidth;}
    int InPlaceNotifierHeight() const override { return m_InPlaceNotifierHeight;}
};

class FlatStyle : public GadgetsStyle {
public:
    explicit FlatStyle(){}
    virtual ~FlatStyle() override {}

    QString StyleName() const override { return QStringLiteral("FlatStyle");}

    static const int m_ThumbnailPaddingX;
    static const int m_ThumbnailPaddingY;
    static const int m_ThumbnailTitleHeight;
    static const int m_ThumbnailWidthPercentage;
    static const int m_ThumbnailDefaultColumnCount;
    static const int m_ThumbnailAreaWidthPercentage;

    static const bool m_ThumbnailDrawBorder;

    void ComputeContentsLayout(GraphicsTableView *gtv, int &col, int &line, int &thumbWidth, int &thumbHeight) const override;
    void RenderBackground(GraphicsTableView *gtv, QPainter *painter) const override;
    QColor BackgroundVeil(GraphicsTableView *gtv) const override;
    void RenderBackgroundOverVeil(GraphicsTableView *gtv, QPainter *painter) const override;
    void Render(Thumbnail *thumb, QPainter *painter) const override;
    void Render(NodeTitle *title, QPainter *painter) const override;
    void Render(SpotLight *light, QPainter *painter) const override;
    void Render(InPlaceNotifier *notifier, QPainter *painter) const override;
    void Render(CloseButton *button, QPainter *painter) const override;
    void Render(CloneButton *button, QPainter *painter) const override;
    void Render(SoundButton *button, QPainter *painter) const override;
    void Render(UpDirectoryButton *button, QPainter *painter) const override;
    void Render(ToggleTrashButton *button, QPainter *painter) const override;
    void Render(CloseTableButton *button, QPainter *painter) const override;
    void Render(TableHeader *header, QPainter *painter) const override;
    void Render(HeaderFilter *filter, QPainter *painter) const override;
    void Render(AccessibleWebElement *awe, QPainter *painter) const override;
    void OnSetNest(Thumbnail *thumb, int nest) const override;
    void OnSetNest(NodeTitle *title, int nest) const override;
    void OnSetPrimary(Thumbnail *thumb, bool primary) const override;
    void OnSetPrimary(NodeTitle *title, bool primary) const override;
    void OnSetHovered(Thumbnail *thumb, bool hovered) const override;
    void OnSetHovered(NodeTitle *title, bool hovered) const override;
    void OnSetState(GraphicsButton *button, GraphicsButton::ButtonState) const override;
    void OnSetElement(AccessibleWebElement *awe, SharedWebElement elem) const override;
    void OnReshow(QGraphicsRectItem *gri) const override;
    QRectF ThumbnailAreaRect(GraphicsTableView *gtv) const override;
    QRectF NodeTitleAreaRect(GraphicsTableView *gtv) const override;
    QRectF ScrollBarAreaRect(GraphicsTableView *gtv) const override;
    QGraphicsRectItem *CreateSelectRect(GraphicsTableView *gtv, QPointF pos) const override;
    QRectF ThumbnailHitRect(const Thumbnail *thumb) const override;

    bool ScrollToChangeDirectory(bool) const override { return false;}
    bool RightClickToRenameNode(bool) const override { return false;}
    bool UseGraphicsItemUpdate() const override { return true;}

    bool NodeTitleDrawBorder() const override { return false;}
    int NodeTitleHeight(GraphicsTableView*) const override { return 1;}
    int InPlaceNotifierWidth() const override { return 706;}
    int InPlaceNotifierHeight() const override { return 154;}
};

class PlainStyle : public FlatStyle {
public:
    explicit PlainStyle(){}
    virtual ~PlainStyle() override {}

    QString StyleName() const override { return QStringLiteral("PlainStyle");}

    static const int m_CardMargin;
    static const int m_ImagePadding;
    static const int m_TitleHeight;
    static const int m_CornerRadius;
    static const int m_HoverGrowth;
    static const int m_RingWidth;
    static const int m_EdgeWidth;
    static const int m_ScrollThumbWidth;
    static const int m_RoundedCellMinWidth;

    void RenderBackground(GraphicsTableView *gtv, QPainter *painter) const override;
    QColor BackgroundVeil(GraphicsTableView *gtv) const override;
    void RenderBackgroundOverVeil(GraphicsTableView *gtv, QPainter *painter) const override;
    void Render(Thumbnail *thumb, QPainter *painter) const override;
    void Render(CloseButton *button, QPainter *painter) const override;
    void Render(CloneButton *button, QPainter *painter) const override;
    void Render(SoundButton *button, QPainter *painter) const override;
    void Render(UpDirectoryButton *button, QPainter *painter) const override;
    void Render(ToggleTrashButton *button, QPainter *painter) const override;
    void Render(CloseTableButton *button, QPainter *painter) const override;
    void Render(TableHeader *header, QPainter *painter) const override;
    void Render(HeaderFilter *filter, QPainter *painter) const override;
    void Render(ScrollIndicator *indicator, QPainter *painter) const override;
    void OnSetState(GraphicsButton *button, GraphicsButton::ButtonState) const override;
    QRectF ThumbnailHitRect(const Thumbnail *thumb) const override;

    bool DrawsScrollBarTrack() const override { return false;}
    bool CoversViewBehind(GraphicsTableView *gtv) const override;
};

#endif

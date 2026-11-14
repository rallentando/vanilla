#ifndef TITLEBAR_HPP
#define TITLEBAR_HPP

#include "switch.hpp"
#include "const.hpp"
#include "devicescale.hpp"

#include <QWidget>

#include <memory>
#include <QPointer>

class MainWindow;
class TreeBank;
class View;
class ExtensionBar;
class LineEdit;
class SiteButton;
class SiteInfoPanel;
class QToolButton;
class QPainter;
class QVariantAnimation;
class TitleEditRow;

class TitleBar : public QWidget {
    Q_OBJECT
public:
    enum Mode { Inner, Outer, Strip };

    enum Button {
        NoButton,
        MenuButton,
        ViewTreeButton,
        ShadeButton,
        MinimizeButton,
        MaximizeButton,
        CloseButton,
        ExpandButton
    };
    static constexpr Button AllButtons[] = {ExpandButton, MenuButton, ViewTreeButton, ShadeButton,
                                            MinimizeButton, MaximizeButton, CloseButton};

    TitleBar(MainWindow *mainwindow, Mode mode);
    Mode GetMode() const { return m_Mode;}

    int CompactWidth() const {
        return ScaleByDevice(SideButtonWidth)*3 + ScaleByDevice(CaptionButtonWidth)*3;
    }

    template <class T> T ScaleByDevice(T v) const {
        return DeviceScale::FromDpi(v, static_cast<int>(logicalDpiY()));
    }

    QRect ButtonRect(Button button) const;
    Button ButtonAt(const QPoint &pos) const;

    bool Reveal(bool on, bool animate);
    void SetStripGeometry(const QRect &target);
    bool IsSliding() const;

    void SetNativeHover(bool hovered);
    void SetNativePressed(bool pressed);
    bool IsNativeHovered() const { return m_NativeHovered;}
    bool IsNativePressed() const { return m_NativePressed;}

    QRect TitleRect() const;
    QRect EditRect() const;
    bool IsEditing() const;
    LineEdit *GetAddressDisplay() const;
    QList<QToolButton*> NavigationButtons() const;
    ExtensionBar *GetExtensionBar() const;
    SiteButton *GetSiteButton() const;
    void BeginEdit();
    void EndEdit(bool giveBack = false);
    bool IsEditable() const;
protected:
    void paintEvent(QPaintEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void mouseDoubleClickEvent(QMouseEvent *ev) override;
    void leaveEvent(QEvent *ev) override;
    void resizeEvent(QResizeEvent *ev) override;
    void showEvent(QShowEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;
    bool event(QEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;

#if defined(Q_OS_WIN)
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#endif

private:
    static const int CaptionButtonWidth = 46;
    static const int SideButtonWidth = 40;

    bool IsCompact() const;
    int BarHeight() const;
    void ApplyRevealGeometry();
    void FinishReveal();
    QVariantAnimation *m_RevealAnimation = nullptr;
    qreal m_RevealShown = 1.0;
    bool m_RevealTarget = false;
    QRect m_StripTarget;
    bool m_Rounded = true;
    bool IsTranslucent() const;
    bool IsDrag(const QPoint &moved) const;
    void StartSystemMove();
    void Trigger(Button button);
    void PaintGlyph(QPainter &painter, Button button, const QRect &rect, const QColor &color) const;

    MainWindow *m_MainWindow;
    Mode m_Mode;
    Button m_Hovered;
    Button m_Pressed;
    bool m_NativeHovered;
    bool m_NativePressed;
    TitleEditRow *m_EditRow = nullptr;
    bool m_EditPending;
    QPoint m_PressPos;
    QPoint m_Pos;
    bool m_Moved;
};

class TitleEditRow : public QObject {
    Q_OBJECT
public:
    TitleEditRow(TitleBar *bar, TreeBank *bank);

    bool IsEditing() const { return m_Editing;}
    LineEdit *Address() const { return m_Address;}
    SiteButton *Site() const { return m_Site;}
    QList<QToolButton*> NavigationButtons() const { return m_Navigation;}
    ExtensionBar *Extensions() const { return m_Extensions;}

    void Begin(const std::shared_ptr<View> &view);
    void End();
    void Layout();

private slots:
    void SyncNavigation();
    void OnAnchorReleased(QWidget *field, bool focusLeft);

private:
    TitleBar *m_Bar;
    TreeBank *m_TreeBank;
    LineEdit *m_Address;
    std::shared_ptr<View> m_View;
    QList<QToolButton*> m_Navigation;
    SiteButton *m_Site;
    QPointer<SiteInfoPanel> m_SitePanel;
    void OpenSitePanel();
    ExtensionBar *m_Extensions;
    bool m_Editing = false;
    bool m_Kept = false;
    void SetKept(bool on);
    void OpenLine();
    bool IsOurs(const QObject *object) const;
    bool eventFilter(QObject *watched, QEvent *ev) override;
};

#endif

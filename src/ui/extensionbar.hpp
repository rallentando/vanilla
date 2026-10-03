#ifndef EXTENSIONBAR_HPP
#define EXTENSIONBAR_HPP

#include "switch.hpp"
#include "devicescale.hpp"

#include "view.hpp"
#include "extensioncontroller.hpp"

#include <QWidget>
#include <QPointer>
#include <QIcon>

class QToolButton;
class QHBoxLayout;
class QScrollArea;
class ExtensionWindow;

class ExtensionBar : public QWidget {
    Q_OBJECT

public:
    explicit ExtensionBar(QWidget *parent);
    ~ExtensionBar() Q_DECL_OVERRIDE;

    template <class T> T ScaleByDevice(T t) const {
        return DeviceScale::FromDpi(t, static_cast<int>(logicalDpiY()));
    }

    void SetView(const SharedView &view);

    static QIcon PuzzleIcon();

    void ApplyTheme();

public slots:
    void ContextChanged();
    void Refresh();
    void ShowList();
    void ActionChanged(const QString &id);
    void Execute(const QString &id, qint64 serial);
    void UrlChanged();

private:
    void Rebind();
    void RefreshList();
    void ClosePopup();
    void Open(const QString &path, bool options = false);
    void AddDirectory();
    void ManageExtensions();
    void More(const QString &path, QToolButton *button, const QPoint &at = QPoint());

    QToolButton *BarButton(const QString &text);
    ExtensionUi::ActionShown ShownFor(const QString &id) const;
    void Decorate(QToolButton *button, const ExtensionRow &row);
    void Click(const QString &path);
    void Pressed(const QString &path);
    bool PanelInstead(const QString &path);
    void Press(const QString &path, bool popup);
    SharedView OwnView() const;

    WeakView m_View;
    QPointer<ExtensionController> m_Controller;
    QMetaObject::Connection m_ContextConnection;
    QMetaObject::Connection m_DestroyConnection;
    QMetaObject::Connection m_UrlConnection;
    QToolButton *m_Button;
    QHBoxLayout *m_Layout;
    QPointer<ExtensionWindow> m_List;
    QPointer<ExtensionWindow> m_Popup;
    QPointer<QScrollArea> m_Rows;
    QList<QToolButton*> m_Pins;
};

#endif

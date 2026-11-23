#ifndef RECEIVER_HPP
#define RECEIVER_HPP

#include "switch.hpp"

#include "devicescale.hpp"

#include <QWidget>
#include <QLineEdit>
#include <QIcon>
#include <QPointer>
#include <QVector>

#include "view.hpp"
#include "inputmapschema.hpp"

class QHideEvent;
class QShowEvent;
class QKeyEvent;
class QKeySequence;
class QFocusEvent;
class QPaintEvent;
class QTimerEvent;
class QResizeEvent;
class QLocalServer;
class QLocalSocket;

class TreeBank;
class View;

class LineEdit : public QLineEdit {
    Q_OBJECT

public:
    LineEdit(QWidget *parent = nullptr);
    ~LineEdit() override;
    bool event(QEvent *ev) override;

    void ApplyTheme();
    void SetPill(bool on);
    bool IsPill() const { return m_Pill;}
    void SetCount(const QString &count);
    QString Count() const { return m_Count;}
    void SetTag(const QString &tag);
    QString Tag() const { return m_Tag;}
    void SetClosable(bool on);
    QRect CloseRect() const;

    static const char *const AddressHoverProperty;
    static QRect AddressRect(const QWidget *site, const QWidget *field);
    static bool AddressHovered(const QWidget *host, const QWidget *site, const QWidget *field);
    static void PaintAddressHover(QPainter *painter, const QWidget *host,
                                  const QWidget *site, const QWidget *field);

    template <class T> T ScaleByDevice(T t) const {
        return DeviceScale::FromDpi(t, static_cast<int>(logicalDpiY()));
    }

protected:
    void paintEvent(QPaintEvent *ev) override;
    void focusInEvent(QFocusEvent *ev) override;
    void focusOutEvent(QFocusEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void inputMethodEvent(QInputMethodEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mouseDoubleClickEvent(QMouseEvent *ev) override;
    void leaveEvent(QEvent *ev) override;
    void resizeEvent(QResizeEvent *ev) override;

signals:
    void Returned();
    void Aborted();
    void FocusIn(Qt::FocusReason);
    void FocusOut(Qt::FocusReason);
    void SelectNextSuggest();
    void SelectPrevSuggest();

private:
    bool m_Pill = false;
    QString m_Count;
    QString m_Tag;
    bool m_Closable = false;
    bool m_CloseHovered = false;
    bool m_ClosePressed = false;
    void SetCloseHovered(bool on);
    int CloseReserve() const;
    void UpdateTextMargins();
};

class Receiver : public QWidget {
    Q_OBJECT

public:
    Receiver(TreeBank *parent = nullptr, bool purge = false);
    ~Receiver() override;

    template <class T> T ScaleByDevice(T t) const {
        return DeviceScale::FromDpi(t, static_cast<int>(logicalDpiY()));
    }

    bool IsPurged() const;
    void Purge();
    void Join();
    void ResizeNotify(QSize size);
    void RefillIfShown();
    QString SoughtWords() const;
    void RefillKeepingChoice();
    void AskForSuggestions();
    void OpenTextSeeker(View* = nullptr, const QString &words = QString());
    void OpenQueryEditor(View* = nullptr, const QString &words = QString());
    void OpenUrlEditor(View* = nullptr);
    void OpenCommand(View* = nullptr);
    void OpenAtField(QWidget *field);
    bool IsAnchored() const { return m_Anchor;}

    struct TargetArgs {
        SharedWebElement element;
        WeakView view;
        quint64 node = 0;
        quint64 loadSerial = 0;
        QString selection;
        QUrl base;
        QPoint hitPos;
        bool playable = false;
        QString selectionHtml;
        QList<quint64> nodes;
        QString nodeTable;
        bool trash = false;
    };
    void OpenTarget(const TargetArgs &target);
    void OpenTargetWhenFocused(const TargetArgs &target, QWidget *view);

    static QStringList UrlAncestors(const QString &text, const QUrl &current);
    static bool IsUrlToLoadInPlace(const QString &text);

    bool HandleMappedKey(QKeyEvent *ev);
    bool HandleTabKey();

    void Dismiss();
    void DismissFocusLeft();

    static bool SeekWords(const QString &line, QString *words = nullptr);

public slots:
    void OnReturned();
    void OnAborted();
    void ForeignCommandReceived();
    void SetString(QString str);
    void SetSuggest(QStringList list);
    void SuitableAction();
    void ReceiveCommand(QString cmd);

    void SelectNextSuggest();
    void SelectPrevSuggest();
    void SetFindCount(int active, int total);

private:
    void MakeOwnedWindow();
    void TakeWindowOwnerIfNeed();

    enum Mode {
        Command,
        Query,
        UrlEdit,
        Search,
        Target,
    } m_Mode;

    enum Placement {
        Center,
        TopRight,
        Anchored,
    };
    bool IsSeeking() const;
    bool TabSeeks() const;
    Placement CurrentPlacement() const;

    bool ListScrolls() const;
    int VisibleSuggestCount() const;
    int SelectableSuggestCount() const;
    void ScrollSuggestsBy(int rows);
    void KeepSelectionShown();
    int LastSuggestScroll() const;
    int m_SuggestScroll = 0;
    bool ScrollToPointer(const QPoint &pos);
    bool m_DraggingScroll = false;
    QRect SuggestRect(int index) const;

    void PreviewChosenTab();
    bool m_Previewing = false;
    bool m_SettingNoted = false;
    int SuggestAt(const QPoint &pos) const;

    void SeekFromField(const QString &words);

    static bool ShouldCloseOnFocusLeft(bool center, bool fieldHasFocus,
                                       bool popupIsUp, bool anyWindowActive);
    void OnFieldFocusOut();
    void CloseIfFocusLeft();
    bool m_FocusWentElsewhere = false;

    void InitializeDisplay(Mode mode, int tab = 0, const QString &text = QString());

    QPointer<QWidget> m_Anchor;
    void ReleaseAnchor(bool focusLeft = false);
    bool m_RejoinAfterHide = false;
    void SubmitAddress(const QString &text);
    bool IsOverField() const;
    bool LineBelow() const;
    int LineHeight() const;
    int ListTop() const;
    int ListBottom() const;
    void ReadForeignCommand(QLocalSocket *socket);

    void DispatchAction(const QString &action);

    struct SuggestRow {
        enum Source {
            Commands,
            Tabs,
            Trash,
            Settings,
            Web,
            SourceCount,
        } source;
        enum Kind {
            RunCommand,
            RunAction,
            EditBinding,
            FillField,
            OpenTab,
            SearchWeb,
            OpenAddress,
            TargetAction,
            TargetSearch,
            TargetCommand,
        } kind;
        QString title;
        QString hint;
        QString text;
        quint64 serial;
        QIcon icon;
        QString section;
        QString key;
        QStringList inputs;
    };
    typedef QVector<SuggestRow> SuggestRows;

    enum TargetPart { TargetLink, TargetImage, TargetSelection, TargetMedia, TargetElement, TargetNode,
                      TargetField };
    struct TargetTables {
        QString node;
        QString link;
        QString image;
        QString media;
        QString selection;
        QString field;
        QStringList engines;
        QStringList commands;
    };
    static TargetTables CurrentTargetTables(const TargetArgs &target);
    static SuggestRows TargetRows(const TargetArgs &target, const TargetTables &tables,
                                  const QString &typed);
    static QString TargetPartName(TargetPart part);
    static bool TargetHits(const QString &action);
    static bool IsSameTarget(const WebElement &held, const WebElement &found);
    static bool IsSameField(const WebElement &held, const WebElement &found);
    static QString TargetTag(const TargetArgs &target);
    static SharedView LiveTarget(const TargetArgs &target, TreeBank *bank);
    static bool IsLiveTarget(const View *held, const View *nodeView, const View *current,
                             quint64 heldLoad, quint64 loadNow);
    void RunTargetRow(const SuggestRow &row);
    void RunNodeRow(const SuggestRow &row, const TargetArgs &target, quint64 request);
    void TakeSelectionHtml(quint64 request, const QString &html);
    static ViewNode *HeldNode(quint64 serial);
    static QString NodeTag(const TargetArgs &target);
    TargetArgs m_Target;
    quint64 m_TargetRequest = 0;

    static SuggestRows ComposeRows(const QVector<SuggestRows> &sources, int capacity);

    int RowCapacity(int sources) const;

    SuggestRows CommandRows(const QString &typed, int limit, bool *exact = nullptr) const;
    static bool AnswersHere(const QString &action, bool list, bool table);
    static int HintWidth(int row, int title, int hint, int gap);
    static int InputsWidth(const QList<int> &widths, int shown, int gap, int more);
    SuggestRows TabRows(const QString &typed, int limit) const;
    SuggestRows TrashRows(const QString &typed, int limit) const;
    SuggestRows SettingRows(const QString &typed, int limit = 1000, bool unset = false) const;
    static QString SettingLine(const QString &key, const QString &value, bool unset);
    static SuggestRows ValueRows(const QString &rest, bool *matched);
    static QString SettingRefusal(const QString &key, const QString &text);
    static QString SettingNote(const QString &line);
    static QString BindingFind(const InputMapSchema::Binding &binding);
    SuggestRows WebRows(const QString &typed, const QUrl &here) const;
    QUrl CurrentAddress() const;

    static bool SourceWord(const QString &line, SuggestRow::Source *source, QString *rest,
                           bool *unset = nullptr);
    static bool SettingCommand(const QString &line, QString *rest, bool *unset);
    static QString SourceName(SuggestRow::Source source);
    static bool IsAssignment(const QString &rest, bool unset = false);

    bool ShowsSourceBar() const;
    QRect SourceBarRect(int index) const;
    int SourceBarAt(const QPoint &pos) const;
    void SelectTab(int index);
    void UpdatePlaceholder();
    int m_ActiveTab = 0;

    static QStringList m_History;
    QStringList m_WebSuggests;
    static void Remember(const QString &line);
    static void RememberAction(const QString &action);

    void SuggestFromSources(const QString &typed);
    void TakeRow(SuggestRow row);

    enum class RowAction {
        Take,
        CloseTab,
        DiscardTab,
        CopyAddress,
        CopyWords,
        ChangeAssignment,
        OpenInSettings,
        ResetSetting,
        OpenCard,
    };
    static QVector<RowAction> RowActions(const SuggestRow &row);
    static QString RowActionLabel(const SuggestRow &row, RowAction action);
    void RunRowAction(const SuggestRow &row, RowAction action);
    void OpenSettingsWhenItMayChange(const QString &fragment);
    void OpenRowActions(int index, const QPoint &globalPos);

    bool HasHeading(int index) const;
    static QString CommandLabel(const QString &name);
    QStringList InputsOf(const QString &name) const;

    void ReadBindings();
    QList<InputMapSchema::Binding> m_Bindings;

    TreeBank *m_TreeBank;

    static QLocalServer *m_LocalServer;
    QPointer<QLocalServer> m_Server;
    friend class tst_receiver;
    LineEdit *m_LineEdit;
    QString m_LineString;
    QStringList m_SuggestStrings;
    SuggestRows m_Rows;
    int m_CurrentSuggestIndex;

protected:
    void focusInEvent(QFocusEvent *ev) override;
    void focusOutEvent(QFocusEvent *ev) override;
    void timerEvent(QTimerEvent *ev) override;
    void paintEvent(QPaintEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;
    void showEvent(QShowEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void wheelEvent(QWheelEvent *ev) override;
    void contextMenuEvent(QContextMenuEvent *ev) override;
    bool eventFilter(QObject *watched, QEvent *ev) override;

signals:

    void TriggerElementAction(Page::CustomAction);

    void OpenNodeWithCommand(QString name);

    void OpenUrl(QUrl);
    void OpenUrl(QList<QUrl>);
    void OpenQueryUrl(QString);
    void OpenBookmarklet(const QString&);
    void SearchWith(QString, QString);

    void Download(QString, QString);
    void SeekText(const QString&, View::FindFlags);
    void KeyEvent(QString);

public slots:
    void DisplaySuggest(const QByteArray&);
signals:
    void SuggestRequest(const QUrl&);
    void AnchorReleased(QWidget *field, bool focusLeft);
};
#endif

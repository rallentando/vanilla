#ifndef CONST_HPP
#define CONST_HPP

#include "switch.hpp"

#include <QtCore>
#include <QSize>
#include <QFont>
#include <QFontInfo>
#include <QFontDatabase>

static const QString VANILLA_LOCAL_SERVER_NAME_PREFIX = QStringLiteral("vanilla_local_server_");

static const QString VANILLA_SHARED_MEMORY_KEY_PREFIX = QStringLiteral("vanilla_shared_memory_");

inline const QFont &DefaultFont(){
    static const QFont font =
        QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    return font;
}

static const QString MONOSPACE_FAMILIES =
    QStringLiteral("ui-monospace, \"Cascadia Mono\", Consolas, monospace");

inline QFont ScaledFont(double factor, int offset = 0){
    QFont font(DefaultFont());
    const qreal points = QFontInfo(font).pointSizeF();
    if(points > 0.0) font.setPointSizeF(points * factor + offset);
    return font;
}

static const int MAX_DRAGGING_PIXMAP_WIDTH  = 600;
static const int MAX_DRAGGING_PIXMAP_HEIGHT = 600;

static const int OPEN_LINK_WARNING_THRESHOLD = 10;

static const QSize DEFAULT_WINDOW_SIZE = QSize(1280, 720);

static const QString EMPTY_FRAME_HTML = QStringLiteral("<html><head></head><body></body></html>");

static const QUrl EMPTY_URL = QUrl();
static const QUrl BLANK_URL = QUrl(QStringLiteral("about:blank"));

static const QString DISABLE_FILENAME = "???";

static const QString NODE_DATETIME_FORMAT = QStringLiteral("yyyyMMddhhmmss");

inline QDateTime NodeDateTimeFromString(const QString &str){
    if(str.length() != 14) return QDateTime();
    for(int i = 0; i < 14; i++){
        if(str.at(i) < QLatin1Char('0') || str.at(i) > QLatin1Char('9')) return QDateTime();
    }
    const QStringView view(str);
    const QDate date(view.mid(0, 4).toInt(), view.mid(4, 2).toInt(), view.mid(6, 2).toInt());
    const QTime time(view.mid(8, 2).toInt(), view.mid(10, 2).toInt(), view.mid(12, 2).toInt());
    if(!date.isValid() || !time.isValid()) return QDateTime();
#if defined(Q_OS_LINUX)
    const QTimeZone local = QTimeZone::systemTimeZone();
    if(local.isValid()) return QDateTime(date, time, local);
#endif
    return QDateTime(date, time);
}

static const QString VANILLA_SCHEME = QStringLiteral("vanilla");

static const char EXTERNAL_COMMAND_PROPERTY[] = "vanilla_external_command";

inline const QFont &TreeBarTitleFont(){
    static const QFont font = ScaledFont(1.0, 1);
    return font;
}

static const int TREE_BAR_TAB_PADDING = 6;

static const int TREE_BAR_TAB_MINIMUM_HEIGHT = 28;

static const int TREE_BAR_TAB_MINIMUM_IMAGE_HEIGHT = 12;

static const int CHIP_CORNER_RADIUS = 3;
static const int MARKER_CORNER_RADIUS = 1;
static const int FIELD_CORNER_RADIUS = 4;
static const int FIELD_PADDING = 3;
static const int FIELD_PADDING_X = 8;
static const int PAGE_FIELD_CORNER_RADIUS = 6;
static const int DIALOG_CORNER_RADIUS = 6;
static const int PAGE_PILL_CORNER_RADIUS = 999;

static const int TOOL_BAR_PADDING = 6;
static const int TOOL_BAR_ICON_SPACING = 5;
static const int TOOL_BAR_ICON_SIZE = 19;
inline const QFont &ToolBarFieldFont(){
    static const QFont font = ScaledFont(1.0, 2);
    return font;
}
static const int TOOL_BAR_FIELD_HEIGHT = 26;

static const int EXTENSION_LIST_WIDTH = 430;
static const int EXTENSION_LIST_ROWS_MAX_HEIGHT = 420;
static const int EXTENSION_PANEL_PADDING = 12;
static const int EXTENSION_ROW_NAME_WIDTH = 205;
static const QSize EXTENSION_POPUP_SIZE = QSize(450, 520);
static const int POPUP_FIT_TIMES[] = { 250, 700, 1500, 3000 };
static const int POPUP_FIT_SETTLE_TIMES[] = { 100, 500 };
inline const QFont &ExtensionListTitleFont(){
    static const QFont font = ScaledFont(1.0, 2);
    return font;
}

static const int MENU_ITEM_EXTRA_HEIGHT = 6;
static const int MENU_MARGIN = 4;

static const int EDGE_WIDGET_SIZE = 10;
static const int TITLE_BAR_HEIGHT = 32;

inline const QFont &TitleBarTitleFont(){ return DefaultFont();}

static const int STATUS_TEXT_PADDING = 5;

inline const QFont &NotifierFont(){ return DefaultFont();}

static const int NOTIFIER_WIDTH_PERCENTAGE = 30;
static const int NOTIFIER_HEIGHT = 50;
static const int NOTIFIER_MINIMUM_WIDTH = 300;

static const int TRANSFER_ITEM_HEIGHT = 25;
static const int TRANSFER_PROGRESS_PERCENTAGE = 40;

inline const QFont &ReceiverFont(){ return DefaultFont();}

static const int RECEIVER_HEIGHT = 50;
static const int LINEEDIT_HEIGHT = 25;
static const int SUGGEST_HEIGHT = 25;
static const int SCROLL_AREA_WIDTH = 30;
static const int SCROLL_AREA_HEIGHT = 50;

static const int MINIMAP_WIDTH = 80;

inline const QFont &DialogTitleFont(){
    static const QFont font = ScaledFont(1.0, 2);
    return font;
}
inline const QFont &DialogTextFont(){ return DefaultFont();}

static const int MINIMUL_DIALOG_WIDTH = 400;
static const int MODELESS_DIALOG_WIDTH_DIVISOR = 4;
static const int AUTOCANCEL_DISTANCE = 10000;

static const QSize SAVING_THUMBNAIL_SIZE   = QSize(400, 300);
static const QSize RESIDENT_THUMBNAIL_SIZE = QSize(200, 150);
static const int THUMBNAIL_JPEG_QUALITY = 90;
static const QSize DEFAULT_THUMBNAIL_SIZE = QSize(200, 150);
static const QSize MINIMUM_THUMBNAIL_SIZE = QSize( 20,  15);
inline const QFont &ThumbnailTitleFont(){
    static const QFont font = ScaledFont(1.0, 1);
    return font;
}

inline const QFont &NodeTitleFont(){
    static const QFont font = ScaledFont(1.0, 3);
    return font;
}

static const int DISPLAY_PADDING_X = 25;
static const int DISPLAY_PADDING_Y = 15;

static const int GADGETS_SCROLL_BAR_MARGIN = 10;
static const int GADGETS_SCROLL_BAR_WIDTH = 15;
static const int GADGETS_SCROLL_CONTROLER_HEIGHT = 30;
static const bool GADGETS_SCROLL_BAR_DRAW_BORDER = true;

static const qreal HIDDEN_CONTENTS_LAYER = -10.0;
static const qreal VIEW_CONTENTS_LAYER = 0.0;
static const qreal COVERING_VIEW_CONTENTS_LAYER = 5.0;
static const qreal MAIN_CONTENTS_LAYER = 10.0;
static const qreal BUTTON_LAYER = 15.0;
static const qreal SPOT_LIGHT_LAYER = 20.0;
static const qreal DRAGGING_CONTENTS_LAYER = 30.0;
static const qreal BUTTON_ON_DRAGGING_LAYER = 35.0;
static const qreal SELECT_RECT_LAYER = 40.0;
static const qreal IN_PLACE_NOTIFIER_LAYER = 50.0;
static const qreal MULTIMEDIA_LAYER = 60.0;

static const int SCENE_WIDGET_LAYER          =  0;
static const int VIEW_WIDGET_LAYER           = 10;
static const int MINIMAP_WIDGET_LAYER        = 15;
static const int COVERING_SCENE_WIDGET_LAYER = 20;
static const int NOTIFIER_WIDGET_LAYER       = 30;
static const int RECEIVER_WIDGET_LAYER       = 40;

static const QString FOR_ACCESSKEY_CSS_SELECTOR =
    QStringLiteral("a,*[href],*[onclick],*[onmouseover],*[contenteditable=\"true\"],"
                   "*[role=\"button\"],*[role=\"link\"],*[role=\"menu\"],"
                   "button,select,label,legend,input,textarea,object,embed,frame,iframe,"
                   "*[role=\"checkbox\"],*[role=\"radio\"],*[role=\"tab\"]");
static const QString HAVE_SOURCE_CSS_SELECTOR = QStringLiteral("*[src]");
static const QString HAVE_REFERENCE_CSS_SELECTOR = QStringLiteral("*[href]");
static const QString REL_IS_NEXT_CSS_SELECTOR = QStringLiteral("*[rel=\"next\"]");
static const QString REL_IS_PREV_CSS_SELECTOR = QStringLiteral("*[rel=\"prev\"]");

static const int ACCESSKEY_GRID_UNIT_SIZE = 10;

static const QSize ACCESSKEY_CHAR_CHIP_SS_SIZE = QSize(16, 14);
inline const QFont &AccessKeyChipSSFont(){ return DefaultFont();}
static const QSize ACCESSKEY_CHAR_CHIP_S_SIZE = QSize(24, 22);
inline const QFont &AccessKeyChipSFont(){
    static const QFont font = ScaledFont(1.5);
    return font;
}

static const QSize ACCESSKEY_CHAR_CHIP_MS_SIZE = ACCESSKEY_CHAR_CHIP_S_SIZE;
inline const QFont &AccessKeyChipMSFont(){ return AccessKeyChipSFont();}
static const QSize ACCESSKEY_CHAR_CHIP_M_SIZE = QSize(36, 33);
inline const QFont &AccessKeyChipMFont(){
    static const QFont font = ScaledFont(2.2);
    return font;
}

static const QSize ACCESSKEY_CHAR_CHIP_LS_SIZE = ACCESSKEY_CHAR_CHIP_M_SIZE;
inline const QFont &AccessKeyChipLSFont(){ return AccessKeyChipMFont();}
static const QSize ACCESSKEY_CHAR_CHIP_L_SIZE = QSize(48, 44);
inline const QFont &AccessKeyChipLFont(){
    static const QFont font = ScaledFont(3.0);
    return font;
}

static const int ACCESSKEY_INFO_HEIGHT = 15;
static const int ACCESSKEY_INFO_MAX_WIDTH = 400;
inline const QFont &AccessKeyInfoFont(){ return DefaultFont();}

static const int GESTURE_TRIGGER_COUNT = 3;
static const int GESTURE_TRIGGER_LENGTH = 20;

static const int OWN_DROP_TO_NEW_WINDOW_MSEC = 1000;

static const int DEFAULT_LOCALVIEW_MAX_FILEIMAGE = 100;
static const int MAXIMUM_LOCALVIEW_MAX_FILEIMAGE = 512;

static const int MAX_SAME_ACTION_COUNT = 100;

#endif

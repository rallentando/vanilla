#ifndef CONST_HPP
#define CONST_HPP

#include "switch.hpp"

#include <QtCore>
#include <QSize>
#include <QFont>

static const QString VANILLA_LOCAL_SERVER_NAME_PREFIX = QStringLiteral("vanilla_local_server_");

static const QString VANILLA_SHARED_MEMORY_KEY_PREFIX = QStringLiteral("vanilla_shared_memory_");

static const char* VANILLA_REMOTE_DEBUGGING_PORT = "47875";

static const QString DEFAULT_FONT = QStringLiteral("Arial");

static const int MAX_DRAGGING_PIXMAP_WIDTH  = 600;
static const int MAX_DRAGGING_PIXMAP_HEIGHT = 600;

static const int OPEN_LINK_WARNING_THRESHOLD = 10;

static const QPoint DEFAULT_WINDOW_POSITION = QPoint(100, 100);
static const QSize DEFAULT_WINDOW_SIZE = QSize(640, 480);
static const QRect DEFAULT_WINDOW_RECT = QRect(DEFAULT_WINDOW_POSITION, DEFAULT_WINDOW_SIZE);

static const QString EMPTY_FRAME_HTML = QStringLiteral("<html><head></head><body></body></html>");

// file
static const QString DISABLE_FILENAME = "???";

// node
static const QString NODE_DATETIME_FORMAT = QStringLiteral("yyyyMMddhhmmss");

// user agent
static const QString DEFAULT_USER_AGENT_IE        = QStringLiteral("Mozilla/5.0 (%SYSTEM%; Trident/7.0; rv:11.0) like Gecko");
static const QString DEFAULT_USER_AGENT_FIREFOX   = QStringLiteral("Mozilla/5.0 (%SYSTEM%; rv:43.0) Gecko/20100101 Firefox/43.0");
static const QString DEFAULT_USER_AGENT_OPERA     = QStringLiteral("Opera/9.80 (%SYSTEM%) Presto/2.12.388 Version/12.17");
static const QString DEFAULT_USER_AGENT_OPR       = QStringLiteral("Mozilla/5.0 (%SYSTEM%) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/47.0.2526.73 Safari/537.36 OPR/34.0.2036.25");
static const QString DEFAULT_USER_AGENT_SAFARI    = QStringLiteral("Mozilla/5.0 (%SYSTEM%) AppleWebKit/534.57.2 (KHTML, like Gecko) Version/5.1.7 Safari/534.57.2");
static const QString DEFAULT_USER_AGENT_CHROME    = QStringLiteral("Mozilla/5.0 (%SYSTEM%) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/47.0.2526.106 Safari/537.36");
static const QString DEFAULT_USER_AGENT_SLEIPNIR  = QStringLiteral("Mozilla/5.0 (%SYSTEM%) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.86 Safari/537.36 Sleipnir/4.4.6");
static const QString DEFAULT_USER_AGENT_VIVALDI   = QStringLiteral("Mozilla/5.0 (%SYSTEM%) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/47.0.2564.48 Safari/537.36 Vivaldi/1.0.357.5");
static const QString DEFAULT_USER_AGENT_NETSCAPE  = QStringLiteral("Mozilla/5.0 (Windows; U; %SYSTEM%; en-US; rv:1.7.2) Gecko/20040804 Netscape/7.2 (ax)");
static const QString DEFAULT_USER_AGENT_SEAMONKEY = QStringLiteral("Mozilla/5.0 (Windows; U; %SYSTEM%; rv:10.0.1) Gecko/20100101 Firefox/10.0.1 SeaMonkey/2.7.1");
static const QString DEFAULT_USER_AGENT_GECKO     = QStringLiteral("Mozilla/5.0 (%SYSTEM%; rv:14.0) Gecko/20100101");
static const QString DEFAULT_USER_AGENT_ICAB      = QStringLiteral("Mozilla/5.0 (compatible; iCab 3.0.3; Macintosh; U; PPC Mac OS X)");
static const QString DEFAULT_USER_AGENT_CAMINO    = QStringLiteral("Mozilla/5.0 (Macintosh; U; PPC Mac OS X 10.4; en; rv:1.9.2.24) Gecko/20111114 Camino/2.1 (like Firefox/3.6.24)");

// titlebar
static const int EDGE_WIDGET_SIZE = 10;
static const int TITLE_BAR_HEIGHT = 21;

static const QFont TITLE_BAR_TITLE_FONT = QFont(DEFAULT_FONT, 10);

// notifier
static const QFont NOTIFIER_FONT = QFont(DEFAULT_FONT, 10);

static const int NOTIFIER_WIDTH_PERCENTAGE = 30;
static const int NOTIFIER_HEIGHT = 50;
static const int NOTIFIER_MINIMUM_WIDTH = 300;

static const int TRANSFER_ITEM_HEIGHT = 25;
static const int TRANSFER_PROGRESS_PERCENTAGE = 40;

// receiver
static const QFont RECEIVER_FONT = QFont(DEFAULT_FONT, 10);

static const int RECEIVER_HEIGHT = 50;
static const int LINEEDIT_HEIGHT = 25;
static const int SUGGEST_HEIGHT = 25;
static const int SCROLL_AREA_WIDTH = 30;
static const int SCROLL_AREA_HEIGHT = 50;

// dialog
static const QString DIALOG_TITLE_STYLE_SHEET = QStringLiteral("QLabel { font-family: Arial; color: white; font-size: 15px;}");
static const QString DIALOG_TEXT_STYLE_SHEET  = QStringLiteral("QLabel { font-family: Arial; color: white; font-size: 12px;}");
static const int MINIMUL_DIALOG_WIDTH = 400;
static const int AUTOCANCEL_DISTANCE = 10000;

// thumbnail
//                                      keep aspect !!
//static const QSize SAVING_THUMBNAIL_SIZE = QSize(1000, 750);
static const QSize SAVING_THUMBNAIL_SIZE  = QSize(200, 150);
static const QSize DEFAULT_THUMBNAIL_SIZE = QSize(200, 150);
static const QSize MINIMUM_THUMBNAIL_SIZE = QSize( 20,  15);

// gadgets
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
static const qreal SELECT_RECT_LAYER = 40.0;
static const qreal IN_PLACE_NOTIFIER_LAYER = 50.0;
static const qreal MULTIMEDIA_LAYER = 60.0;

static const QString FOR_ACCESSKEY_CSS_SELECTOR =
    QStringLiteral("*[href],*[onclick],*[onmouseover],*[contenteditable=\"true\"],"
                 VV"*[role=\"button\"],*[role=\"link\"],*[role=\"menu\"],"
                 VV"button,select,label,legend,input,textarea,object,embed,frame,iframe,"
                 VV"*[role=\"checkbox\"],*[role=\"radio\"],*[role=\"tab\"]");
static const QString HAVE_SOURCE_CSS_SELECTOR = QStringLiteral("*[src]");
static const QString HAVE_REFERENCE_CSS_SELECTOR = QStringLiteral("*[href]");
static const QString REL_IS_NEXT_CSS_SELECTOR = QStringLiteral("*[rel=\"next\"]");
static const QString REL_IS_PREV_CSS_SELECTOR = QStringLiteral("*[rel=\"prev\"]");

static const int ACCESSKEY_GRID_UNIT_SIZE = 10;

static const QSize ACCESSKEY_CHAR_CHIP_SS_SIZE = QSize(16, 14);
static const QFont ACCESSKEY_CHAR_CHIP_SS_FONT = QFont(DEFAULT_FONT, 10);
static const QSize ACCESSKEY_CHAR_CHIP_S_SIZE = QSize(24, 22);
static const QFont ACCESSKEY_CHAR_CHIP_S_FONT = QFont(DEFAULT_FONT, 15);

static const QSize ACCESSKEY_CHAR_CHIP_MS_SIZE = ACCESSKEY_CHAR_CHIP_S_SIZE;
static const QFont ACCESSKEY_CHAR_CHIP_MS_FONT = ACCESSKEY_CHAR_CHIP_S_FONT;
static const QSize ACCESSKEY_CHAR_CHIP_M_SIZE = QSize(36, 33);
static const QFont ACCESSKEY_CHAR_CHIP_M_FONT = QFont(DEFAULT_FONT, 22);

static const QSize ACCESSKEY_CHAR_CHIP_LS_SIZE = ACCESSKEY_CHAR_CHIP_M_SIZE;
static const QFont ACCESSKEY_CHAR_CHIP_LS_FONT = ACCESSKEY_CHAR_CHIP_M_FONT;
static const QSize ACCESSKEY_CHAR_CHIP_L_SIZE = QSize(48, 44);
static const QFont ACCESSKEY_CHAR_CHIP_L_FONT = QFont(DEFAULT_FONT, 30);

static const int ACCESSKEY_INFO_HEIGHT = 15;
static const int ACCESSKEY_INFO_MAX_WIDTH = 400;
static const QFont ACCESSKEY_INFO_FONT = QFont(DEFAULT_FONT, 10);

static const int GESTURE_TRIGGER_COUNT = 3;
static const int GESTURE_TRIGGER_LENGTH = 20;

static const int DEFAULT_LOCALVIEW_MAX_FILEIMAGE = 100;

static const int MAX_SAME_ACTION_COUNT = 100;

#endif

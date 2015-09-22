
;; this script generate WebView, GraphicsWebView, QuickWebView,
;; WebEngineView, QuickWebEngineView, WebPage, WebEnginePage.

(use file.util)
(use text.diff)

;; WebView :
;;   //[[WV]]
;;   //[[/WV]]
;; GraphicsWebView :
;;   //[[GWV]]
;;   //[[/GWV]]
;; WebEngineView :
;;   //[[WEV]]
;;   //[[/WEV]]

;; QuickWebView :
;;   //[[QWV]]
;;   //[[/QWV]]
;; QuickWebEngineView :
;;   //[[QWEV]]
;;   //[[/QWEV]]

;; WebView or WebPage
(define wv-converter
  '("WebViewBase" "WebView"
    "WEBVIEWBASE" "WEBVIEW"
    "webviewbase" "webview"

    "WebPageBase" "WebPage"
    "WEBPAGEBASE" "WEBPAGE"
    "webpagebase" "webpage"

    "WebFrameBase"  "WebFrame"
    "WebHistoryBase" "WebHistory"
    "WebElementBase" "WebElement"
    "WebSettingsBase" "WebSettings"

    "(?:\r\n|\n\r|\n|\r) *//\\[\\[!WV\\]\\](?:\n|[^\n])+?(?:\r\n|\n\r|\n|\r) *//\\[\\[/!WV\\]\\]" ""
    "(?:\r\n|\n\r|\n|\r) *//\\[\\[GWV\\]\\](?:\n|[^\n])+?(?:\r\n|\n\r|\n|\r) *//\\[\\[/GWV\\]\\]" ""
    "(?:\r\n|\n\r|\n|\r) *//\\[\\[WEV\\]\\](?:\n|[^\n])+?(?:\r\n|\n\r|\n|\r) *//\\[\\[/WEV\\]\\]" ""
    "(?:\r\n|\n\r|\n|\r) *//\\[\\[/?!?[A-Z]*\\]\\]" ""
    "QResizeEventBase" "QResizeEvent"
    "QContextMenuEventBase" "QContextMenuEvent"
    "QMouseEventBase" "QMouseEvent"
    "QDragEnterEventBase" "QDragEnterEvent"
    "QDragMoveEventBase" "QDragMoveEvent"
    "QDropEventBase" "QDropEvent"
    "QDragLeaveEventBase" "QDragLeaveEvent"
    "QWheelEventBase" "QWheelEvent"))

;; GraphicsWebView or WebPage
(define gwv-converter
  '("WebViewBase" "GraphicsWebView"
    "WEBVIEWBASE" "GRAPHICSWEBVIEW"
    "webviewbase" "graphicswebview"

    "WebPageBase" "WebPage"
    "WEBPAGEBASE" "WEBPAGE"
    "webpagebase" "webpage"

    "WebFrameBase" "WebFrame"
    "WebHistoryBase" "WebHistory"
    "WebElementBase" "WebElement"
    "WebSettingsBase" "WebSettings"

    "(?:\r\n|\n\r|\n|\r) *//\\[\\[!GWV\\]\\](?:\n|[^\n])+?(?:\r\n|\n\r|\n|\r) *//\\[\\[/!GWV\\]\\]" ""
    "(?:\r\n|\n\r|\n|\r) *//\\[\\[WV\\]\\](?:\n|[^\n])+?(?:\r\n|\n\r|\n|\r) *//\\[\\[/WV\\]\\]" ""
    "(?:\r\n|\n\r|\n|\r) *//\\[\\[WEV\\]\\](?:\n|[^\n])+?(?:\r\n|\n\r|\n|\r) *//\\[\\[/WEV\\]\\]" ""
    "(?:\r\n|\n\r|\n|\r) *//\\[\\[/?!?[A-Z]*\\]\\]" ""
    "QResizeEventBase" "QGraphicsSceneResizeEvent"
    "QContextMenuEventBase" "QGraphicsSceneContextMenuEvent"
    "QHoverEventBase" "QGraphicsSceneHoverEvent"
    "QMouseEventBase" "QGraphicsSceneMouseEvent"
    "QDragEnterEventBase" "QGraphicsSceneDragDropEvent"
    "QDragMoveEventBase" "QGraphicsSceneDragDropEvent"
    "QDropEventBase" "QGraphicsSceneDragDropEvent"
    "QDragLeaveEventBase" "QGraphicsSceneDragDropEvent"
    "QWheelEventBase" "QGraphicsSceneWheelEvent"))

;; WebEngineView or WebEnginePage
(define wev-converter
  '("WebViewBase" "WebEngineView"
    "WEBVIEWBASE" "WEBENGINEVIEW"
    "webviewbase" "webengineview"

    "WebPageBase" "WebEnginePage"
    "WEBPAGEBASE" "WEBENGINEPAGE"
    "webpagebase" "webenginepage"

    "WebFrameBase" "WebEngineFrame"
    "WebHistoryBase" "WebEngineHistory"
    "WebElementBase" "WebEngineElement"
    "WebSettingsBase" "WebEngineSettings"

    "(?:\r\n|\n\r|\n|\r) *//\\[\\[!WEV\\]\\](?:\n|[^\n])+?(?:\r\n|\n\r|\n|\r) *//\\[\\[/!WEV\\]\\]" ""
    "(?:\r\n|\n\r|\n|\r) *//\\[\\[GWV\\]\\](?:\n|[^\n])+?(?:\r\n|\n\r|\n|\r) *//\\[\\[/GWV\\]\\]" ""
    "(?:\r\n|\n\r|\n|\r) *//\\[\\[WV\\]\\](?:\n|[^\n])+?(?:\r\n|\n\r|\n|\r) *//\\[\\[/WV\\]\\]" ""
    "(?:\r\n|\n\r|\n|\r) *//\\[\\[/?!?[A-Z]*\\]\\]" ""
    "QResizeEventBase" "QResizeEvent"
    "QContextMenuEventBase" "QContextMenuEvent"
    "QMouseEventBase" "QMouseEvent"
    "QDragEnterEventBase" "QDragEnterEvent"
    "QDragMoveEventBase" "QDragMoveEvent"
    "QDropEventBase" "QDropEvent"
    "QDragLeaveEventBase" "QDragLeaveEvent"
    "QWheelEventBase" "QWheelEvent"))

;; QuickWebView or WebPage
(define qwv-converter
  '("QuickWebViewBase" "QuickWebView"
    "QUICKWEBVIEWBASE" "QUICKWEBVIEW"
    "quickwebviewbase" "quickwebview"
    "QmlWebViewBase" "QmlWebView"

    "WebPageBase" "WebPage"
    "WEBPAGEBASE" "WEBPAGE"
    "webpagebase" "webpage"

    "WebFrameBase"  "WebFrame"
    "WebHistoryBase" "WebHistory"
    "WebElementBase" "WebElement"
    "WebSettingsBase" "WebSettings"

    "(?:\r\n|\n\r|\n|\r) *//\\[\\[!QWV\\]\\](?:\n|[^\n])+?(?:\r\n|\n\r|\n|\r) *//\\[\\[/!QWV\\]\\]" ""
    "(?:\r\n|\n\r|\n|\r) *//\\[\\[QWEV\\]\\](?:\n|[^\n])+?(?:\r\n|\n\r|\n|\r) *//\\[\\[/QWEV\\]\\]" ""
    "(?:\r\n|\n\r|\n|\r) *//\\[\\[/?!?[A-Z]*\\]\\]" ""))

;; QuicWebView(qml) or WebPage
(define qwvq-converter ;; for qml
  '("WebViewBase" "WebView"
    "WEBVIEWBASE" "WEBVIEW"
    "webviewbase" "webview"
    "webViewBase" "webView"

    "WebPageBase" "WebPage"
    "WEBPAGEBASE" "WEBPAGE"
    "webpagebase" "webpage"

    "WebFrameBase"  "WebFrame"
    "WebHistoryBase" "WebHistory"
    "WebElementBase" "WebElement"
    "WebSettingsBase" "WebSettings"

    "(?:\r\n|\n\r|\n|\r) *//\\[\\[!QWV\\]\\](?:\n|[^\n])+?(?:\r\n|\n\r|\n|\r) *//\\[\\[/!QWV\\]\\]" ""
    "(?:\r\n|\n\r|\n|\r) *//\\[\\[QWEV\\]\\](?:\n|[^\n])+?(?:\r\n|\n\r|\n|\r) *//\\[\\[/QWEV\\]\\]" ""
    "(?:\r\n|\n\r|\n|\r) *//\\[\\[/?!?[A-Z]*\\]\\]" ""))

;; QuickWebEngineView or WebEnginePage
(define qwev-converter
  '("QuickWebViewBase" "QuickWebEngineView"
    "QUICKWEBVIEWBASE" "QUICKWEBENGINEVIEW"
    "quickwebviewbase" "quickwebengineview"
    "QmlWebViewBase" "QmlWebEngineView"

    "WebPageBase" "WebEnginePage"
    "WEBPAGEBASE" "WEBENGINEPAGE"
    "webpagebase" "webenginepage"

    "WebFrameBase"  "WebEngineFrame"
    "WebHistoryBase" "WebEngineHistory"
    "WebElementBase" "WebEngineElement"
    "WebSettingsBase" "WebEngineSettings"

    "(?:\r\n|\n\r|\n|\r) *//\\[\\[!QWEV\\]\\](?:\n|[^\n])+?(?:\r\n|\n\r|\n|\r) *//\\[\\[/!QWEV\\]\\]" ""
    "(?:\r\n|\n\r|\n|\r) *//\\[\\[QWV\\]\\](?:\n|[^\n])+?(?:\r\n|\n\r|\n|\r) *//\\[\\[/QWV\\]\\]" ""
    "(?:\r\n|\n\r|\n|\r) *//\\[\\[/?!?[A-Z]*\\]\\]" ""))

;; QuickWebEngineView(qml) or WebEnginePage
(define qwevq-converter ;; for qml
  '("WebViewBase" "WebEngineView"
    "WEBVIEWBASE" "WEBENGINEVIEW"
    "webviewbase" "webengineview"
    "webViewBase" "webEngineView"

    "WebPageBase" "WebEnginePage"
    "WEBPAGEBASE" "WEBENGINEPAGE"
    "webpagebase" "webenginepage"

    "WebFrameBase"  "WebEngineFrame"
    "WebHistoryBase" "WebEngineHistory"
    "WebElementBase" "WebEngineElement"
    "WebSettingsBase" "WebEngineSettings"

    "experimental\.evaluateJavaScript" "runJavaScript"

    "(?:\r\n|\n\r|\n|\r) *//\\[\\[!QWEV\\]\\](?:\n|[^\n])+?(?:\r\n|\n\r|\n|\r) *//\\[\\[/!QWEV\\]\\]" ""
    "(?:\r\n|\n\r|\n|\r) *//\\[\\[QWV\\]\\](?:\n|[^\n])+?(?:\r\n|\n\r|\n|\r) *//\\[\\[/QWV\\]\\]" ""
    "(?:\r\n|\n\r|\n|\r) *//\\[\\[/?!?[A-Z]*\\]\\]" ""))

(define (convert source target converter)
  (call-with-output-file target
    (let1 result (apply regexp-replace-all* source converter)
      (pa$ display result))))

(define (convert-if-need base alist force-convert)
  (let1 oldest (^() (car (sort (cons base (map car alist)) file-mtime<?)))
    (when (or force-convert (not (equal? base (oldest))))
      (for-each (pa$ apply convert (file->string base)) alist))))

(define (main . args)
  (begin
    (make-directory* "./gen")
    (let1 force-convert? (equal? '("." "..") (directory-list "./gen"))

      (convert-if-need "./base/webviewbase.hpp"
                       `(("./gen/webview.hpp"            ,wv-converter)
                         ("./gen/graphicswebview.hpp"    ,gwv-converter)
                         ("./gen/webengineview.hpp"      ,wev-converter))
                       force-convert?)

      (convert-if-need "./base/webviewbase.cpp"
                       `(("./gen/webview.cpp"            ,wv-converter)
                         ("./gen/graphicswebview.cpp"    ,gwv-converter)
                         ("./gen/webengineview.cpp"      ,wev-converter))
                       force-convert?)

      (convert-if-need "./base/webpagebase.hpp"
                       `(("./gen/webpage.hpp"            ,wv-converter)
                         ("./gen/webenginepage.hpp"      ,wev-converter))
                       force-convert?)

      (convert-if-need "./base/webpagebase.cpp"
                       `(("./gen/webpage.cpp"            ,wv-converter)
                         ("./gen/webenginepage.cpp"      ,wev-converter))
                       force-convert?)

      (convert-if-need "./base/quickwebviewbase.hpp"
                       `(("./gen/quickwebview.hpp"       ,qwv-converter)
                         ("./gen/quickwebengineview.hpp" ,qwev-converter))
                       force-convert?)

      (convert-if-need "./base/quickwebviewbase.cpp"
                       `(("./gen/quickwebview.cpp"       ,qwv-converter)
                         ("./gen/quickwebengineview.cpp" ,qwev-converter))
                       force-convert?)

      (convert-if-need "./base/quickwebviewbase.qml"
                       `(("./gen/quickwebview.qml"       ,qwvq-converter)
                         ("./gen/quickwebengineview.qml" ,qwevq-converter))
                       force-convert?)

      (print "success"))))


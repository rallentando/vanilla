// versionless imports, as in quickwebengineview6.qml. the file used to ask
// for 'QtQuick.Controls 1.4' as well, which does not exist in Qt 6 -- and
// for 'QtQuick.Dialogs 1.2' and 'QtQuick.Window', neither of which it uses.
import QtQuick
import QtWebView

WebView {
    signal viewChanged()
    signal scrollChanged(point pos)
    signal callBackResult(int id, variant result)

    // the backend has no zoom API; css zoom on the body stands in.
    // logical units: WebView2 applies the display scale itself (C-3c).
    property real cssZoom: 1.0

    /*
      Every load ends here, and every one of the four statuses is a name the
      backend actually has.

      The failed branch used to ask for 'LoadFaliedStatus', which is not one
      of them: QML answers undefined, the comparison is never true, and a
      failed load reported nothing at all -- 'View::m_IsLoading' stayed true
      for the rest of the view's life. Stopping was never handled either.
      Both are ends of a load, and the history list needs to hear about the
      ends, so both say so now. ('tst_nativehistory' checks these names
      against the backend's own type description. C-3c)

      They do not say the same thing, though. A load which was *stopped* did
      not fail: the user pressed Stop, or the navigation was taken over --
      and 'loadFinished(false)' is the one word this view has for "failed",
      which the status bar then repeats. So stopping calls 'loadStopped'
      instead: the same ending, with nothing said about it. The other quick
      view has the same rule the other way round -- it only ever looks at
      'LoadFailedStatus' ('quickwebengineview6.qml') -- and the widget views
      reach it through 'LoadEnding' (D-280).
     */
    onLoadingChanged: {
        var status = loadRequest.status

        if(status == WebView.LoadStartedStatus){
            viewInterface.loadStarted()
        }
        if(status == WebView.LoadSucceededStatus){
            // css zoom is a property of the document, and the document is
            // new; put it back before the scroll restore that follows.
            restoreZoom()
            viewInterface.loadFinished(true)
        }
        if(status == WebView.LoadFailedStatus){
            viewInterface.loadFinished(false)
        }
        if(status == WebView.LoadStoppedStatus){
            viewInterface.loadStopped()
        }
    }

    onLoadProgressChanged: {
        viewInterface.loadProgress(loadProgress)
    }

    onTitleChanged: {
        viewInterface.titleChanged(title)
    }

    onUrlChanged: {
        viewInterface.urlChanged(url)
    }

    // rewind and fast forward are gone from here on purpose. they walked
    // the backend's history one step at a time, waiting for each load, and
    // so loaded every page in between to reach the end. the view's own list
    // knows where the end is and gets there in one load (C-3c).

    function setScroll(pos){
        runJavaScript
        (viewInterface.setScrollRatioPointJsCode(pos),
         function(_){
             emitScrollChanged()
         })
    }

    function saveScroll(){
        runJavaScript
        (viewInterface.getScrollValuePointJsCode(),
         function(result){
             viewInterface.saveScrollToNode(Qt.point(result[0], result[1]))
         })
    }

    function restoreScroll(){
        var pos = viewInterface.restoreScrollFromNode()
        runJavaScript
        (viewInterface.setScrollValuePointJsCode(pos))
    }

    function setZoom(zoom){
        cssZoom = zoom
        runJavaScript("document.body.style.zoom = " + zoom + ";")
    }

    function saveZoom(){
        viewInterface.saveZoomToNode(cssZoom)
    }

    function restoreZoom(){
        var zoom = viewInterface.restoreZoomFromNode()
        if(zoom > 0 && zoom != cssZoom) setZoom(zoom)
        else if(cssZoom != 1.0) setZoom(cssZoom)
    }

    function evaluateJavaScript(id, code){
        runJavaScript
        (code,
         function(result){
             callBackResult(id, result)
         })
    }

    function emitScrollChanged(){
        runJavaScript
        (viewInterface.getScrollRatioPointJsCode(),
         function(pointf){
             scrollChanged(Qt.point(pointf[0], pointf[1]))
         })
    }
}

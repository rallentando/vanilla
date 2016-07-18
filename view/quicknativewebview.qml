import QtQuick 2.5
import QtQuick.Dialogs 1.2
import QtQuick.Controls 1.4
import QtQuick.Window 2.2
import QtWebView 1.1

WebView {
    signal viewChanged()
    signal scrollChanged(point pos)
    signal callBackResult(int id, variant result)

    onLoadingChanged: {
        var status = loadRequest.status

        if(status == WebView.LoadStartedStatus){
            viewInterface.loadStarted()
        }
        if(status == WebView.LoadSucceededStatus){
            viewInterface.loadFinished(true)
        }
        if(status == WebView.LoadFaliedStatus){
            viewInterface.loadFinished(false)
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

    function saveZoom(){
        // not yet implemented.
        //viewInterface.saveZoomToNode(zoomFactor)
    }

    function restoreZoom(){
        // not yet implemented.
        //zoomFactor = viewInterface.restoreZoomFromNode()
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

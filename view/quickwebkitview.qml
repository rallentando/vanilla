import QtQuick 2.5
import QtQuick.Dialogs 1.2
import QtQuick.Window 2.2
import QtWebKit 3.0
import QtWebKit.experimental 1.0

WebView {
    id: webKitView
    signal viewChanged()
    signal scrollChanged(point pos)
    signal statusBarMessage(string message)
    signal titleChanged_(string title)
    signal linkHovered_(string arg1, string arg2, string arg3)
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

    onLinkHovered: {
        linkHovered_(hoveredUrl.toString(), hoveredTitle, '')
    }

    onTitleChanged: {
        titleChanged_(title)
        viewInterface.changeNodeTitle(title)
    }

    onUrlChanged: {
        viewInterface.changeNodeUrl(url)
    }

    function setScroll(pos){
        experimental.evaluateJavaScript
        (viewInterface.setScrollRatioPointJsCode(pos),
         function(_){
             emitScrollChangedIfNeed()
         })
    }

    function saveScroll(){
        experimental.evaluateJavaScript
        (viewInterface.getScrollValuePointJsCode(),
         function(result){
             viewInterface.saveScrollToNode(Qt.point(result[0], result[1]))
         })
    }

    function restoreScroll(){
        var pos = viewInterface.restoreScrollFromNode()
        experimental.evaluateJavaScript
        (viewInterface.setScrollValuePointJsCode(pos))
    }

    function saveZoom(){
        // not yet implemented.
    }

    function restoreZoom(){
        // not yet implemented.
    }

    function evaluateJavaScript(id, code){
        experimental.evaluateJavaScript
        (code,
         function(result){
             callBackResult(id, result)
         })
    }

    function emitScrollChangedIfNeed(){
        experimental.evaluateJavaScript
        (viewInterface.getScrollValuePointJsCode(),
         function(result){
             if(Qt.point(result[0], result[1]) ==
                viewInterface.restoreScrollFromNode())
                 return

             experimental.evaluateJavaScript
             (viewInterface.getScrollRatioPointJsCode(),
              function(pointf){
                  scrollChanged(Qt.point(pointf[0], pointf[1]))
              })
         })
    }

    function setFocusToElement(xpath){
        experimental.evaluateJavaScript
        (viewInterface.setFocusToElementJsCode(xpath))
    }

    function fireClickEvent(xpath, pos){
        experimental.evaluateJavaScript
        (viewInterface.fireClickEventJsCode(xpath, pos/devicePixelRatio))
    }

    function setTextValue(xpath, text){
        experimental.evaluateJavaScript
        (viewInterface.setTextValueJsCode(xpath, text))
    }

    function adjustContents(){
        experimental.preferredMinimumContentsWidth = parent.width
    }

    function seekText(str, opt){
        var option = 0
        if(opt & viewInterface.findBackwardIntValue())
            option |= WebViewExperimental.FindBackward
        if(opt & viewInterface.caseSensitivelyIntValue())
            option |= WebViewExperimental.FindCaseSensitively
        if(opt & viewInterface.wrapsAroundDocumentIntValue())
            option |= WebViewExperimental.FindWrapsAroundDocument
        if(opt & viewInterface.highlightAllOccurrencesIntValue())
            option |= WebViewExperimental.FindHighlightAllOccurrences

        experimental.findText(str, option)
        emitScrollChangedIfNeed()
    }

    function setUserAgent(agent){
        experimental.userAgent = agent
    }

    function setDefaultTextEncoding(encoding){
        // not yet implemented.
    }

    function setPreference(item, value){
        if     (item == "AutoLoadImages")                    experimental.preferences.autoLoadImages = value
        else if(item == "CaretBrowsingEnabled")              experimental.preferences.caretBrowsingEnabled = value
        else if(item == "DeveloperExtrasEnabled")            experimental.preferences.developerExtrasEnabled = value
        else if(item == "DnsPrefetchEnabled")                experimental.preferences.dnsPrefetchEnabled = value
        else if(item == "FrameFlatteningEnabled")            experimental.preferences.frameFlatteningEnabled = value
        else if(item == "JavascriptEnabled")                 experimental.preferences.javascriptEnabled = value
        else if(item == "LocalStorageEnabled")               experimental.preferences.localStorageEnabled = value
        else if(item == "NotificationsEnabled")              experimental.preferences.notificationsEnabled = value
        else if(item == "OfflineWebApplicationCacheEnabled") experimental.preferences.offlineWebApplicationCacheEnabled = value
        else if(item == "PluginsEnabled")                    experimental.preferences.pluginsEnabled = value
        else if(item == "PrivateBrowsingEnabled")            experimental.preferences.privateBrowsingEnabled = value
        else if(item == "XSSAuditingEnabled")                experimental.preferences.xssAuditingEnabled = value
        else if(item == "WebAudioEnabled")                   experimental.preferences.webAudioEnabled = value
        else if(item == "WebGLEnabled")                      experimental.preferences.webGLEnabled = value

        else if(item == "LocalContentCanAccessFileUrls")     experimental.preferences.fileAccessFromFileURLsAllowed = value
        else if(item == "LocalContentCanAccessRemoteUrls")   experimental.preferences.universalAccessFromFileURLsAllowed = value
    }

    function setFontFamily(item, value){
        if     (item == "StandardFont")  experimental.preferences.standardFontFamily = value
        else if(item == "FixedFont")     experimental.preferences.fixedFontFamily = value
        else if(item == "SerifFont")     experimental.preferences.serifFontFamily = value
        else if(item == "SansSerifFont") experimental.preferences.sansSerifFontFamily = value
        else if(item == "CursiveFont")   experimental.preferences.cursiveFontFamily = value
        else if(item == "FantasyFont")   experimental.preferences.fantasyFontFamily = value
    }

    function setFontSize(item, value){
        if     (item == "MinimumFontSize")      experimental.preferences.minimumFontSize = value
        else if(item == "DefaultFontSize")      experimental.preferences.defaultFontSize = value
        else if(item == "DefaultFixedFontSize") experimental.preferences.defaultFixedFontSize = value
    }

    function inspect(){
        inspector.visible = true
    }

    Window {
        id: inspector
        visible: false
        WebView {
            anchors.fill: parent
            url: webKitView.experimental.remoteInspectorUrl
        }
    }

    experimental {

        certificateVerificationDialog : Item {
            Component.onCompleted: {
                model.accept()
            }
        }

        alertDialog : MessageDialog {
            id: messageDialog
            title: qsTr("Alert")

            onAccepted: {
                console.log("Message: " + messageDialog.text)
                visible = false
            }
            onRejected: {
                console.log("Canceled")
                visible = false
            }
            Component.onCompleted: {
                visible = true
            }
        }

        filePicker : FileDialog {
            id: fileDialog
            title: qsTr("Please choose a file or files")

            onAccepted: {
                console.log("You chose: " + fileDialog.fileUrls)
                visible = false
            }
            onRejected: {
                console.log("Canceled")
                visible = false
            }
            Component.onCompleted: {
                visible = true
            }
        }

        colorChooser : ColorDialog {
            id: colorDialog
            title: qsTr("Please choose a color")

            onAccepted: {
                console.log("You chose: " + colorDialog.color)
                visible = false
            }
            onRejected: {
                console.log("Canceled")
                visible = false
            }
            Component.onCompleted: {
                visible = true
            }
        }

        preferences {
            fullScreenEnabled : true
            navigatorQtObjectEnabled : true
        }
    }
}

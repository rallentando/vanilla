import QtQuick 2.5
import QtQuick.Dialogs 1.2
import QtQuick.Controls 1.4
import QtQuick.Window 2.2
import QtWebEngine 1.2
import QtWebEngine.experimental 1.0

Rectangle {
    WebEngineView {
        id: webEngineView
        signal viewChanged()
        signal scrollChanged(point pos)
        signal statusBarMessage(string message)
        signal titleChanged_(string title)
        signal linkHovered_(string arg1, string arg2, string arg3)
        signal callBackResult(int id, variant result)
        property real devicePixelRatio : 1.0

        anchors.fill: parent

        onLoadingChanged: {
            var status = loadRequest.status

            if(status == WebEngineView.LoadStartedStatus){
                viewInterface.onLoadStarted()
            }
            if(status == WebEngineView.LoadSucceededStatus){
                viewInterface.onLoadFinished(true)
            }
            if(status == WebEngineView.LoadFaliedStatus){
                viewInterface.onLoadFinished(false)
            }
        }

        onLoadProgressChanged: {
            viewInterface.onLoadProgress(loadProgress)
        }

        onLinkHovered: {
            linkHovered_(hoveredUrl.toString(), '', '')
        }

        onTitleChanged: {
            titleChanged_(title)
            viewInterface.changeNodeTitle(title)
        }

        onUrlChanged: {
            viewInterface.changeNodeUrl(url)
        }

        onFullScreenRequested: {
            viewInterface.setFullScreen(request.toggleOn)
            request.accept()
        }

        onDevicePixelRatioChanged: {
            experimental.viewport.devicePixelRatio = devicePixelRatio
        }

        onJavaScriptConsoleMessage: {
            if(level === WebEngineView.InfoMessageLevel)
                viewInterface.handleJavascriptConsoleMessage(message)
        }

        function setScroll(pos){
            runJavaScript
            (viewInterface.setScrollRatioPointJsCode(pos),
             function(_){
                 emitScrollChangedIfNeed()
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
            viewInterface.saveZoomToNode(devicePixelRatio)
        }

        function restoreZoom(){
            devicePixelRatio = viewInterface.restoreZoomFromNode()
        }

        function evaluateJavaScript(id, code){
            runJavaScript
            (code,
             function(result){
                 callBackResult(id, result)
             })
        }

        function emitScrollChangedIfNeed(){
            runJavaScript
            (viewInterface.getScrollValuePointJsCode(),
             function(result){
                 if(Qt.point(result[0], result[1]) ==
                    viewInterface.restoreScrollFromNode())
                     return

                 runJavaScript
                 (viewInterface.getScrollRatioPointJsCode(),
                  function(pointf){
                      scrollChanged(Qt.point(pointf[0], pointf[1]))
                  })
             })
        }

        function setFocusToElement(xpath){
            runJavaScript
            (viewInterface.setFocusToElementJsCode(xpath))
        }

        function fireClickEvent(xpath, pos){
            runJavaScript
            (viewInterface.fireClickEventJsCode(xpath, pos/devicePixelRatio))
        }

        function setTextValue(xpath, text){
            runJavaScript
            (viewInterface.setTextValueJsCode(xpath, text))
        }

        function adjustContents(){
            //experimental.preferredMinimumContentsWidth = parent.width
        }

        function seekText(str, opt){
            var option = 0
            if(opt & viewInterface.findBackwardIntValue())
                option |= FindBackward
            if(opt & viewInterface.caseSensitivelyIntValue())
                option |= FindCaseSensitively

            findText(str, option)
            emitScrollChangedIfNeed()
        }

        function setUserAgent(agent){
            //experimental.userAgent = agent
        }

        function setDefaultTextEncoding(encoding){
            settings.defaultTextEncoding = encoding
        }

        function setPreference(item, value){
            if     (item == "AutoLoadImages")                  settings.autoLoadImages = value
            else if(item == "JavascriptCanAccessClipboard")    settings.javascriptCanAccessClipboard = value
            else if(item == "JavascriptCanOpenWindows")        settings.javascriptCanOpenWindows = value
            else if(item == "JavascriptEnabled")               settings.javascriptEnabled = value
            else if(item == "LinksIncludedInFocusChain")       settings.linksIncludedInFocusChain = value
            else if(item == "LocalContentCanAccessFileUrls")   settings.localContentCanAccessFileUrls = value
            else if(item == "LocalContentCanAccessRemoteUrls") settings.localContentCanAccessRemoteUrls = value
            else if(item == "LocalStorageEnabled")             settings.localStorageEnabled = value
            else if(item == "PluginsEnabled")                  settings.pluginsEnabled = value
            else if(item == "SpatialNavigationEnabled")        settings.spatialNavigationEnabled = value
            else if(item == "HyperlinkAuditingEnabled")        settings.hyperlinkAuditingEnabled = value
            else if(item == "ScrollAnimatorEnabled")           settings.scrollAnimatorEnabled = value

            else if(item == "ScreenCaptureEnabled")            settings.screenCaptureEnabled = value
            else if(item == "WebAudioEnabled")                 settings.webAudioEnabled = value
            else if(item == "WebGLEnabled")                    settings.webGLEnabled = value
            else if(item == "Accelerated2dCanvasEnabled")      settings.accelerated2dCanvasEnabled = value
            else if(item == "AutoLoadIconsForPage")            settings.autoLoadIconsForPage = value
            else if(item == "TouchIconsEnabled")               settings.touchIconsEnabled = value

            else if(item == "ErrorPageEnabled")                settings.errorPageEnabled = value
            else if(item == "FullScreenSupportEnabled")        settings.fullScreenSupportEnabled = value
        }

        function setFontFamily(item, value){
            // not implemented yet?
            //if     (item == "StandardFont")  experimental.settings.standardFontFamily = value
            //else if(item == "FixedFont")     experimental.settings.fixedFontFamily = value
            //else if(item == "SerifFont")     experimental.settings.serifFontFamily = value
            //else if(item == "SansSerifFont") experimental.settings.sansSerifFontFamily = value
            //else if(item == "CursiveFont")   experimental.settings.cursiveFontFamily = value
            //else if(item == "FantasyFont")   experimental.settings.fantasyFontFamily = value
        }

        function setFontSize(item, value){
            // not implemented yet?
            //if     (item == "MinimumFontSize")      experimental.settings.minimumFontSize = value
            //else if(item == "DefaultFontSize")      experimental.settings.defaultFontSize = value
            //else if(item == "DefaultFixedFontSize") experimental.settings.defaultFixedFontSize = value
        }

        experimental {

            //onFullScreenRequested: {
            //    if(viewInterface.isFullScreen())
            //        viewInterface.showNormal()
            //    else
            //        viewInterface.showFullScreen()
            //}

            //onIsFullScreenChanged: {
            //}

            onExtraContextMenuEntriesComponentChanged: {
            }

            //extraContextMenuEntriesComponent: ContextMenuExtras {}

            //onFeaturePermissionRequested: {
            //    var featureString
            //    switch(feature){
            //    case MediaAudioDevices:      featureString = "MediaAudioDevices"      break
            //    case MediaVideoDevices:      featureString = "MediaVideoDevices"      break
            //    case MediaAudioVideoDevices: featureString = "MediaAudioVideoDevices" break
            //    }
            //    featurePermissionDialog.securityOrigin = securityOrigin
            //    featurePermissionDialog.title = "Feature Permission Requested."
            //    featurePermissionDialog.text = "Feature Permission Requested."
            //    featurePermissionDialog.informativeText =
            //        qsTr("Url: ") + securityOrigin.toString() + "\n"
            //        qsTr("Feature: ") + featureString + "\n\n"
            //        qsTr("Allow this feature?")
            //    featurePermissionDialog.visible = true
            //}
            //featurePermissionDialog : MessageDialog {
            //    url securityOrigin
            //    Feature feature
            //    standardButtons : StandardButton.Yes | StandardButton.No
            //    onYes: {
            //        grantFeaturePermission(securityOrigin, feature, true)
            //        visible = false
            //    }
            //    onNo: {
            //        grantFeaturePermission(securityOrigin, feature, false)
            //        visible = false
            //    }
            //    Component.onCompleted:{
            //        visible = true
            //    }
            //}
        }
    }
}

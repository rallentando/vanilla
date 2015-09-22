import QtQuick 2.5
import QtQuick.Dialogs 1.2
import QtQuick.Window 2.2
//[[QWV]]
import QtWebKit 3.0
import QtWebKit.experimental 1.0
//[[/QWV]]
//[[QWEV]]
import QtWebEngine 1.1
import QtWebEngine.experimental 1.0
//[[/QWEV]]

Rectangle {
    WebViewBase {
        id: webViewBase
        signal viewChanged()
        signal scrollChanged(point pos)
        signal statusBarMessage(string message)
        signal titleChanged_(string title)
        signal linkHovered_(string arg1, string arg2, string arg3)
        signal callBackResult(int id, variant result)
        //[[QWEV]]
        property real devicePixelRatio : 1.0
        //[[/QWEV]]

        anchors.fill: parent

        onLoadingChanged: {
            var status = loadRequest.status

            if(status == WebViewBase.LoadStartedStatus){
                statusBarMessage(qsTr('started loading.'))
            }
            if(status == WebViewBase.LoadSucceededStatus){
                restoreScroll()
                viewChanged()
                statusBarMessage(qsTr('finished loading.'))
                progressBar.width = parent.width
            }
            if(status == WebViewBase.LoadFaliedStatus){
                statusBarMessage(qsTr('failed to load.'))
                progressBar.width = parent.width
            }
        }

        onLoadProgressChanged: {
            adjustContents()
            restoreScroll()
            viewChanged()
            statusBarMessage(qsTr('loading ... (%1 percent)')
                             .replace('%1', loadProgress.toString()))
            progressBar.width = parent.width * loadProgress / 100
        }

        onLinkHovered: {
            //[[QWV]]
            linkHovered_(hoveredUrl.toString(), hoveredTitle, '')
            //[[/QWV]]
            //[[QWEV]]
            linkHovered_(hoveredUrl.toString(), '', '')
            //[[/QWEV]]
        }

        onTitleChanged: {
            titleChanged_(title)
            viewInterface.changeNodeTitle(title)
        }

        onUrlChanged: {
            viewInterface.changeNodeUrl(url)
        }

        //onNavigationRequested: {
        //    if(request.navigationType == WebViewBase.LinkClickedNavigation) console.log("link")
        //    if(request.navigationType == WebViewBase.FormSubmittedNavigation) console.log("submit")
        //    if(request.navigationType == WebViewBase.BackForwardNavigation) console.log("backforward")
        //    if(request.navigationType == WebViewBase.ReloadNavigation) console.log("reload")
        //    if(request.navigationType == WebViewBase.FormResubmittedNavigation) console.log("resubmit")
        //    if(request.navigationType == WebViewBase.OtherNavigation) console.log("other")
        //
        //    // cannot catch mouse button...
        //    if(request.mouseButton == Qt.LeftButton) console.log("left button")
        //    if(request.mouseButton == Qt.RightButton) console.log("right button")
        //    if(request.mouseButton == Qt.MidButton) console.log("mid button")
        //
        //    // cannot catch keyboard modifiers...
        //    if(request.keyboardModifiers & Qt.ShiftModifier) console.log("shift mod")
        //    if(request.keyboardModifiers & Qt.ControlModifier) console.log("control mod")
        //
        //    if(request.navigationType != WebViewBase.LinkClickedNavigation){
        //        request.action = WebViewBase.AcceptRequest
        //        return
        //    }
        //    if(request.mouseButton == Qt.LeftButton ||
        //       request.mouseButton == Qt.MidButton){
        //
        //        if(request.keyboardModifiers & Qt.ShiftModifier ||
        //           request.mouseButton == Qt.MidButton){
        //
        //            var ctrl = request.keyboardModifiers & Qt.ControlModifier;
        //            request.action = WebViewBase.IgnoreRequest
        //            viewInterface.openInNewViewNode(request.url, !ctrl)
        //            return
        //
        //        } else if(viewInterface.enableLoadHack() &&
        //                  !(request.keyboardModifiers & Qt.ControlModifier)){
        //
        //            request.action = WebViewBase.IgnoreRequest
        //            viewInterface.openInNewHistNode(request.url, true)
        //            return
        //        }
        //    }
        //    request.action = WebViewBase.AcceptRequest
        //}

        //[[QWEV]]
        onDevicePixelRatioChanged: {
            experimental.viewport.devicePixelRatio = devicePixelRatio
        }
        //[[/QWEV]]

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
            //[[QWEV]]
            viewInterface.saveZoomToNode(devicePixelRatio)
            //[[/QWEV]]
        }

        function restoreZoom(){
            //[[QWEV]]
            devicePixelRatio = viewInterface.restoreZoomFromNode()
            //[[/QWEV]]
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

        function adjustContents(){
            experimental.preferredMinimumContentsWidth = parent.width
        }

        function seekText(str, opt){
            var option = 0
            if(opt & viewInterface.findBackwardIntValue())
                option |= WebViewBaseExperimental.FindBackward
            if(opt & viewInterface.caseSensitivelyIntValue())
                option |= WebViewBaseExperimental.FindCaseSensitively
            //[[QWV]]
            if(opt & viewInterface.wrapsAroundDocumentIntValue())
                option |= WebViewBaseExperimental.FindWrapsAroundDocument
            if(opt & viewInterface.highlightAllOccurrencesIntValue())
                option |= WebViewBaseExperimental.FindHighlightAllOccurrences
            //[[/QWV]]

            experimental.findText(str, option)
            emitScrollChangedIfNeed()
        }

        function setUserAgent(agent){
            experimental.userAgent = agent
        }

        function setDefaultTextEncoding(encoding){
            //[[QWV]]
            //[[/QWV]]
            //[[QWEV]]
            settings.defaultTextEncoding = encoding
            //[[/QWEV]]
        }

        function setPreference(item, value){
            //[[QWV]]
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
            //[[/QWV]]
            //[[QWEV]]
            if     (item == "AutoLoadImages")                    settings.autoLoadImages = value
            else if(item == "HyperlinkAuditingEnabled")          settings.hyperlinkAuditingEnabled = value
            else if(item == "JavascriptCanAccessClipboard")      settings.javascriptCanAccessClipboard = value
            else if(item == "JavascriptCanOpenWindows")          settings.javascriptCanOpenWindows = value
            else if(item == "JavascriptEnabled")                 settings.javascriptEnabled = value
            else if(item == "LinksIncludedInFocusChain")         settings.linksIncludedInFocusChain = value
            else if(item == "LocalContentCanAccessFileUrls")     settings.localContentCanAccessFileUrls = value
            else if(item == "LocalContentCanAccessRemoteUrls")   settings.localContentCanAccessRemoteUrls = value
            else if(item == "LocalStorageEnabled")               settings.localStorageEnabled = value
            else if(item == "ErrorPageEnabled")                  settings.errorPageEnabled = value
            //[[/QWEV]]
        }

        function setFontFamily(item, value){
            //[[QWV]]
            if     (item == "StandardFont")  experimental.preferences.standardFontFamily = value
            else if(item == "FixedFont")     experimental.preferences.fixedFontFamily = value
            else if(item == "SerifFont")     experimental.preferences.serifFontFamily = value
            else if(item == "SansSerifFont") experimental.preferences.sansSerifFontFamily = value
            else if(item == "CursiveFont")   experimental.preferences.cursiveFontFamily = value
            else if(item == "FantasyFont")   experimental.preferences.fantasyFontFamily = value
            //[[/QWV]]
            //[[QWEV]]
            // not implemented yet?
            //if     (item == "StandardFont")  experimental.settings.standardFontFamily = value
            //else if(item == "FixedFont")     experimental.settings.fixedFontFamily = value
            //else if(item == "SerifFont")     experimental.settings.serifFontFamily = value
            //else if(item == "SansSerifFont") experimental.settings.sansSerifFontFamily = value
            //else if(item == "CursiveFont")   experimental.settings.cursiveFontFamily = value
            //else if(item == "FantasyFont")   experimental.settings.fantasyFontFamily = value
            //[[/QWEV]]
        }

        function setFontSize(item, value){
            //[[QWV]]
            if     (item == "MinimumFontSize")      experimental.preferences.minimumFontSize = value
            else if(item == "DefaultFontSize")      experimental.preferences.defaultFontSize = value
            else if(item == "DefaultFixedFontSize") experimental.preferences.defaultFixedFontSize = value
            //[[/QWV]]
            //[[QWEV]]
            // not implemented yet?
            //if     (item == "MinimumFontSize")      experimental.settings.minimumFontSize = value
            //else if(item == "DefaultFontSize")      experimental.settings.defaultFontSize = value
            //else if(item == "DefaultFixedFontSize") experimental.settings.defaultFixedFontSize = value
            //[[/QWEV]]
        }

        //[[QWV]]
        function inspect(){
            inspector.visible = true
        }

        Window {
            id: inspector
            visible: false
            WebViewBase {
                anchors.fill: parent
                url: webViewBase.experimental.remoteInspectorUrl
            }
        }
        //[[/QWV]]

        experimental {

            //[[QWEV]]

            // moved.
            //inspectable : true

            //onNewViewRequested: {
            //    if(request.destination == WebViewBase.NewViewInDialog){
            //        request.openIn(webViewBase)
            //        return
            //    }
            //    if(request.mouseButton == Qt.LeftButton ||
            //       request.mouseButton == Qt.MidButton){
            //
            //        if(request.keyboardModifiers & Qt.ShiftModifier ||
            //           request.mouseButton == Qt.MidButton){
            //
            //            if(request.keyboardModifiers & Qt.ControlModifier){
            //                if(view = viewInterface.newViewNodeBackground())
            //                    request.openIn(view)
            //            } else {
            //                if(view = viewInterface.newViewNodeForeground())
            //                    request.openIn(view)
            //            }
            //            return
            //
            //        } else if(viewInterface.enableLoadHack() &&
            //                  !(request.keyboardModifiers & Qt.ControlModifier)){
            //
            //            if(view = viewInterface.newHistNodeForeground())
            //                request.openIn(view)
            //            return
            //        }
            //    }
            //    request.openIn(webViewBase)
            //}

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
            //[[/QWEV]]

            // properties.
            //transparentBackground
            //useDefaultContentItemSize
            //preferredMinimumContentsWidth
            //deviceWidth
            //deviceHeight
            //userScripts

            // dialogs.
            //alertDialog
            //confirmDialog
            //promptDialog
            //authenticationDialog
            //proxyAuthenticationDialog
            //certificateVerificationDialog
            //itemSelector
            //filePicker
            //databaseQuotaDialog
            //colorChooser

            //[[QWV]]
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
                // orthodox switches
                //autoLoadImages
                fullScreenEnabled : true
                //javascriptEnabled
                //pluginsEnabled
                //offlineWebApplicationCacheEnabled
                //localStorageEnabled
                //xssAuditingEnabled
                //privateBrowsingEnabled
                //dnsPrefetchEnabled
                navigatorQtObjectEnabled : true
                //frameFlatteningEnabled
                //developerExtrasEnabled
                //webGLEnabled
                //webAudioEnabled
                //caretBrowsingEnabled
                //notificationsEnabled
                //universalAccessFromFileURLsAllowed
                //fileAccessFromFileURLsAllowed

                // font family
                //standardFontFamily
                //fixedFontFamily
                //serifFontFamily
                //sansSerifFontFamily
                //cursiveFontFamily
                //fantasyFontFamily

                // font size
                //minimumFontSize
                //defaultFontSize
                //defaultFixedFontSize
            }
            //[[/QWV]]
        }
    }
    Rectangle {
        id: progressBar
        anchors.left: parent.left
        anchors.top: parent.top
        width: 0
        height: 5
        color: (width == parent.width) ? 'transparent' : '#aa000000'
    }
}

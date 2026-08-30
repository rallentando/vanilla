import QtQuick
import QtWebEngine

WebEngineView {
    // named so that a 'Connections' below can aim at this view itself:
    // inside the root item 'parent' is not it. (D-162)
    id: webEngineView

    signal viewChanged()
    signal scrollChanged(point pos)
    signal callBackResult(int id, variant result)

    // since Qt6, 'WebEngineScript' is not creatable from QML.
    // it must be obtained from the 'WebEngine' singleton.
    function makeDefaultScript(){
        var script = WebEngine.script()
        script.injectionPoint = WebEngineScript.DocumentReady
        script.worldId = WebEngineScript.MainWorld
        script.runsOnSubFrames = true
        script.sourceCode = viewInterface.defaultScript()
        return script
    }

    onNavigationRequested: {
        if(userScripts.collection.length == 0){
            userScripts.collection = [makeDefaultScript()]
        }
    }

    onLoadingChanged: function(loadingInfo) {
        var status = loadingInfo.status

        // the application's own pages draw themselves with their own
        // scripts, so a directory saying '!Javascript' must not reach them.
        // The widgets view has this on its page ('WebEnginePage::
        // HandleLoading', D-119); this view loads through the QML item and
        // that page never sees the load, so the url is handed over here.
        // The address at the start of the load is what is asked, because a
        // move to the settings page inside this same view is exactly what
        // somebody who is locked out tries first. (D-128)
        viewInterface.suspendSpecificSettingsIfNeed(loadingInfo.url)

        if(status == WebEngineView.LoadStartedStatus){
            viewInterface.loadStarted()
        }
        if(status == WebEngineView.LoadSucceededStatus){
            viewInterface.loadFinished(true)
        }
        if(status == WebEngineView.LoadFailedStatus){
            viewInterface.loadFinished(false)
        }
    }

    onLoadProgressChanged: {
        viewInterface.loadProgress(loadProgress)
    }

    onLinkHovered: function(hoveredUrl) {
        viewInterface.linkHovered(hoveredUrl.toString(), '', '')
    }

    onTitleChanged: {
        viewInterface.titleChanged(title)
    }

    onUrlChanged: {
        viewInterface.urlChanged(url)
    }

    onIconChanged: {
        viewInterface.iconUrlChanged(icon)
    }

    // the audio state lives on the QML view; the C++ side relays this to
    // the media-position timer (MEDIATIME, D-063).
    onRecentlyAudibleChanged: {
        viewInterface.recentlyAudibleChanged(recentlyAudible)
    }

    onContextMenuRequested: function(request) {
        request.accepted = true
        var isMedia = (request.mediaType == ContextMenuRequest.MediaTypeVideo ||
                       request.mediaType == ContextMenuRequest.MediaTypeAudio)
        viewInterface.contextMenuRequested(request, isMedia)
    }

    onWindowCloseRequested: {
        viewInterface.windowCloseRequested()
    }

    onJavaScriptConsoleMessage: function(level, message, lineNumber, sourceID) {
        viewInterface.javascriptConsoleMessage(level, message)
    }

    onFeaturePermissionRequested: function(securityOrigin, feature) {
        viewInterface.featurePermissionRequested(securityOrigin, feature)
    }

    // 'window.print()' in the page. nothing was listening, so a page which
    // asked to be printed was simply not. (D-091)
    onPrintRequested: {
        viewInterface.printRequested()
    }

    onRenderProcessTerminated: function(terminationStatus, exitCode) {
        viewInterface.renderProcessTerminated(terminationStatus, exitCode)
    }

    // the certificate the engine refuses to accept on its own. Nothing was
    // listening, so an unanswered error was destroyed and the load failed
    // without ever asking -- the directory's 'SslErrorPolicy' never got a
    // say on this view. The answer is the widgets one: the page this view
    // owns is not the page it draws with, but it is the one which knows
    // what the policy says. (D-162)
    onCertificateError: function(error) {
        viewInterface.certificateError(error)
    }

    // these two arrived in 6.7/6.8. 'Connections' with 'ignoreUnknownSignals'
    // so that an older Qt leaves them alone instead of failing to load the
    // whole view -- a '.qml' has no '#if QT_VERSION'. (D-162)
    Connections {
        target: webEngineView
        ignoreUnknownSignals: true
        function onDesktopMediaRequested(request){
            viewInterface.desktopMediaRequested(request)
        }
        function onWebAuthUxRequested(request){
            viewInterface.webAuthUxRequested(request)
        }
    }

    // 'newViewRequested' has been renamed to 'newWindowRequested' since Qt6.
    // A FIXME stood here for years: "application will crash when Page A
    // opens Page B, and close Page A". On Qt 6.11 it does not: closing the
    // opener leaves the opened view running and nulls its 'window.opener'.
    // Tried on the real application -- open then close 6s later, open then
    // close in the same task, and closing the opener by the user's Close
    // while the opened view lives -- none of them fall. (D-200)
    onNewWindowRequested: function(request) {
        // null means the request was refused -- a link dropped back onto the
        // view it came from asks for a window and gets none (D-089).
        var view = request.destination == WebEngineNewWindowRequest.InNewBackgroundTab
            ? viewInterface.newViewBackground()
            : viewInterface.newView()
        if(view) request.openIn(view)
    }

    onFullScreenRequested: function(request) {
        viewInterface.fullScreenRequested(request.toggleOn)
        request.accept()
    }

    onContentsSizeChanged: function(size) {
        viewInterface.contentsSizeChanged(size)
    }

    onScrollPositionChanged: function(position) {
        viewInterface.scrollPositionChanged(position)
    }

    Connections {
        target: profile
        function onDownloadRequested(download){
            viewInterface.downloadRequested(download)
        }
    }

    function setScroll(pos){
        runJavaScript
        (viewInterface.setScrollRatioPointJsCode(pos),
         WebEngineScript.MainWorld)
    }

    function saveScroll(){
        runJavaScript
        (viewInterface.getScrollValuePointJsCode(),
         WebEngineScript.MainWorld,
         function(result){
             viewInterface.saveScrollToNode(Qt.point(result[0], result[1]))
         })
    }

    function restoreScroll(){
        var pos = viewInterface.restoreScrollFromNode()
        runJavaScript
        (viewInterface.setScrollValuePointJsCode(pos),
         WebEngineScript.MainWorld)
    }

    function saveZoom(){
        viewInterface.saveZoomToNode(zoomFactor)
    }

    function restoreZoom(){
        zoomFactor = viewInterface.restoreZoomFromNode()
    }

    function evaluateJavaScript(id, code){
        runJavaScript
        (code,
         WebEngineScript.MainWorld,
         function(result){
             callBackResult(id, result)
         })
    }

    function emitScrollChanged(){
        runJavaScript
        (viewInterface.getScrollRatioPointJsCode(),
         WebEngineScript.MainWorld,
         function(pointf){
             scrollChanged(Qt.point(pointf[0], pointf[1]))
         })
    }

    function seekText(str, opt){
        var option = 0
        if(opt & viewInterface.findBackwardIntValue())
            option |= WebEngineView.FindBackward
        if(opt & viewInterface.caseSensitivelyIntValue())
            option |= WebEngineView.FindCaseSensitively

        findText(str, option)
    }

    // 'navigationHistory' is the Qt5 name; since Qt6 the property is 'history'.
    function rewind(){
        var count = history.backItems.rowCount()
        if(count) goBackOrForward(-count)
    }
    function fastForward(){
        var count = history.forwardItems.rowCount()
        if(count) goBackOrForward(count)
    }
    function copy(){
        triggerWebAction(WebEngineView.Copy)
    }
    function cut(){
        triggerWebAction(WebEngineView.Cut)
    }
    function paste(){
        triggerWebAction(WebEngineView.Paste)
    }
    function pasteAndMatchStyle(){
        triggerWebAction(WebEngineView.PasteAndMatchStyle)
    }
    // for a page which is being edited. (D-091)
    function toggleBold(){
        triggerWebAction(WebEngineView.ToggleBold)
    }
    function toggleItalic(){
        triggerWebAction(WebEngineView.ToggleItalic)
    }
    function toggleUnderline(){
        triggerWebAction(WebEngineView.ToggleUnderline)
    }
    function toggleStrikethrough(){
        triggerWebAction(WebEngineView.ToggleStrikethrough)
    }
    function alignLeft(){
        triggerWebAction(WebEngineView.AlignLeft)
    }
    function alignCenter(){
        triggerWebAction(WebEngineView.AlignCenter)
    }
    function alignRight(){
        triggerWebAction(WebEngineView.AlignRight)
    }
    function alignJustified(){
        triggerWebAction(WebEngineView.AlignJustified)
    }
    function indent(){
        triggerWebAction(WebEngineView.Indent)
    }
    function outdent(){
        triggerWebAction(WebEngineView.Outdent)
    }
    function insertOrderedList(){
        triggerWebAction(WebEngineView.InsertOrderedList)
    }
    function insertUnorderedList(){
        triggerWebAction(WebEngineView.InsertUnorderedList)
    }
    function changeTextDirectionLTR(){
        triggerWebAction(WebEngineView.ChangeTextDirectionLTR)
    }
    function changeTextDirectionRTL(){
        triggerWebAction(WebEngineView.ChangeTextDirectionRTL)
    }
    function undo(){
        triggerWebAction(WebEngineView.Undo)
    }
    function redo(){
        triggerWebAction(WebEngineView.Redo)
    }
    function selectAll(){
        triggerWebAction(WebEngineView.SelectAll)
    }
    function unselect(){
        runJavaScript("(function(){ document.activeElement.blur(); getSelection().removeAllRanges();})();",
                      WebEngineScript.MainWorld)
    }
    function reloadAndBypassCache(){
        triggerWebAction(WebEngineView.ReloadAndBypassCache)
    }
    function stopAndUnselect(){
        stop(); unselect()
    }
    // the PDF half of 'Print'. 'printToPdf' is a slot on the engine's own
    // item, so this is the same thin wrapper the rest of the actions are
    // ('save', 'stop'): the C++ side names one QML function per action and
    // never reaches for an engine slot itself. It was left empty, and
    // because nothing ever called it the emptiness cost nothing and stayed
    // -- 'tst_quickviewbridge' now watches for that. The image half stays on
    // the C++ side; it grabs the widget's framebuffer, which QML cannot
    // reach. (D-161)
    function print_(filePath){
        printToPdf(filePath)
    }
    function save(){
        triggerWebAction(WebEngineView.SavePage)
    }
    function toggleMediaControls(){
        triggerWebAction(WebEngineView.ToggleMediaControls)
    }
    function toggleMediaLoop(){
        triggerWebAction(WebEngineView.ToggleMediaLoop)
    }
    function toggleMediaPlayPause(){
        triggerWebAction(WebEngineView.ToggleMediaPlayPause)
    }
    function toggleMediaMute(){
        triggerWebAction(WebEngineView.ToggleMediaMute)
    }
    function grantFeaturePermission_(securityOrigin, feature, granted){
        grantFeaturePermission(securityOrigin, feature, granted);
    }

    /*
      What happens to a view which is no longer the one on screen.

      The word arrives from 'webview/@SuspendHiddenViews' and is turned into
      the engine's enum here, the same way 'setUnknownUrlSchemePolicy' does
      it: the C++ side of this view knows no engine enum, and the number
      behind 'Frozen' is not something to write down twice. Anything which
      is not one of the two suspensions leaves the page running, which is
      the safe reading of a word from an older or newer settings file.

      'isDiscarded' is asked by the C++ before it reloads a page whose render
      process died: a discarded tab loses that process on purpose, and
      reloading it would put it straight back to Active. (D-116, D-162)
     */
    function suspend(state){
        if(state == "Discarded")
            lifecycleState = WebEngineView.LifecycleState.Discarded
        else if(state == "Frozen")
            lifecycleState = WebEngineView.LifecycleState.Frozen
    }

    function wakeUp(){
        if(lifecycleState != WebEngineView.LifecycleState.Active)
            lifecycleState = WebEngineView.LifecycleState.Active
    }

    function isDiscarded(){
        return lifecycleState == WebEngineView.LifecycleState.Discarded
    }

    function setUserAgent(agent){
        profile.httpUserAgent = agent
    }

    function setAcceptLanguage(language){
        profile.httpAcceptLanguage = language
    }

    function setDefaultTextEncoding(encoding){
        settings.defaultTextEncoding = encoding
    }

    function setUnknownUrlSchemePolicy(policy){
        if      (policy == "DisallowUnknownUrlSchemes")
            settings.unknownUrlSchemePolicy = WebEngineSettings.DisallowUnknownUrlSchemes
        else if (policy == "AllowUnknownUrlSchemesFromUserInteraction")
            settings.unknownUrlSchemePolicy = WebEngineSettings.AllowUnknownUrlSchemesFromUserInteraction
        else if (policy == "AllowAllUnknownUrlSchemes")
            settings.unknownUrlSchemePolicy = WebEngineSettings.AllowAllUnknownUrlSchemes
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
        // available again since Qt6.8
        else if(item == "ScrollAnimatorEnabled")           settings.scrollAnimatorEnabled = value

        else if(item == "ScreenCaptureEnabled")            settings.screenCaptureEnabled = value
        else if(item == "WebGLEnabled")                    settings.webGLEnabled = value
        else if(item == "Accelerated2dCanvasEnabled")      settings.accelerated2dCanvasEnabled = value
        else if(item == "AutoLoadIconsForPage")            settings.autoLoadIconsForPage = value
        else if(item == "TouchIconsEnabled")               settings.touchIconsEnabled = value
        else if(item == "FocusOnNavigationEnabled")        settings.focusOnNavigationEnabled = value
        else if(item == "PrintElementBackgrounds")         settings.printElementBackgrounds = value
        else if(item == "AllowRunningInsecureContent")     settings.allowRunningInsecureContent = value
        else if(item == "AllowGeolocationOnInsecureOrigins") settings.allowGeolocationOnInsecureOrigins = value
        // since Qt5.10
        else if(item == "AllowWindowActivationFromJavaScript") settings.allowWindowActivationFromJavaScript = value
        else if(item == "ShowScrollBars")                  settings.showScrollBars = value
        // since Qt5.11
        else if(item == "PlaybackRequiresUserGesture")     settings.playbackRequiresUserGesture = value
        else if(item == "WebRTCPublicInterfacesOnly")      settings.webRTCPublicInterfacesOnly = value
        else if(item == "JavascriptCanPaste")              settings.javascriptCanPaste = value
        // since Qt5.12
        else if(item == "DnsPrefetchEnabled")              settings.dnsPrefetchEnabled = value
        // since Qt5.13
        else if(item == "PdfViewerEnabled")                settings.pdfViewerEnabled = value

        else if(item == "ErrorPageEnabled")                settings.errorPageEnabled = value
        else if(item == "FullScreenSupportEnabled")        settings.fullScreenSupportEnabled = value

        // since Qt6.4
        else if(item == "NavigateOnDropEnabled")           settings.navigateOnDropEnabled = value
        // since Qt6.6
        else if(item == "ReadingFromCanvasEnabled")        settings.readingFromCanvasEnabled = value
        // since Qt6.7
        else if(item == "ForceDarkMode")                   settings.forceDarkMode = value
        // since Qt6.9
        else if(item == "PrintHeaderAndFooter")            settings.printHeaderAndFooter = value
        else if(item == "PreferCSSMarginsForPrinting")     settings.preferCSSMarginsForPrinting = value
        else if(item == "TouchEventsApiEnabled")           settings.touchEventsApiEnabled = value
        // since Qt6.10
        else if(item == "BackForwardCacheEnabled")         settings.backForwardCacheEnabled = value
        // since Qt6.11
        else if(item == "TrimAccessibilityIdentifiers")    settings.trimAccessibilityIdentifiers = value
    }

    // No font setting reaches this view, and none can.
    //
    // The widgets side hands every one of these to 'QWebEngineSettings'
    // ('View::LoadSettings' fills the global one from 'webview/font/...',
    // and 'ApplySpecificSettings' copies it down to the page). The QML
    // 'WebEngineSettings' has no counterpart: not one font property, and no
    // injection point either. Checked three ways against Qt 6.11.1 --
    // 'qquickwebenginesettings_p.h' declares no font 'Q_PROPERTY', the whole
    // of QtWebEngineQuick's headers never spell 'fontFamily', and the
    // 'WebEngineSettings' entry of 'plugins.qmltypes' lists 23 properties of
    // which none is a font.
    //
    // So the two functions which used to stand here were empty, and the C++
    // which called them was calling into nothing. Both sides are gone rather
    // than left looking as though the setting arrives. Approximating them
    // with injected CSS was considered and refused: it cannot rebind the
    // generic families, cannot honour a minimum size at all, and would
    // change how pages which set their own fonts are drawn. (D-161)
}

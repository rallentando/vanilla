import QtQuick 2.7
import QtQuick.Dialogs 1.2
import QtQuick.Controls 1.4
import QtQuick.Window 2.2
import QtWebEngine 1.3

WebEngineView {
    signal viewChanged()
    signal scrollChanged(point pos)
    signal callBackResult(int id, variant result)

    WebEngineScript {
        id: defaultScript
        injectionPoint: WebEngineScript.DocumentReady
        worldId: WebEngineScript.MainWorld
        runOnSubframes: true
    }

    onNavigationRequested: {
        if(userScripts.length == 0){
            defaultScript.sourceCode = viewInterface.defaultScript()
            userScripts = [defaultScript]
        }
    }

    onLoadingChanged: {
        var status = loadRequest.status

        if(status == WebEngineView.LoadStartedStatus){
            viewInterface.loadStarted()
        }
        if(status == WebEngineView.LoadSucceededStatus){
            viewInterface.loadFinished(true)
        }
        if(status == WebEngineView.LoadFaliedStatus){
            viewInterface.loadFinished(false)
        }
    }

    onLoadProgressChanged: {
        viewInterface.loadProgress(loadProgress)
    }

    onLinkHovered: {
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

    onWindowCloseRequested: {
        viewInterface.windowCloseRequested()
    }

    onJavaScriptConsoleMessage: {
        viewInterface.javascriptConsoleMessage(level, message)
    }

    onFeaturePermissionRequested: {
        viewInterface.featurePermissionRequested(securityOrigin, feature)
    }

    onRenderProcessTerminated: {
        viewInterface.renderProcessTerminated(terminationStatus, exitCode)
    }

    onNewViewRequested: {
        if(request.destination == WebEngineView.NewViewInBackgroundTab)
            request.openIn(viewInterface.newViewBackground())
        else request.openIn(viewInterface.newView())
    }

    onFullScreenRequested: {
        viewInterface.fullScreenRequested(request.toggleOn)
        request.accept()
    }

    onContentsSizeChanged: {
        viewInterface.contentsSizeChanged(size)
    }

    onScrollPositionChanged: {
        viewInterface.scrollPositionChanged(position)
    }

    profile.onDownloadRequested: {
        viewInterface.downloadRequested(download)
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
            option |= FindBackward
        if(opt & viewInterface.caseSensitivelyIntValue())
            option |= FindCaseSensitively

        findText(str, option)
    }

    function rewind(){
        var count = navigationHistory.backItems.rowCount()
        if(count) goBackOrForward(-count)
    }
    function fastForward(){
        var count = navigationHistory.forwardItems.rowCount()
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
        runJavaScript("(function(){ document.activeElement.blur(); getSelection().removeAllRanges();})()",
                      WebEngineScript.MainWorld)
    }
    function reloadAndBypassCache(){
        triggerWebAction(WebEngineView.ReloadAndBypassCache)
    }
    function stopAndUnselect(){
        stop(); unselect()
    }
    function print_(){
        // not yet implemented.
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

    function setUserAgent(agent){
        profile.httpUserAgent = agent
    }

    function setAcceptLanguage(language){
        profile.httpAcceptLanguage = language
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

        else if(item == "ScreenCaptureEnabled")            settings.screenCaptureEnabled = value
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
}

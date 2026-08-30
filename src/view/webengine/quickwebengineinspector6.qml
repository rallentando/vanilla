// versionless imports, as in quickwebengineview6.qml.
import QtQuick
import QtWebEngine

// the DevTools window of a 'QuickWebEngineView'. a quick view has no
// 'QWebEnginePage', so 'setDevToolsPage' -- the route the widgets view takes
// -- is closed to it; the QML side's equivalent is handing this view to the
// inspected view's 'devToolsView'. the profile and the zoom are set from
// C++, where the D-064 pair of them already lives. (D-086)
WebEngineView {

    // the frontend reads its stored theme at boot but does not apply it
    // (the language is applied; watched on Qt 6.11 / the theme picker then
    // shows 'dark' over a light window). setting the value back onto itself
    // fires the change event, whose listener is the code that does the
    // applying. the load finishing comes before the frontend has registered
    // that listener, so this keeps poking for a while; re-setting an applied
    // value changes nothing. the same nudge as the widgets window (D-064).
    onLoadingChanged: function(loadingInfo) {
        if(loadingInfo.status == WebEngineView.LoadSucceededStatus)
            nudgeTheme()
    }

    function nudgeTheme(){
        runJavaScript(
            '(function(){' +
            // the absolute url: a script handed to 'runJavaScript' has
            // 'about:blank' as its base, and a relative specifier does not
            // resolve from there.
            '    var url = new URL("core/common/common.js", location.href).href;' +
            '    var tries = 20;' +
            '    function nudge(){' +
            '        import(url).then(function(mod){' +
            '            var theme = mod.Settings.Settings.instance().moduleSetting("ui-theme");' +
            '            theme.set(theme.get());' +
            '        }).catch(function(){});' +
            '        if(--tries > 0) setTimeout(nudge, 500);' +
            '    }' +
            '    setTimeout(nudge, 500);' +
            '})();')
    }
}

const counter = document.querySelector('#counter');
const state = {ready: false, visited: false, opens: 0};
chrome.storage.local.set({apiProbe: JSON.stringify({
  message: chrome.i18n.getMessage('probe'), language: chrome.i18n.getUILanguage(),
  tabsQuery: typeof chrome.tabs?.query, windows: typeof chrome.windows,
  frames: typeof chrome.webNavigation?.getAllFrames,
  dnr: typeof chrome.declarativeNetRequest?.getEnabledRulesets
})});
if (chrome.tabs?.query) {
  chrome.storage.local.get('timeoutProbe', value => {
    if (value.timeoutProbe === 'arm' && chrome.webview) {
      chrome.webview.postMessage = () => {};
      const started = Date.now();
      chrome.tabs.query({active: true, currentWindow: true}).then(
        tabs => chrome.storage.local.set({timeoutProbe: JSON.stringify({ms: Date.now() - started, count: tabs.length})}),
        error => chrome.storage.local.set({timeoutProbe: 'error:' + error}));
      return;
    }
    Promise.all([chrome.tabs.query({}), chrome.tabs.query({active: true, currentWindow: true})]).then(async ([all, current]) => {
      const pick = tab => ({id: tab.id, windowId: tab.windowId, active: tab.active, url: tab.url});
      let reply = 'none';
      if (current.length) reply = await chrome.tabs.sendMessage(current[0].id, {vanilla: 'ping'}).catch(error => 'error:' + error.message);
      chrome.storage.local.set({tabsProbe: JSON.stringify({at: Date.now(), self: location.href, all: all.map(pick), current: current.map(pick), reply})});
    }, error => chrome.storage.local.set({tabsProbe: 'error:' + error}));
  });
}
function updateTitle() {
  if (state.ready) document.title = `visited=${state.visited};opens=${state.opens};focused=${document.hasFocus()}`;
}
window.addEventListener('focus', updateTitle);
window.addEventListener('blur', updateTitle);
window.addEventListener('keydown', event => { if (event.key === 'F8') document.title = 'keyboard-probe'; });
chrome.storage.local.get({count: 0, visited: false, opens: 0}, value => {
  counter.textContent = value.count;
  document.querySelector('#visited').textContent = String(value.visited);
  const opens = value.opens + 1;
  chrome.storage.local.set({opens}, () => {
    Object.assign(state, {ready: true, visited: value.visited, opens});
    updateTitle();
  });
});
document.querySelector('#increment').onclick = () => {
  const count = Number(counter.textContent) + 1;
  chrome.storage.local.set({count}, () => { counter.textContent = count; document.title = `clicked=${count}`; });
};

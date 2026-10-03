document.documentElement.dataset.vanillaExtension = chrome.runtime.id;
chrome.storage.local.set({visited: true});
chrome.storage.onChanged.addListener(changes => {
  if (changes.apiProbe) document.documentElement.dataset.apiProbe = changes.apiProbe.newValue;
  if (changes.tabsProbe) document.documentElement.dataset.tabsProbe = changes.tabsProbe.newValue;
  if (changes.timeoutProbe) document.documentElement.dataset.timeoutProbe = changes.timeoutProbe.newValue;
});
chrome.storage.local.get(['apiProbe', 'tabsProbe', 'timeoutProbe'], value => {
  if (value.apiProbe) document.documentElement.dataset.apiProbe = value.apiProbe;
  if (value.tabsProbe) document.documentElement.dataset.tabsProbe = value.tabsProbe;
  if (value.timeoutProbe) document.documentElement.dataset.timeoutProbe = value.timeoutProbe;
});
document.addEventListener('vanilla-extension-storage', event => {
  chrome.storage.local.set(JSON.parse(event.detail));
});
chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (message && message.vanilla === 'ping') sendResponse('pong');
});

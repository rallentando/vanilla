chrome.tabs.query({active: true, currentWindow: true}).then(
  tabs => { document.title = 'tabs:' + JSON.stringify(tabs.map(tab => ({id: tab.id, url: tab.url}))); },
  error => { document.title = 'tabs:error:' + error; });

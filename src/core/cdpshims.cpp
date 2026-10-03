#include "switch.hpp"

#include "cdpshims.hpp"

#include <QJsonArray>
#include <QJsonDocument>

namespace Cdp {

namespace {

const char *FAILING = R"js(
  const failing = (error, callback) => {
    setTimeout(() => {
      let said = false;
      try { chrome.runtime.lastError = { message: error.message }; said = chrome.runtime.lastError.message === error.message; } catch (e) {}
      if (!said) return;
      try { callback(); } catch (e) {}
      try { delete chrome.runtime.lastError; } catch (e) {}
    }, 0);
  };
  const toCallback = (answer, callback, bare) => {
    answer.then((value) => { try { if (bare) callback(); else callback(value); } catch (e) {} },
                (error) => { failing(error, callback); });
    return undefined;
  };
  const listenedTo = (listeners, added, emptied) => ({
    addListener(f) {
      if (typeof f !== 'function' || listeners.includes(f)) return;
      listeners.push(f);
      if (added) added();
    },
    removeListener(f) {
      const at = listeners.indexOf(f);
      if (at >= 0) listeners.splice(at, 1);
      if (!listeners.length && emptied) emptied();
    },
    hasListener(f) { return listeners.includes(f); },
    hasListeners() { return listeners.length > 0; }
  });
)js";

const char *ENGINE = R"js(
  const EDGE = (() => {
    try {
      const brands = navigator.userAgentData.brands;
      if (!Array.isArray(brands)) return false;
      for (const one of brands) if (one && one.brand === 'Microsoft Edge WebView2') return true;
    } catch (e) {}
    return false;
  })();
  made.push(EDGE ? 'engine:edge' : 'engine:qt');
)js";

const char *HOST = R"js(
  const HOST_KEY = '__VANILLA_HOST_KEY__';
  const hosted = new Set(/^[0-9a-f]{64}$/.test(HOST_KEY) && typeof fetch === 'function'
                         ? ['tabs.query', 'tabs.get', 'tabs.create', 'tabs.update', 'tabs.remove', 'tabs.reload',
                            'tabs.duplicate', 'tabs.move',
                            // the zoom, which is the node's (E-3c, D-387).
                            'tabs.getZoom', 'tabs.setZoom',
                            // the current tab as a picture (D-439).
                            'tabs.captureVisibleTab',
                            // the side panel (D-440).
                            'sidePanel.setOptions', 'sidePanel.getOptions', 'sidePanel.setPanelBehavior',
                            'sidePanel.getPanelBehavior', 'sidePanel.open', 'sidePanel.close',
                            // the notifications, as the application's dialogs (D-418b).
                            'notifications.create', 'notifications.update', 'notifications.clear',
                            'notifications.getAll', 'notifications.getPermissionLevel',
                            'windows.getCurrent', 'windows.getAll', 'windows.update',
                            'history.search', 'bookmarks.getTree',
                            // and the other reads of that tree (D-403).
                            'bookmarks.get', 'bookmarks.getChildren', 'bookmarks.getSubTree', 'bookmarks.search',
                            'search.query', 'sessions.restore',
                            // the same leaves as the sites looked at last (D-450a):
                            // WebView2's own knows nothing of the tree.
                            'topSites.get',
                            // the fonts this computer has (D-404). Dark Reader asks
                            // from its popup.
                            'fontSettings.getFontList',
                            // the extension's own shortcuts (D-416).
                            'commands.getAll',
                            // the button and the menu (S2b-11 / S2b-12, D-374).
                            'action.setIcon', 'action.setBadgeText', 'action.setBadgeBackgroundColor',
                            'action.setBadgeTextColor', 'action.setTitle', 'action.enable', 'action.disable',
                            'action.getBadgeText', 'action.getBadgeBackgroundColor', 'action.getBadgeTextColor',
                            'action.getTitle', 'action.isEnabled', 'action.getUserSettings',
                            // the button pressed by the extension itself (D-419).
                            'action.openPopup',
                            'contextMenus.create', 'contextMenus.update', 'contextMenus.remove', 'contextMenus.removeAll',
                            // and its offscreen document (S2b-15, D-380).
                            'offscreen.createDocument', 'offscreen.closeDocument', 'offscreen.hasDocument', 'offscreen.contexts',
                            // the downloads an extension asks for (S2b-13, D-375), and
                            // what it does to one of its own afterwards (D-405).
                            'downloads.expect', 'downloads.search',
                            // ('open' is not: it would run what was downloaded, with
                            //  none of Chrome's gates. R-224 high 1.)
                            'downloads.cancel', 'downloads.pause', 'downloads.resume', 'downloads.erase',
                            'downloads.show',
                            // which rulesets are enabled, which this Qt engine
                            // answers wrong or never (D-393).
                            'declarativeNetRequest.getEnabledRulesets', 'declarativeNetRequest.updateEnabledRulesets',
                            // and the rules it puts in itself, which it takes
                            // the application down with or waits on for ever.
                            'declarativeNetRequest.getDynamicRules', 'declarativeNetRequest.updateDynamicRules',
                            'declarativeNetRequest.getSessionRules', 'declarativeNetRequest.updateSessionRules',
                            // the user scripts, which the application keeps and puts
                            // in (D-443).
                            'userScripts.register', 'userScripts.unregister', 'userScripts.getScripts',
                            'userScripts.update',
                            // and the worlds' configuration (D-445).
                            'userScripts.configureWorld', 'userScripts.getWorldConfigurations',
                            'userScripts.resetWorldConfiguration'] : []);
  // and what WebView2 has of its own and answers for, which is asked of
  // nobody there (D-379 addition 2, decision 8) -- its rules among it, which
  // it evaluates itself (D-393): the menu -- E-1 mirrors
  // the engine's items into the application's own, so a second
  // registration would be those items twice over and the engine's
  // 'onClicked' would be hidden -- the downloads, and the offscreen
  // document -- and the notifications, the engine's own there (D-418b,
  // D-335). 'installContexts', 'installDownloads' and the menu's own
  // 'create' are each keyed on one of these names and go with them.
  if (EDGE) {
    const engines = [];
    hosted.forEach((call) => { if (/^(?:contextMenus|downloads|offscreen|declarativeNetRequest|commands|notifications)\./.test(call)) engines.push(call); });
    engines.forEach((call) => hosted.delete(call));
  }
  // the user scripts: on the Qt engine, and only for an extension whose
  // manifest asks for them, as in Chrome -- elsewhere there is no
  // namespace at all, and what asks whether it can register them (uBOL:
  // does 'getScripts' throw?) is told no (D-443). WebView2 gives the
  // application no user scripts of a profile to put them in.
  // The offscreen document likewise: an extension which does not ask for
  // it is asked nothing of it, and its 'runtime.getContexts' is the
  // engine's own -- the application refuses it 'offscreen.*', and the call
  // failed with that where Chrome answers (E, 2026-10-03).
  {
    let permissions = [];
    try {
      const manifest = chrome.runtime.getManifest();
      if (Array.isArray(manifest.permissions)) permissions = manifest.permissions;
    } catch (e) {}
    const unasked = [];
    hosted.forEach((call) => {
      if ((/^userScripts\./.test(call) && (EDGE || !permissions.includes('userScripts')))
          || (/^offscreen\./.test(call) && !permissions.includes('offscreen'))) unasked.push(call);
    });
    unasked.forEach((call) => hosted.delete(call));
  }
  // a Tab as the host gives one. Anything else -- a bare number, a thing
  // with no place -- is not a Tab and is no answer at all.
  // (a whole number, and nothing else: what an extension hands back is
  //  looked for by the same rule a port's numbers are matched by, and a
  //  fraction or an infinity is no tab of anybody's. R-142 low 3.)
  const isTab = (tab) => !!tab && typeof tab === 'object' && Number.isInteger(tab.id) && tab.id > 0
                         && Number.isInteger(tab.index) && tab.index >= 0;
  // where each tab the host has named sits, which is what the 'index' of a
  // 'sender.tab' is read from (R-138 mid 1). Nothing a page or a content
  // script says reaches this table: it is written from the host's own
  // answers and from nowhere else. The oldest goes when the room runs out,
  // as the worker's tabs do.
  const indexes = new Map(), INDEXES_KEPT = 4096;
  const note = (tab) => {
    if (!isTab(tab)) return;
    if (indexes.has(tab.id)) indexes.delete(tab.id);
    else if (indexes.size >= INDEXES_KEPT) indexes.delete(indexes.keys().next().value);
    indexes.set(tab.id, tab.index);
  };
  // every Tab an answer carries: one of them ('tabs.get', 'tabs.create',
  // 'tabs.update', 'vanilla.tabOf'), a list ('tabs.query'), or a window's
  // ('windows.getCurrent' and 'getAll' with 'populate'). Taken BEFORE the
  // answer is handed on, an extension being as likely as not to read
  // 'sender.tab.index' the moment 'tabs.query' answers (Vimium does).
  //
  // What the TREE answers with writes nothing here and is meant to: a
  // 'HistoryItem' and a 'BookmarkTreeNode' each name themselves with a
  // STRING (D-367 decision 4) and 'isTab' wants a whole number, so no node
  // of the tree is ever taken for a tab however much of a tab's shape it
  // has ('index' and all). A node's 'children' are not walked either: a
  // window's 'tabs' are the one list this goes down into.
  const noted = (value) => {
    for (const one of (Array.isArray(value) ? value : [value])) {
      note(one);
      if (one && Array.isArray(one.tabs)) for (const tab of one.tabs) note(tab);
    }
    return value;
  };
  // HOW A CALL REACHES THE HOST. On the Qt engine, and from a document on
  // either, it is the fetch below. From a WORKER of WebView2 it is not: a
  // service worker's fetch of the application's own scheme never reaches
  // the host there (measured: it fails in about two milliseconds), so what
  // is asked waits for a port to the relay page instead -- invariant 12 of
  // D-379. 'RELAYED' is set where the shim is put together, of 'EDGE' and
  // of which shim it is, and is read nowhere but here and in the events'
  // loop.
  //
  // The port is E-2b-1's to bring. Until it does, a call waits and ends on
  // the ten seconds a fetch of the host's was always given: an extension's
  // calls fail on this engine as they failed before, later rather than at
  // once. What waits is BOUNDED -- an extension which asks and asks with
  // nobody to answer is not to grow a list without end -- so the calls kept
  // are 'WAITING_MOST' and the next one fails where it is made, with the
  // refusal any call nobody answers ends in.
  const RELAY_DEADLINE = 10000, WAITING_MOST = 64;
  const waiting = new Map();
  let transport = null, tickets = 0;
  // how the worker asks for the relay when it has nothing to send a call
  // over: the port's own end puts it in (E-2b-1), and nobody does on the
  // Qt engine, where no call is ever relayed.
  let wantRelay = () => {};
  // what E-2b-1 plugs in: a 'send(ticket, call)' and an answer by ticket.
  // A call which has NOT gone over a transport yet goes over a new one at
  // once, in the order it was asked. One which HAS gone is left where it
  // is: it may have been done at the other end already, and is never sent
  // a second time (invariant 8 of D-378). A ticket nobody waits for
  // answers nothing.
  const setTransport = (send) => {
    transport = typeof send === 'function' ? send : null;
    if (!transport) return;
    const out = [];
    waiting.forEach((one, ticket) => { if (!one.sent) out.push([ticket, one]); });
    for (const [ticket, one] of out) { one.sent = true; try { transport(ticket, one.call); } catch (e) { one.settle(null); } }
  };
  const relayAnswer = (ticket, answer) => {
    const one = waiting.get(ticket);
    if (one) one.settle(answer);
  };
  // and the transport GOING: every call which was waiting fails NOW, sent
  // or not, and none of them is ever sent again (invariant 8 again -- one
  // which was out may have been done, and doing it twice is worse than
  // failing it once). A round of the events is one of them, and the loop
  // makes of that failure what it makes of a round which broke: it backs
  // off and is armed again, so the next transport begins it.
  const relayFail = () => {
    const lost = [];
    waiting.forEach((one) => lost.push(one));
    for (const one of lost) one.settle(null);
  };
  // and where nothing is relayed -- the Qt engine, and a document on
  // either -- there is no hook to plug anything into. Neither written over
  // nor walked to: it is the relay's own way in, not something an
  // extension is meant to find.
  if (RELAYED) try {
    Object.defineProperty(self, '__vanillaRelay',
                          { value: { attach: setTransport, answer: relayAnswer, fail: relayFail }, configurable: true });
  } catch (e) {}
  // one call over the relay: what the caller waits on, the way to give it
  // up without answering it (the events' loop stops a round that way), and
  // whether it ended on its own DEADLINE, which the events' loop reads and
  // nobody else does.
  const relayAsk = (call, deadline) => {
    let resolve = null, reject = null;
    const promise = new Promise((res, rej) => { resolve = res; reject = rej; });
    const unavailable = () => new Error('chrome.' + call.api + ' is not available in this browser');
    const handle = { promise, cancel: () => {}, expired: false };
    if (waiting.size >= WAITING_MOST) { reject(unavailable()); return handle; }
    const ticket = ++tickets;
    let timer = null, settled = false;
    const forget = () => {
      settled = true;
      waiting.delete(ticket);
      try { if (timer !== null && typeof clearTimeout === 'function') clearTimeout(timer); } catch (e) {}
      timer = null;
    };
    // an answer of the relay's is the answer the host's JSON is, and is
    // read the same way: a success is the value, the application's refusal
    // is its own words, and anything else -- the deadline among it -- is
    // the failure a stand-in has always answered with.
    const settle = (answer) => {
      if (settled) return;
      forget();
      if (answer && answer.ok === true) { resolve(noted(answer.value)); return; }
      reject(answer && typeof answer.error === 'string' ? new Error(answer.error) : unavailable());
    };
    const entry = { call, settle, sent: false };
    waiting.set(ticket, entry);
    // the deadline is the ten seconds a fetch of the host's always had,
    // except for the one call the host HOLDS: the events' round is given
    // the loop's own long deadline, and ending on that is another round
    // and no failure.
    const ms = Number.isInteger(deadline) && deadline > 0 ? deadline : RELAY_DEADLINE;
    try { timer = setTimeout(() => { handle.expired = true; settle(null); }, ms); } catch (e) {}
    if (transport) { entry.sent = true; try { transport(ticket, call); } catch (e) { settle(null); } }
    // with nothing to send it over, the relay is asked for: the page which
    // holds the port opens one when it hears (E-2b-1).
    else wantRelay();
    handle.cancel = () => { if (!settled) forget(); };
    return handle;
  };
  // the calls whose arguments go in the body, too big for the header: an
  // extension's rules (D-393b). The host knows them by the same names.
  const BODIED = new Set(['declarativeNetRequest.updateDynamicRules', 'declarativeNetRequest.updateSessionRules',
                          // the content scripts of the page's own world, by name (D-420).
                          'vanilla.mainScripts',
                          // and the user scripts, with their code (D-443, D-444), and
                          // a worker's answer to a user script's message (D-445).
                          'userScripts.register', 'userScripts.update', 'vanilla.userScriptReply']);
)js" R"js(
  // a call of the host's, fetched: the key and the call in the headers,
  // 'body' (a string) where there is one, 'signal' where there is one. A
  // fetch which throws is a promise which fails.
  const fetchHost = (api, args, body, signal) => {
    const options = { method: 'POST', cache: 'no-store', credentials: 'omit',
                      headers: { 'X-Vanilla-Key': HOST_KEY, 'X-Vanilla-Call': encodeURIComponent(JSON.stringify({ api, args })) } };
    if (body !== undefined) options.body = body;
    if (signal) options.signal = signal;
    try { return Promise.resolve(fetch('vanilla-extension://host/call', options)); }
    catch (e) { return Promise.reject(e); }
  };
  // 'deadline': the one call which waits on the user (D-437) is given
  // longer than the ten seconds everything else is.
  const ask = (api, args, deadline) => {
    const unavailable = () => new Error('chrome.' + api + ' is not available in this browser');
    if (RELAYED) return relayAsk({ api, args }, deadline).promise;
    let asking;
    try {
      const bodied = BODIED.has(api);
      let signal;
      if (typeof AbortSignal === 'function' && typeof AbortSignal.timeout === 'function') signal = AbortSignal.timeout(Number.isInteger(deadline) && deadline > 0 ? deadline : 10000);
      asking = fetchHost(api, bodied ? [] : args, bodied ? JSON.stringify(args) : undefined, signal);
    } catch (e) { return Promise.reject(unavailable()); }
    return asking.then((response) => response.json()).then(
      (answer) => {
        if (answer && answer.ok === true) return noted(answer.value);
        throw (answer && typeof answer.error === 'string' ? new Error(answer.error) : unavailable());
      },
      () => { throw unavailable(); });
  };
)js";

const char *EVENTS = R"js(
  // this shim's subscription, of the same 128 bits a document names itself
  // with: in a header, never in an address, and nowhere in the DOM. Where
  // it cannot be made -- as where there is no key at all -- those of
  // 'SOUNDED' are the stand-ins which never fire, as they were.
  const eventToken = (() => {
    try {
      const bytes = new Uint8Array(16);
      crypto.getRandomValues(bytes);
      return Array.from(bytes, (b) => (b < 16 ? '0' : '') + b.toString(16)).join('');
    } catch (e) { return null; }
  })();
  // the third set 'ours' is read from, beside 'hosted' and 'OWN', and the
  // only one whose being ours depends on the key: with one, these are ours
  // whatever the engine may one day have of those names; without one,
  // nobody has taken them from the engine or from the stand-ins.
  const SOUNDED = new Set(hosted.size && eventToken
                          ? ['tabs.onCreated', 'tabs.onRemoved', 'tabs.onUpdated', 'tabs.onActivated',
                             // where a tab went, what is highlighted, a zoom (D-415).
                             'tabs.onMoved', 'tabs.onHighlighted', 'tabs.onZoomChange',
                             'windows.onFocusChanged',
                             // told once each, from the button and the menu (D-374).
                             'action.onClicked', 'contextMenus.onClicked',
                             // and of a download the extension asked for (D-375),
                             // and erased (D-405).
                             'downloads.onCreated', 'downloads.onChanged', 'downloads.onErased',
                             // and the visits, which the host tells of from the
                             // same difference the tab events come from (D-403).
                             'history.onVisited', 'history.onVisitRemoved',
                             // and its shortcuts pressed (D-416).
                             'commands.onCommand',
                             // the side panel's (D-440).
                             'sidePanel.onOpened', 'sidePanel.onClosed',
                             // a notification's dialog answered or gone (D-418b).
                             'notifications.onClicked', 'notifications.onClosed'] : []);
  // the events of what WebView2 answers for are the engine's own there
  // (D-379 addition 2, decision 8): a stand-in over one of those names
  // would hide an event which fires. (A worker there stands in front of
  // the menu's all the same, to hand over a tab of the application's: it
  // is 'OWN' and not this, and it hears the engine's event. D-385.)
  if (EDGE) ['contextMenus.onClicked', 'downloads.onCreated', 'downloads.onChanged', 'downloads.onErased',
            'commands.onCommand', 'notifications.onClicked', 'notifications.onClosed'].forEach((name) => SOUNDED.delete(name));
  // one listener which throws is that listener's failure and no bar to
  // the rest, as it is of the engine's own events.
  const tell = (to, args) => { for (const f of to) { try { f.apply(undefined, args); } catch (e) { try { console.error(e); } catch (x) {} } } };
  // 'name' -> a function of the event's arguments which says whether to
  // hold it back (a piece below sets one: the downloads', D-375); whoever
  // holds one hands it over later, to the listeners of that moment,
  // through 'handOver' (which the install below makes, of its listeners).
  const holders = new Map();
  let handOver = () => {};
  // ('own' -- the map the calls the shims answer themselves are read from --
  //  is handed in rather than closed over: this piece stands above the
  //  stand-ins, which is where that map is declared.)
  const installEvents = (own) => {
    if (!SOUNDED.size) return;
    // the shim's own deadline. It is long because the answer is: a call
    // which ends on it has heard nothing, which is not the same as a host
    // which cannot be reached, so it is another round and is not counted.
    // For the same reason a call which was HELD a good while and then broke
    // is not counted either (the content side's 'lost' keeps that rule).
    const DEADLINE = 120000, HELD_LONG = 25000;
    const BACKOFF = [1000, 2000, 4000, 8000, 16000], TRIES = 5;
    const now = () => { try { return Date.now(); } catch (e) { return 0; } };
    const listeners = new Map();
    SOUNDED.forEach((name) => listeners.set(name, []));
    const anybody = () => { let any = false; listeners.forEach((list) => { if (list.length) any = true; }); return any; };
    // the names of the events with a listener now: said with every call, so
    // that the host wakes this worker, when it sleeps, for those and no
    // others (D-451). 'said' is what the host was told last, and 'told' its
    // number: each new list is numbered after the last, and the host takes
    // none older than what it has (calls may pass each other: R-295 mid).
    const heard = () => { const out = []; listeners.forEach((list, name) => { if (list.length) out.push(name); }); return out; };
    let said = null, telling = false, told = 0;
    // ONE round at a time, and one timer with it: 'gen' says which round is
    // the round running, so that an answer, a deadline or a failure of an
    // older one settles nothing. 'mark' is the same for the timer, for a
    // world where 'clearTimeout' is not there or does not take.
    let going = false, gen = 0, failures = 0, warned = false;
    let timer = null, mark = 0, stopper = null;
    const disarm = () => {
      try { if (timer !== null && typeof clearTimeout === 'function') clearTimeout(timer); } catch (e) {}
      timer = null;
      mark++;
    };
    const arm = (ms, what) => {
      disarm();
      const mine = mark;
      timer = setTimeout(() => { if (mine !== mark) return; timer = null; what(); }, ms);
    };
    // the loop stops. Whatever is on its way settles nothing, the timer is
    // taken back, and the call which is out is given up on where this world
    // can give one up -- where it cannot, its answer is simply not handed
    // out and nothing is asked after it.
    const halt = () => {
      going = false;
      gen++;
      disarm();
      const out = stopper;
      stopper = null;
      if (out) { try { out.abort(); } catch (e) {} }
    };
    // where every tab sits now, written OVER and not added to: a tab which
    // is not in the order is not in the table either, and an extension
    // which reads 'index' of it reads 0, as it does of a tab nobody ever
    // named.
    //
    // Only an order which IS one is written down: every entry a tab's
    // number, and none of them twice. One with a hole in it, or with the
    // same tab in two places, is no order the host gave, and reading places
    // off it would put a tab where nobody said it was (R-145 low 2) -- so
    // the table is left exactly as it was, all of it or none.
    //
    // The head of a long order is what is looked at and what is kept, the
    // table being bounded: a place there is no room for is no worse than one
    // the table never had. (Which is why what lies beyond the head is
    // neither read nor answered for: nothing of it is written down.)
    const place = (order) => {
      const kept = Math.min(order.length, INDEXES_KEPT);
      const seen = new Set();
      for (let i = 0; i < kept; i++) {
        if (!Number.isInteger(order[i]) || order[i] <= 0 || seen.has(order[i])) return;
        seen.add(order[i]);
      }
      indexes.clear();
      for (let i = 0; i < kept; i++) indexes.set(order[i], i);
    };
    const deliver = (value) => {
      // the table first, so that a listener which reads 'sender.tab.index'
      // of a tab it was handed earlier reads where that tab is NOW
      // (R-138 mid 1). An answer with no order -- 'stale' is the only one --
      // does not touch the table; one with the order and NO events is what
      // the host says when only where the tabs sit has changed, and is an
      // answer like any other (the next call goes at once).
      if (Array.isArray(value.order)) place(value.order);
      if (!Array.isArray(value.events)) return;
      // the listeners of the moment the answer came, all of them at once: one
      // which an earlier event's listener adds does not hear of the later
      // events of the same answer, and one it removes still does (the rule
      // 'demux' keeps for a message).
      const snapshot = new Map();
      listeners.forEach((list, name) => snapshot.set(name, list.slice()));
      for (const one of value.events) {
        // an event of no name of ours, or one whose arguments are not a
        // list to hand over, is nothing this can speak: it is dropped, not
        // guessed at.
        if (!one || typeof one !== 'object' || typeof one.name !== 'string' || !Array.isArray(one.args)) continue;
        const to = snapshot.get(one.name);
        if (!to) continue;
        // one a piece of this shim asked to hold (a download's, before the
        // call which asked for it has answered: D-375) is kept for it.
        const holder = holders.get(one.name);
        if (holder && holder(one.args)) continue;
        tell(to, one.args);
      }
    };
    const broke = (mine, began) => {
      if (mine !== gen) return;
      disarm();
      stopper = null;
      if (now() - began >= HELD_LONG) { round(); return; }
      failures++;
      if (failures > TRIES) {
        going = false;
        if (!warned) {
          warned = true;
          try { console.warn('Vanilla: the compatibility layer could not ask this browser what happens to the tabs; this extension will not be told of tabs opening, closing or changing'); } catch (e) {}
        }
        return;
      }
      arm(BACKOFF[failures - 1], round);
    };
)js" R"js(
    function round(){
      if (!anybody()) { halt(); return; }
      going = true;
      const mine = ++gen, began = now();
      // a worker of WebView2 reaches the host over the relay and never by
      // fetch (invariant 12 of D-379), this call with every other. What
      // ends it is that call's own deadline, which is a call which broke
      // and another try; the long deadline below is a HELD fetch's and is
      // nothing here.
      if (RELAYED) {
        // the events are the one call the host HOLDS, so this one is given
        // the loop's own long deadline and not the ten seconds a call
        // which is answered where it arrives gets. Ending on it has heard
        // nothing, which is another round and is not counted -- the very
        // rule the fetch below keeps.
        const out = relayAsk({ api: 'vanilla.events', args: [eventToken, heard()] }, DEADLINE);
        stopper = { abort: out.cancel };
        out.promise.then(
          (value) => {
            if (mine !== gen) return;
            disarm();
            stopper = null;
            if (!value || typeof value !== 'object') { broke(mine, began); return; }
            // a round the RELAY gave up on at its end: the port went, and
            // the host was told to let this call go rather than hold an
            // answer nobody will ever read (D-379 decision 10). The
            // subscription is the same one, so this is a round which BROKE
            // -- backed off and armed again -- and never 'stale', which
            // would stop the loop for good.
            if (value.aborted === true) { broke(mine, began); return; }
            if (value.stale === true) { halt(); return; }
            failures = 0;
            warned = false;
            deliver(value);
            if (mine !== gen) return;
            round();
          },
          () => { if (mine !== gen) return; stopper = null; if (out.expired) { round(); return; } broke(mine, began); });
        return;
      }
      let expired = false;
      const names = heard(), listed = JSON.stringify(names);
      if (listed !== said) { said = listed; told++; }
      // a controller where there is one, because the loop must be able to
      // stop this call AND to tell its own deadline from a call which
      // broke, which an 'AbortSignal.timeout' does not let it do. With only
      // the timeout, the deadline is the signal's; with neither, the call
      // is left to the host and the answer to an older round is dropped.
      let signal;
      stopper = null;
      if (typeof AbortController === 'function') {
        try { stopper = new AbortController(); signal = stopper.signal; } catch (e) { stopper = null; }
      }
      if (stopper)
        arm(DEADLINE, () => {
          if (mine !== gen) return;
          expired = true;
          const out = stopper;
          stopper = null;
          if (out) { try { out.abort(); } catch (e) {} }
        });
      else if (typeof AbortSignal === 'function' && typeof AbortSignal.timeout === 'function') {
        try { signal = AbortSignal.timeout(DEADLINE); } catch (e) {}
      }
      fetchHost('vanilla.events', [eventToken, names, told], undefined, signal).then((response) => response.json()).then(
        (answer) => {
          if (mine !== gen) return;
          disarm();
          stopper = null;
          if (!answer || answer.ok !== true || !answer.value || typeof answer.value !== 'object') { broke(mine, began); return; }
          // a call a newer one of the same subscription took the place of.
          // There is another loop to be answered, so this one ENDS here: it
          // hands nothing out, counts nothing, and does not ask again.
          if (answer.value.stale === true) { halt(); return; }
          failures = 0;
          warned = false;
          deliver(answer.value);
          // (the last listener may have gone while it was being handed to:
          //  then the loop stopped, and 'gen' says so.)
          if (mine !== gen) return;
          round();
        },
        () => { if (mine !== gen) return; if (expired) { round(); return; } broke(mine, began); });
    }
    // the first listener of any of them starts the loop, and one added
    // after it gave up starts it again with the count back: an extension
    // which listens anew is not to be left with events which never come.
    const begin = () => {
      if (going) return;
      failures = 0;
      warned = false;
      round();
    };
    // the listeners changed after the host was told of them -- as those a
    // worker adds at its top after the first: it is told again, once what
    // runs now is done, in a call of its own (the call which is held may be
    // held a long while) -- the last one gone with it, as none. Not over
    // the relay: a worker there is not woken (D-451).
    //
    // One which does not get through is sent again a second later, three
    // times at most, while it is still the newest (R-295 mid).
    const send = (names, mine, left) => {
      fetchHost('vanilla.eventNames', [eventToken, names, mine]).then(undefined, () => {
        if (mine !== told || left <= 0) return;
        try { setTimeout(() => { if (mine === told) send(names, mine, left - 1); }, 1000); } catch (e) {}
      });
    };
    const retell = () => {
      if (RELAYED || said === null || telling) return;
      telling = true;
      Promise.resolve().then(() => {
        telling = false;
        const names = heard(), now = JSON.stringify(names);
        if (now === said) return;
        said = now;
        send(names, ++told, 3);
      });
    };
    handOver = (name, args) => { const to = listeners.get(name); if (to) tell(to.slice(), args); };
    SOUNDED.forEach((name) => {
      const list = listeners.get(name);
      own.set(name, {
        // whatever else is handed over -- a filter, which these events take
        // none of in Chrome either -- is taken and not read: an extension
        // which hands one over is not to fail here.
        addListener(f) {
          if (typeof f !== 'function' || list.includes(f)) return;
          list.push(f);
          begin();
          retell();
        },
        removeListener(f) {
          const at = list.indexOf(f);
          if (at < 0) return;
          list.splice(at, 1);
          if (!anybody()) halt();
          retell();
        },
        hasListener(f) { return list.includes(f); },
        hasListeners() { return list.length > 0; },
        // (the stand-in has these, so an extension which calls one of them
        //  is not failed by the event's becoming real. No rule is kept.)
        addRules() {}, removeRules() {}, getRules() {},
      });
    });
  };
)js";

const char *STAND_INS = R"js(
  // What the shims answer of their own, asking nobody: the calls which are
  // carried by the ports a content script's shim opens (S2b-5a, R-140).
  // 'installEnvelope' owns those ports and fills 'own'; the names are known
  // HERE because the stand-ins go in first, and because a namespace of
  // which something is ours is to be stood in for even where the engine has
  // one of that name (the engine's 'chrome.webNavigation', the day it has
  // one, knows nothing of these ports). They are ours with a key or
  // without: nobody is asked for them.
  //
  // The two 'webNavigation' events are here for the same reason (S2b-5c,
  // R-147): what fires them is a document telling of its own navigation
  // over its port, so the key has nothing to do with them, and so is
  // 'onCommitted' (a document's first link: D-426), 'onDOMContentLoaded'
  // and 'onCompleted' (what it tells of its loading) and 'onTabReplaced'
  // (kept and never fired: D-452). The others of that namespace stay the
  // stand-in which never fires.
  const GRANTED = ['permissions.getAll', 'permissions.contains', 'permissions.request'];
  const OWN = new Set(['tabs.sendMessage', 'webNavigation.getAllFrames',
                       'webNavigation.onHistoryStateUpdated', 'webNavigation.onReferenceFragmentUpdated',
                       'webNavigation.onCommitted', 'webNavigation.onDOMContentLoaded',
                       'webNavigation.onCompleted', 'webNavigation.onTabReplaced',
                       ...(EDGE ? [] : GRANTED)]);
  // ('scripting.executeScript' joins where the manifest names the
  //  permission, as the envelope goes in: S2b-14, D-376. And
  //  'contextMenus.onClicked' in a worker of WebView2, before the stand-ins
  //  go in: 'installMenuTab', D-385. There the engine's own
  //  'scripting.executeScript', 'insertCSS' and 'removeCSS' join instead,
  //  the tab looked up again: 'installScriptingTab', D-386.)
  const own = new Map();
  // a namespace of ours whole: each of its members, and the namespace told
  // of as made (the identity's, the alarms', the idle's).
  const ownAll = (ns, members) => {
    for (const key of Object.keys(members)) {
      OWN.add(ns + '.' + key);
      own.set(ns + '.' + key, members[key]);
    }
    made.push(ns);
  };
  const installStandIns = () => {
    const event = () => ({ addListener() {}, removeListener() {},
                           hasListener() { return false; }, hasListeners() { return false; },
                           addRules() {}, removeRules() {}, getRules() {} });
    // a name all in capitals is a CONSTANT: a number the engine would have
    // given, which is never made up here (R-127) and is the one thing read
    // off the engine inside a namespace stood in for.
    const CONSTANT = /^[A-Z]+(_[A-Z0-9]+)*$/;
    const known = (key) => /^[a-zA-Z]/.test(key) && !CONSTANT.test(key) && key !== 'then' && key !== 'toJSON';
    // the numbers of a namespace which are Chrome's own, as its
    // documentation gives them: a name all in capitals is a number the
    // engine would have given, and made up it is a made-up answer (R-127)
    // -- but these three are the same in every Chrome, and what hears
    // 'windows.onFocusChanged' compares what it is handed with
    // 'WINDOW_ID_NONE' (Vimium's 'tab_recency', Stands' 'window.js'). Read
    // only where the namespace is stood in for at all, and only after the
    // engine's own: 'chrome.tabs.TAB_ID_NONE' is not here because the
    // engine answers that one itself.
    //
    // 'MAX_SESSION_RESULTS' is the most 'sessions.getRecentlyClosed' would
    // ever hand back and is 25 in every Chrome; Vimium reads
    // 'chrome.sessions?.MAX_SESSION_RESULTS || 25' as it builds its
    // commands (D-371). It says nothing of how much the application's own
    // trash holds, which is a setting of the user's, and there is no
    // 'getRecentlyClosed' here to hand back that many of anything.
    const CONSTANTS = { windows: { WINDOW_ID_NONE: -1, WINDOW_ID_CURRENT: -2 },
                        sessions: { MAX_SESSION_RESULTS: 25 } };
    const fixed = (name, key) => {
      const table = CONSTANTS[name];
      return table && typeof key === 'string' && Object.prototype.hasOwnProperty.call(table, key) ? table : null;
    };
    // The namespaces nothing of the engine's is passed through in: the
    // ones whose members take or hand back ids of the ENGINE's own -- the
    // tab and window ids, and the bookmark, history and session ids of its
    // own store, which are not the application's either. Edge has a
    // 'chrome.tabs' and a 'chrome.windows' of WebView2's, so 'tabs.setZoom'
    // or 'windows.create' passed through would be handed the application's
    // id and land on another tab or fail silently; 'search.query' takes a
    // 'tabId', 'sessions.restore' a session id of the engine's and
    // 'webNavigation.getFrame' a tab and a frame of its own
    // (D-378 addition 1, decision 5).
    //
    // Everywhere else a namespace is stood in for -- 'i18n', 'action',
    // 'contextMenus', 'downloads', 'offscreen' -- what the engine has and
    // the shims do not is the engine's to answer, as it was before: there
    // is no id of ours in it to be taken for one of the engine's.
)js" R"js(
    const IDS = new Set(['tabs', 'windows', 'webNavigation', 'bookmarks', 'history', 'sessions', 'search']);
    const standIn = (name, real) => {
      const kept = new Map();
      return new Proxy({}, {
        get(target, key) {
          // what the extension itself put on the namespace is read back, as
          // from Chrome's own object: uBOL's pages add 'chrome.i18n.render'
          // and call it (D-394). The object behind the stand-in is nobody's
          // but this extension's, so this hands nobody anything.
          if (Object.prototype.hasOwnProperty.call(target, key)) return target[key];
          // a CONSTANT is the engine's to give wherever the engine has it:
          // a number, and no id of ours (R-127).
          if (typeof key === 'string' && CONSTANT.test(key) && real && key in real) return real[key];
          const call = typeof key === 'string' ? name + '.' + key : '';
          // what the application answers for is asked of IT, even where the
          // engine has a member of that name: this engine's own
          // 'chrome.tabs.update' is there and fails every call it is given
          // ('The specified target is not found.', measured 2026-09-21).
          // What is neither ours nor in a namespace of 'IDS' stays the
          // engine's, called on the engine's own object. Inside 'IDS' it is
          // answered here instead, member by member -- the stand-in which
          // fails rather than a call of the engine's given an id which is
          // not its own (D-378 addition 1, decision 5).
          const ours = hosted.has(call) || OWN.has(call) || SOUNDED.has(call);
          if (!ours && !REFUSED.has(call) && !IDS.has(name) && real && key in real) {
            const value = real[key];
            return typeof value === 'function' ? value.bind(real) : value;
          }
          // what the shims answer themselves is theirs, and is not kept
          // beside: 'own' is filled by 'installEnvelope', which runs after
          // this goes in, and is read here at the extension's first call.
          // (Were 'installEnvelope' to fail, the call fails the way any
          // other one nobody answers for does.)
          if (own.has(call)) return own.get(call);
          const table = fixed(name, key);
          if (table) return table[key];
          if (typeof key !== 'string' || !known(key)) return undefined;
          if (kept.has(key)) return kept.get(key);
          let value;
          if (hosted.has(call)) value = (...args) => {
            const callback = typeof args[args.length - 1] === 'function' ? args.pop() : null;
            const answer = ask(name + '.' + key, args);
            return callback ? toCallback(answer, callback) : answer;
          };
          else if (/^on[A-Z]/.test(key)) value = event();
          else if (/^[A-Z][a-z]/.test(key)) value = Object.freeze({});
          else value = (...args) => {
            const error = new Error('chrome.' + name + '.' + key + ' is not available in this browser');
            const callback = typeof args[args.length - 1] === 'function' ? args[args.length - 1] : null;
            if (callback) { failing(error, callback); return undefined; }
            return Promise.reject(error);
          };
          kept.set(key, value);
          return value;
        },
        has(target, key) {
          return Object.prototype.hasOwnProperty.call(target, key)
              || !!(real && key in real) || !!fixed(name, key) || (typeof key === 'string' && known(key));
        }
      });
    };
    // Standing in front of a root so that the engine building a namespace
    // again does not take the stand-ins back: what this keeps, what it
    // leaves to the object, and what was measured are above the string.
    // 'root' is the name of the binding and 'object' what it holds --
    // 'chrome', and since S2b-14 the engine's own 'browser' beside it.
    //
    // 'ours' is what the loop below put in and nothing else: 'made' is no
    // use here, a page's shim having pushed 'extension' onto it before any
    // of this ran. What 'installStorage' and 'installEnvelope' write later
    // goes on 'chrome.runtime' and 'chrome.storage', which are the
    // object's and are read through.
    const keepStandIns = (root, object, ours) => {
      if (typeof Proxy !== 'function') return false;
      let holder = null;
      try { holder = self; } catch (e) { return false; }
      if (!holder) return false;
      let was = null;
      try { was = Object.getOwnPropertyDescriptor(holder, root); } catch (e) { return false; }
      if (!was || !('value' in was) || was.value !== object) return false;
      const proxy = new Proxy(object, {
        get(target, key) {
          if (ours.has(key)) {
            // save where the engine has locked that name down. A proxy may
            // not answer anything but the object's own value for a property
            // which is neither writable nor configurable, and a TypeError
            // out of every read of that root would be worse than losing
            // the stand-in for one name.
            const there = Object.getOwnPropertyDescriptor(target, key);
            if (!there || there.configurable || there.writable) return ours.get(key);
          }
          const value = target[key];
          return typeof value === 'function' ? value.bind(target) : value;
        }
      });
      // only the value is given, so whatever the binding's own attributes
      // are -- and a 'var chrome' at a global's top is writable but not
      // configurable -- they stay what they were. Where it cannot be
      // written at all, nothing happens and the stand-ins are as they were
      // before this: the engine's to take back.
      try { Object.defineProperty(holder, root, { value: proxy }); } catch (e) { return false; }
      return holder[root] === proxy;
    };
    const names = ['tabs', 'windows', 'webNavigation', 'scripting', 'declarativeNetRequest', 'contextMenus',
                   'alarms', 'notifications', 'action', 'bookmarks', 'history', 'sessions', 'search',
                   'commands', 'permissions', 'cookies', 'webRequest', 'idle', 'downloads', 'topSites',
                   'offscreen', 'tabGroups', 'sidePanel', 'favicon',
                   // the fonts (D-404): one call the application answers.
                   'fontSettings'];
    // the namespaces the application answers something for, and those the
    // shims answer something of themselves. ('SOUNDED' adds none: it has
    // members only where 'hosted' does, and 'tabs' and 'windows' are both
    // in there already.) Since S2b-6 'bookmarks' and 'history' are in here
    // too wherever there is a key, one call of each being the
    // application's; since S2b-7 'search' is, for 'search.query' (D-369);
    // since S2b-9 'sessions' is, for 'sessions.restore' alone (D-371).
    // Everything else of those four stays the stand-in which fails rather
    // than pretend, their events included (D-367 decision 8) --
    // 'sessions.getRecentlyClosed' among them, the application naming
    // nothing of what its trash holds, so that there is nothing for a
    // 'restore' to be asked for by name -- and the engine building a real
    // 'chrome.search' for an extension whose manifest asked for 'search'
    // (Vimium's does) does not take the stand-in back: what is read
    // through the binding is ours (D-368).
    const spoken = new Set(), ours = new Map();
    hosted.forEach((call) => spoken.add(call.slice(0, call.indexOf('.'))));
    OWN.forEach((call) => spoken.add(call.slice(0, call.indexOf('.'))));
    // and what the engine must not be handed is only held back where the
    // stand-in is in front: 'REFUSED' puts it there too.
    REFUSED.forEach((call) => spoken.add(call.slice(0, call.indexOf('.'))));
    // and a namespace something of ours is in is stood in for whether or
    // not it is in the list: 'i18n' is, and only where the copy wrote the
    // messages ('installMessages'), the engine's own being what it was
    // everywhere else.
    spoken.forEach((name) => { if (!names.includes(name)) names.push(name); });
    for (const name of names) {
      let real = null;
      try { real = chrome[name] || null; } catch (e) {}
      // what the engine has is left alone, save where something of that
      // namespace is not the engine's to answer: 'tabs', which it has two
      // members of, and the namespaces of 'hosted' and of 'OWN'. The
      // engine has no 'chrome.windows' today; the day it does, what is
      // asked of the application is still asked of it. 'tabs' and
      // 'webNavigation' are stood in for with no key too: what the ports
      // answer is asked of nobody.
      if (real && name !== 'tabs' && !spoken.has(name)) continue;
      try { const value = standIn(name, real);
            Object.defineProperty(chrome, name, { value, configurable: true, writable: true, enumerable: true });
            ours.set(name, value); made.push(name); }
      catch (e) { refused.push(name); }
    }
    // and the binding is taken over last, so that what it hands back is
    // every stand-in which went in (D-368). A failure here is no failure of
    // the shim's: the stand-ins are in the object either way.
    if (keepStandIns('chrome', chrome, ours)) made.push('chrome');
    // The SECOND root (S2b-14, R-186(b)). Since Chromium 148 the engine has
    // 'browser' beside 'chrome' -- the WebExtensions name, native now --
    // and it is another object whose namespaces are lazy accessors of their
    // own: measured on WebView2 runtime 153, 'browser.tabs !== chrome.tabs'
    // and 'browser.tabs.query({ active: true, currentWindow: true })'
    // answered with the id of the ENGINE's tab where 'chrome.tabs.query'
    // answered with the application's. So an extension which speaks
    // 'browser.*' -- or one carrying the webextension polyfill, which makes
    // itself a no-op wherever a native 'browser' is there -- would go round
    // the stand-ins into the engine's own ids, which is the one thing the
    // stand-ins are for (D-378 addition 1, decision 5).
    //
    // The SAME stand-in objects go there, so that the two roots are one
    // answer and not two: what an extension keeps off 'chrome.tabs' and
    // what it keeps off 'browser.tabs' are the same object, listeners and
    // all. What is not stood in for -- 'storage', 'runtime', 'scripting' --
    // stays the engine's on both roots, and a CONSTANT is read off the
    // engine as before. Where there is no native 'browser' nothing is made:
    // the polyfill builds its own out of 'chrome', which is the stand-ins
    // by then.
    let other = null;
    try { const root = self.browser; if (root && typeof root === 'object') other = root; } catch (e) {}
    if (other) {
      // written over and never assigned to: the engine's member is a lazy
      // accessor, and this is strict code where its setter -- or the want
      // of one -- would throw.
      ours.forEach((value, name) => {
        try { Object.defineProperty(other, name, { value, configurable: true, writable: true, enumerable: true }); }
        catch (e) { refused.push('browser.' + name); }
      });
      if (keepStandIns('browser', other, ours)) made.push('browser');
    }
  };
)js";

const char *GRANTS = R"js(
  // What of the engine's is never passed through, for what it does
  // once called: in this Qt engine 'declarativeNetRequest.updateSessionRules'
  // takes the whole application down, and 'updateDynamicRules' never
  // answers (measured 2026-09-23, uBOL's pages). The stand-in fails
  // instead, as for a call nobody answers. WebView2's are its own and
  // evaluate the rules themselves: there nothing is held back.
  // And on either engine, what of 'management' changes anything rather
  // than reads (D-450a): the engine's 'setEnabled' stops the engine's copy
  // while the settings still say it is on (measured 2026-09-27, both
  // engines), and which extensions are on, and which are there at all, is
  // the settings' to say; the apps' calls would make shortcuts or install
  // things of the engine's own, which is nobody's here. What reads -- 'get',
  // 'getAll', 'getSelf', the two 'getPermissionWarnings' -- and the events
  // stay the engine's.
  const REFUSED = new Set((EDGE ? [] : ['declarativeNetRequest.updateSessionRules',
                                        'declarativeNetRequest.updateDynamicRules'])
                          .concat(['setEnabled', 'uninstall', 'uninstallSelf', 'launchApp', 'setLaunchType',
                                   'createAppShortcut', 'generateAppForLink', 'installReplacementWebApp']
                                  .map((key) => 'management.' + key)));
  // (read when the stand-ins go in, which is after this piece.)

  // What the manifest grants, answered out of the manifest where this Qt
  // engine has no 'permissions.getAll' or 'contains' (measured 2026-09-23):
  // the permissions it names and the hosts it asks for -- its
  // 'host_permissions' and the addresses of its content scripts, as Chrome
  // counts them. Optional ones never: nothing here grants them. And where
  // the engine has no 'scripting' at all, 'getRegisteredContentScripts'
  // answers that there are none, which is so: 'registerContentScripts' is
  // the stand-in which fails. (An engine's own 'scripting' is left alone,
  // as it is for 'executeScript'.) Both failing took uBOL's start with
  // them, into a reload (D-392).
  if (!EDGE) {
    const granted = () => {
      let manifest = null;
      try { manifest = chrome.runtime.getManifest(); } catch (e) {}
      manifest = manifest || {};
      const origins = [];
      const add = (list) => { if (Array.isArray(list)) list.forEach((o) => { if (typeof o === 'string' && !origins.includes(o)) origins.push(o); }); };
      add(manifest.host_permissions);
      if (Array.isArray(manifest.content_scripts)) manifest.content_scripts.forEach((cs) => add(cs && cs.matches));
      const permissions = Array.isArray(manifest.permissions) ? manifest.permissions.filter((p) => typeof p === 'string') : [];
      return { permissions, origins };
    };
    const answering = (compute) => (...args) => {
      const callback = typeof args[args.length - 1] === 'function' ? args.pop() : null;
      let answer;
      try { answer = Promise.resolve(compute(...args)); } catch (e) { answer = Promise.reject(e); }
      return callback ? toCallback(answer, callback) : answer;
    };
    // an address is held when it is asked for as it was granted, or when
    // what was granted is every address: no more is worked out, and what
    // this cannot say is answered no.
    const covers = (origins, origin) => origins.includes(origin) || origins.includes('<all_urls>')
      || (origins.includes('*://*/*') && /^(\*|https?):\/\//.test(origin));
    own.set('permissions.getAll', answering(() => granted()));
    own.set('permissions.contains', answering((asked) => {
      if (!asked || typeof asked !== 'object') throw new TypeError('permissions.contains takes an object of permissions and origins');
      const held = granted();
      return (Array.isArray(asked.permissions) ? asked.permissions : []).every((p) => held.permissions.includes(p))
          && (Array.isArray(asked.origins) ? asked.origins : []).every((o) => covers(held.origins, o));
    }));
    // 'permissions.request' (D-417): what the manifest grants already is
    // granted again at once, as Chrome grants it with no prompt -- uBOL
    // asks for the addresses it holds before it raises a site's filtering.
    // What only 'optional_permissions' names -- or an address not granted --
    // is not granted: nothing here asks the user and keeps what they allowed,
    // so the answer is the one Chrome gives a user who declines, 'false'.
    // What neither names is refused in Chrome's words.
    let toldOptional = false;
    own.set('permissions.request', answering((asked) => {
      if (!asked || typeof asked !== 'object') throw new TypeError('permissions.request takes an object of permissions and origins');
      const held = granted();
      let manifest = null;
      try { manifest = chrome.runtime.getManifest(); } catch (e) {}
      manifest = manifest || {};
      const strings = (list) => Array.isArray(list) ? list.filter((s) => typeof s === 'string') : [];
      const optionalPermissions = strings(manifest.optional_permissions);
      let optional = false;
      for (const p of strings(asked.permissions)) {
        if (held.permissions.includes(p)) continue;
        if (!optionalPermissions.includes(p)) throw new Error('Only permissions specified in the manifest may be requested.');
        optional = true;
      }
      // (an address is only told held or optional where 'covers' can say
      //  so; one it cannot -- a narrower pattern inside a granted one,
      //  which Chrome counts as granted -- is not refused as unlisted but
      //  declined, as 'contains' answers it no. R-239 part 2 mid 1.)
      for (const o of strings(asked.origins)) {
        if (covers(held.origins, o)) continue;
        optional = true;
      }
      if (!optional) return true;
      if (!toldOptional) {
        toldOptional = true;
        try { console.warn('Vanilla: the compatibility layer cannot grant an optional permission; it answers as if you declined'); } catch (e) {}
      }
      return false;
    }));
    let enginesScripting = null;
    try { enginesScripting = chrome.scripting || null; } catch (e) {}
    // (a namespace the engine has not got is stood in for whatever is in
    //  'OWN', and what 'own' holds is what the stand-in hands out.)
    if (!enginesScripting) own.set('scripting.getRegisteredContentScripts', answering(() => []));
  }
)js";

const char *ACTION = R"js(
  const installAction = () => {
    if (!hosted.has('action.setIcon')) return;
    const encode = (bytes) => {
      const b = new Uint8Array(bytes);
      let s = '';
      for (let i = 0; i < b.length; i += 0x2000) s += String.fromCharCode.apply(null, b.subarray(i, i + 0x2000));
      return btoa(s);
    };
    const isImage = (v) => !!v && typeof v === 'object' && typeof v.width === 'number' && typeof v.height === 'number' && !!v.data;
    const pick = (data) => {
      if (isImage(data)) return data;
      if (!data || typeof data !== 'object') return null;
      let best = 0, chosen = null;
      for (const key of Object.keys(data)) {
        const n = Number(key);
        if (Number.isInteger(n) && n > best && n <= 64 && isImage(data[key])) { best = n; chosen = data[key]; }
      }
      return chosen;
    };
    const png = (image) => {
      if (typeof OffscreenCanvas === 'function') {
        const canvas = new OffscreenCanvas(image.width, image.height);
        canvas.getContext('2d').putImageData(image, 0, 0);
        return canvas.convertToBlob({ type: 'image/png' }).then((blob) => blob.arrayBuffer()).then(encode);
      }
      if (typeof document === 'object' && document && typeof document.createElement === 'function') {
        const canvas = document.createElement('canvas');
        canvas.width = image.width; canvas.height = image.height;
        canvas.getContext('2d').putImageData(image, 0, 0);
        const url = canvas.toDataURL('image/png');
        return Promise.resolve(url.slice(url.indexOf(',') + 1));
      }
      return Promise.reject(new Error('chrome.action.setIcon is not available in this browser'));
    };
    // a path as Chrome reads one: against the address of the script which
    // asks ('../icons/x.png' from 'background_scripts/', Vimium), and from
    // the root once resolved. One which resolves to another origin is left
    // as it was: the application refuses what is not inside the extension.
    const resolve = (path) => {
      if (typeof path !== 'string') return path;
      try {
        const url = new URL(path, self.location.href);
        if (url.origin !== self.location.origin) return path;
        return decodeURIComponent(url.pathname.replace(/^\/+/, ''));
      } catch (e) { return path; }
    };
    const resolved = (path) => {
      if (typeof path === 'string') return resolve(path);
      if (!path || typeof path !== 'object') return path;
      const out = {};
      for (const key of Object.keys(path)) out[key] = resolve(path[key]);
      return out;
    };
    // what goes in the header, as it will go: a call longer than the
    // application reads ('CALL_LIMIT', 16 KiB) is refused here, before it
    // is sent, in the words of a call which was refused (R-169 mid 3).
    const CALL_LIMIT = 16 * 1024;
    const fits = (sent) => {
      try { return encodeURIComponent(JSON.stringify({ api: 'action.setIcon', args: [sent] })).length <= CALL_LIMIT; }
      catch (e) { return false; }
    };
    const setIcon = (details, callback) => {
      const answer = Promise.resolve().then(() => {
        if (!details || typeof details !== 'object') throw new Error('Invalid value for argument 1.');
        const sent = {};
        if (details.tabId !== undefined) sent.tabId = details.tabId;
        if (details.imageData === undefined) {
          if (details.path !== undefined) sent.path = resolved(details.path);
          if (!fits(sent)) throw new Error('Invalid value for argument 1. Property \'path\': too long to send.');
          return ask('action.setIcon', [sent]);
        }
        const image = pick(details.imageData);
        if (!image) throw new Error('Invalid value for argument 1. Property \'imageData\': no ImageData of a size up to 64.');
        return png(image).then((encoded) => {
          sent.imageData = { png: encoded };
          if (!fits(sent)) throw new Error('Invalid value for argument 1. Property \'imageData\': too large to send.');
          return ask('action.setIcon', [sent]);
        });
      });
      return typeof callback === 'function' ? toCallback(answer, callback, true) : answer;
    };
    own.set('action.setIcon', setIcon);
    made.push('action.setIcon');

    // 'contextMenus.create' as Chrome has it (R-169 mid 4): the id is
    // handed back at once, whatever the application then says of it; what
    // it says reaches the callback, or nobody. An 'onclick' is a function,
    // which no call to the application can carry: refused before anything
    // is sent, in Chrome's words, which refuses it for a worker too.
    if (hosted.has('contextMenus.create')) {
      const create = (properties, callback) => {
        if (properties && typeof properties === 'object' && typeof properties.onclick === 'function')
          throw new Error('Extensions using event pages or Service Workers cannot pass an onclick parameter to chrome.contextMenus.create. Instead, use the chrome.contextMenus.onClicked event.');
        const answer = ask('contextMenus.create', [properties]);
        if (typeof callback === 'function') toCallback(answer, callback, true);
        else answer.then(() => {}, () => {});
        return properties && typeof properties === 'object' && properties.id !== undefined ? properties.id : undefined;
      };
      own.set('contextMenus.create', create);
      made.push('contextMenus.create');
    }
  };
)js";

const char *CONTEXTS = R"js(
  const installContexts = () => {
    if (!hosted.has('offscreen.contexts')) return;
    let runtime = null;
    try { runtime = chrome.runtime; } catch (e) { return; }
    if (!runtime || typeof runtime !== 'object') return;
    const real = typeof runtime.getContexts === 'function' ? runtime.getContexts.bind(runtime) : null;
    const LISTS = ['contextTypes', 'documentUrls', 'documentIds', 'contextIds', 'tabIds', 'windowIds', 'frameIds', 'documentOrigins'];
    const CONTEXTS_WAIT = 10000;
    const getContexts = (filter, callback) => {
      const answer = Promise.resolve().then(() => {
        // the filter as Chrome reads it: an object whose lists are lists,
        // and of no other keys. What is not is refused here and asked of
        // nobody (R-189 mid 2, R-191 low 5). (Chrome refuses 'null' too;
        // it is read as nothing here.)
        const f = filter === undefined || filter === null ? {} : filter;
        if (typeof f !== 'object' || Array.isArray(f) || LISTS.some((k) => f[k] !== undefined && !Array.isArray(f[k]))
            || (f.incognito !== undefined && typeof f.incognito !== 'boolean')
            || Object.keys(f).some((k) => k !== 'incognito' && !LISTS.includes(k)))
          throw new Error("Invalid value for argument 1. Property 'filter': expected a ContextFilter.");
        // the engine's own answer in whatever form it gives it -- a
        // promise, a list at once, or through the callback it is handed
        // (this engine's: 'undefined' back, the callback called, a
        // failure as 'lastError' -- measured, D-380 追記 3) -- and its
        // failure is the call's, as is the application's. One which gives
        // nothing at all is waited on no longer than the application is.
        const theirs = !real ? Promise.resolve([]) : new Promise((resolve, reject) => {
          const said = (v) => {
            let failed = null;
            try { failed = chrome.runtime.lastError; } catch (e) {}
            if (failed) reject(new Error(String(failed.message || failed))); else resolve(v);
          };
          let got;
          try { got = real(f, said); } catch (e) { reject(e); return; }
          if (got && typeof got.then === 'function') got.then(resolve, reject);
          else if (got !== undefined) resolve(got);
          else setTimeout(() => resolve([]), CONTEXTS_WAIT);
        }).then((v) => Array.isArray(v) ? v : []);
        const ours = ask('offscreen.contexts', []).then((v) => Array.isArray(v) ? v : []);
        return Promise.all([theirs, ours]).then(([engines, documents]) => {
          const origin = 'chrome-extension://' + chrome.runtime.id;
          const docs = documents.map((d) => ({ contextType: 'OFFSCREEN_DOCUMENT', contextId: String(d.contextId), documentId: String(d.documentId),
                                               documentUrl: String(d.documentUrl), documentOrigin: origin,
                                               frameId: 0, tabId: -1, windowId: -1, incognito: false }));
          const listed = (key, value) => !Array.isArray(f[key]) || f[key].includes(value);
          return engines.concat(docs).filter((c) => c && typeof c === 'object'
            && listed('contextTypes', c.contextType) && listed('documentUrls', c.documentUrl) && listed('documentIds', c.documentId)
            && listed('contextIds', c.contextId) && listed('tabIds', c.tabId) && listed('windowIds', c.windowId)
            && listed('frameIds', c.frameId) && listed('documentOrigins', c.documentOrigin)
            && (f.incognito === undefined || f.incognito === !!c.incognito));
        });
      });
      return typeof callback === 'function' ? toCallback(answer, callback) : answer;
    };
    Object.defineProperty(runtime, 'getContexts', { value: getContexts, configurable: true, writable: true, enumerable: true });
    made.push('runtime.getContexts');
  };
)js";

const char *DOWNLOADS = R"js(
  const goodDownloadUrl = (url) => typeof url === 'string' && /^(?:blob:|data:)/i.test(url);
  const installDownloads = () => {
    if (!hosted.has('downloads.expect')) return;
    let asking = 0;
    const held = [];
    const holdOf = (name) => (args) => { if (!asking) return false; held.push([name, args]); return true; };
    holders.set('downloads.onCreated', holdOf('downloads.onCreated'));
    holders.set('downloads.onChanged', holdOf('downloads.onChanged'));
    const release = () => {
      if (asking || !held.length) return;
      const list = held.splice(0);
      setTimeout(() => { for (const [name, args] of list) handOver(name, args); }, 0);
    };
    const download = (options, callback) => {
      const answer = Promise.resolve().then(() => {
        if (!options || typeof options !== 'object' || !goodDownloadUrl(options.url))
          throw new Error('Invalid value for argument 1. Property \'url\': Invalid URL.');
        const filename = typeof options.filename === 'string' ? options.filename : '';
        asking++;
        return ask('downloads.expect', [{ url: options.url, filename }]).then(
          (id) => { asking--; release(); return id; },
          (error) => { asking--; release(); throw error; });
      });
      return typeof callback === 'function' ? toCallback(answer, callback) : answer;
    };
    own.set('downloads.download', download);
    made.push('downloads.download');
  };
)js";

const char *MENU_MIRROR = R"js(
  let menuChosen = () => {};
  const menuNumbers = new Map();
  const fromHost = (args) => {
    if (!Array.isArray(args)) return args;
    const out = args.slice();
    const info = out[0];
    if (info && typeof info === 'object') {
      const copy = Object.assign({}, info);
      for (const key of ['menuItemId', 'parentMenuItemId'])
        if (typeof copy[key] === 'string' && menuNumbers.has(copy[key])) copy[key] = menuNumbers.get(copy[key]);
      out[0] = copy;
    }
    return out;
  };
  const installMenuMirror = () => {
    if (!RELAYED || !hosted.size) return;
    let menus = null, permitted = false;
    try { menus = chrome.contextMenus; } catch (e) {}
    try {
      const manifest = chrome.runtime.getManifest();
      permitted = !!manifest && Array.isArray(manifest.permissions) && manifest.permissions.includes('contextMenus');
    } catch (e) {}
    if (!permitted || !menus) return;
    const engine = {};
    for (const name of ['create', 'update', 'remove', 'removeAll']) {
      if (typeof menus[name] !== 'function') return;
      engine[name] = menus[name].bind(menus);
    }
    const WAITING_MOST = 1024, BATCH_MOST = 8000;
    const waiting = [];
    let sending = false;
    const flush = () => {
      if (sending || !waiting.length) return;
      const batch = [];
      let size = 0;
      while (waiting.length) {
        let one = 0;
        try { one = encodeURIComponent(JSON.stringify(waiting[0])).length; } catch (e) { waiting.shift(); continue; }
        if (batch.length && size + one > BATCH_MOST) break;
        batch.push(waiting.shift());
        size += one;
      }
      if (!batch.length) return;
      sending = true;
      const done = () => { sending = false; flush(); };
      try { Promise.resolve(ask('vanilla.menuMirror', [batch])).then(done, done); } catch (e) { done(); }
    };
    // what the engine accepted, as JSON says it (an 'onclick' the engine
    // refuses on a worker anyway).
    const told = (op, args) => {
      if (waiting.length >= WAITING_MOST) return;
      let copy;
      try { copy = JSON.parse(JSON.stringify(args)); } catch (e) { return; }
      waiting.push([op, copy]);
      if (!sending) Promise.resolve().then(flush);
    };
    const accepted = () => { try { return !chrome.runtime.lastError; } catch (e) { return false; } };
    // the call as the engine has it; the extension's callback, its
    // 'lastError' and its promise are the engine's.
    const called = (op, args, callback) => {
      if (typeof callback === 'function') {
        engine[op](...args, function () { if (accepted()) told(op, args); return callback.apply(this, arguments); });
        return undefined;
      }
      return new Promise((resolve, reject) => {
        engine[op](...args, () => {
          let error = null;
          try { error = chrome.runtime.lastError || null; } catch (e) {}
          if (error) { reject(new Error(error.message)); return; }
          told(op, args);
          resolve();
        });
      });
    };
    own.set('contextMenus.create', (properties, callback) => {
      const id = engine.create(properties, function () {
        if (accepted()) told('create', [properties]);
        if (typeof callback === 'function') return callback.apply(this, arguments);
      });
      const named = properties && typeof properties === 'object' ? properties.id : undefined;
      if (typeof named === 'number' && menuNumbers.size < 4096) menuNumbers.set(String(named), named);
      return id;
    });
    own.set('contextMenus.update', (id, properties, callback) => called('update', [id, properties], callback));
    own.set('contextMenus.remove', (id, callback) => called('remove', [id], callback));
    own.set('contextMenus.removeAll', (callback) => called('removeAll', [], callback));
    for (const name of ['create', 'update', 'remove', 'removeAll']) OWN.add('contextMenus.' + name);
  };
)js";

const char *MENU_TAB = R"js(
  const installMenuTab = () => {
    if (!RELAYED || !hosted.size) return;
    let real = null;
    try { real = chrome.contextMenus ? chrome.contextMenus.onClicked : null; } catch (e) {}
    if (!real || typeof real.addListener !== 'function') return;
    const list = [], CHAINED_MOST = 16;
    let chain = Promise.resolve(), chained = 0;
    // the engine's tab as the application's: its own fields as they are,
    // save the ones which carry the engine's numbers -- the tab, its
    // window, its opener, its group and its split view.
    const retab = (tab, answer) => {
      if (!tab || typeof tab !== 'object') return tab;
      const copy = Object.assign({}, tab);
      delete copy.openerTabId;
      if ('groupId' in copy) copy.groupId = -1;
      if ('splitViewId' in copy) copy.splitViewId = -1;
      if (isTab(answer)) Object.assign(copy, { id: answer.id, index: answer.index, windowId: 1 });
      else Object.assign(copy, { id: -1, windowId: -1 });
      return copy;
    };
    const heard = (info, tab) => {
      // asked now, handed over in turn. A refusal, the deadline and an
      // answer of any other shape are all "no tab of ours".
      let answer = null;
      if (chained < CHAINED_MOST) {
        const pageUrl = info && typeof info === 'object' && typeof info.pageUrl === 'string' ? info.pageUrl : '';
        try { answer = Promise.resolve(ask('vanilla.menuTab', [pageUrl])).then((value) => value, () => null); }
        catch (e) { answer = null; }
      }
      chained++;
      const hand = (value) => {
        chained--;
        let handed;
        try { handed = retab(tab, value); } catch (e) { handed = { id: -1, windowId: -1 }; }
        tell(list.slice(), [info, handed]);
      };
      chain = chain.then(() => answer).then(hand, () => hand(null));
    };
    try { real.addListener(heard); } catch (e) { return; }
    // the button's own menu (D-469): the engine's calls told to the host,
    // and a choice of it handed to the same listeners, in their turn.
    try { installMenuMirror(); } catch (e) {}
    menuChosen = (args) => {
      if (!Array.isArray(args)) return;
      const handed = args.slice(0, 2);
      chain = chain.then(() => tell(list.slice(), handed), () => tell(list.slice(), handed));
    };
    OWN.add('contextMenus.onClicked');
    own.set('contextMenus.onClicked', Object.assign(listenedTo(list), { addRules() {}, removeRules() {}, getRules() {} }));
  };
)js";

const char *SCRIPTING_TAB = R"js(
  const engineScripting = new Map();
  // and the engine's own 'tabs.query' and 'tabs.sendMessage', which find a
  // tab with no link by asking its document (D-434). Taken here for the
  // same reason: after the stand-ins, 'chrome.tabs' is ours.
  const engineTabs = new Map();
  const takeEngineTabs = () => {
    let tabs = null;
    try { tabs = chrome.tabs || null; } catch (e) {}
    for (const key of ['query', 'sendMessage']) {
      let f = null;
      try { f = tabs ? tabs[key] : null; } catch (e) {}
      if (typeof f === 'function') engineTabs.set(key, f.bind(tabs));
    }
  };
  const installScriptingTab = () => {
    if (!RELAYED || !hosted.size) return;
    let real = null;
    try { real = chrome.scripting || null; } catch (e) {}
    if (!real || typeof real !== 'object') return;
    for (const key of ['executeScript', 'insertCSS', 'removeCSS']) {
      let f = null;
      try { f = real[key]; } catch (e) {}
      if (typeof f === 'function') engineScripting.set(key, f.bind(real));
    }
    engineScripting.forEach((f, key) => OWN.add('scripting.' + key));
    takeEngineTabs();
  };
  // A tab with no link -- uBOL's carrier stands down on this engine so
  // that the worker may sleep (R-232 mid 1) -- is found by asking
  // (D-434): the host says where tab 'tabId' is, the engine's tabs
  // showing that address are the candidates, and each is asked over
  // the engine's own 'tabs.sendMessage' for its top document's nonce,
  // which the host answers with the tab of ours it is bound to. The
  // number handed on is the one the engine delivered to AND the host
  // named as 'tabId': the address only narrows the asking. Nothing is
  // kept: the next call asks again.
  const WHO_ANSWER = 1000, WHO_BUDGET = 3000, WHO_MOST = 4;
  const bare = (url) => typeof url === 'string' && url ? url.split('#')[0] : null;
  const within = (promise, ms) => new Promise((resolve) => {
    const timer = setTimeout(() => resolve(null), Math.max(0, ms));
    Promise.resolve(promise).then((v) => { clearTimeout(timer); resolve(v); }, () => { clearTimeout(timer); resolve(null); });
  });
  const askedTab = (tabId) => {
    const query = engineTabs.get('query'), send = engineTabs.get('sendMessage');
    if (!query || !send) return Promise.resolve(null);
    return ask('tabs.get', [tabId]).then((tab) => {
      const url = bare(tab && tab.url);
      if (!url) return null;
      // (counted from here: a worker woken by the call waited for its
      //  relay before the host could answer at all. R-252 mid 2.)
      const end = Date.now() + WHO_BUDGET;
      let listed;
      try { listed = query({}); } catch (e) { listed = null; }
      return within(listed, WHO_BUDGET).then((list) => {
        const candidates = (Array.isArray(list) ? list : [])
          .filter((t) => t && Number.isInteger(t.id) && t.id > 0 && bare(t.url) === url);
        if (!candidates.length || candidates.length > WHO_MOST) return null;
        const one = (i) => {
          if (i >= candidates.length || Date.now() >= end) return null;
          const theirs = candidates[i].id;
          let sent;
          try { sent = send(theirs, { __vanillaWho: 1 }, { frameId: 0 }); } catch (e) { sent = null; }
          return within(sent, Math.min(WHO_ANSWER, end - Date.now())).then((answer) => {
            const nonce = answer && typeof answer === 'object' ? answer.nonce : null;
            if (typeof nonce !== 'string' || !/^[0-9a-f]{32}$/.test(nonce)) return null;
            return within(ask('vanilla.tabOf', [nonce]), end - Date.now())
              .then((named) => isTab(named) && named.id === tabId ? theirs : null);
          }).then((found) => found !== null ? found : one(i + 1));
        };
        return one(0);
      });
    }, () => null);
  };
  // 'run' with the injection on the engine's tab 'found' comes to -- copies,
  // and the caller's objects as they were: everything but the tab goes to
  // the engine as it was given -- or refused where there is none. (The
  // target as it was at the call: what the caller puts there meanwhile is
  // not read. R-299 mid 1.)
  const onTheirTab = (found, injection, run) => {
    const target = injection.target;
    return found.then((theirs) => {
      if (theirs === null) throw new Error('No tab with id: ' + target.tabId + '.');
      return run(Object.assign({}, injection, { target: Object.assign({}, target, { tabId: theirs }) }));
    });
  };
)js";

const char *INSTALLED = R"js(
  // one event of 'runtime' the application keeps owed for this worker and
  // hands over once: 'name' stood in for, 'call' asked, and 'argsOf' making
  // the listeners' arguments of the answer (null: nothing to fire).
  // 'runtime.onStartup' goes the same way (D-414: the Qt engine never fires
  // it either; measured 2026-09-24, three starts).
  const installOwed = (name, call, argsOf) => {
    if (EDGE || !/^[0-9a-f]{64}$/.test(HOST_KEY) || typeof fetch !== 'function') return;
    let runtime = null;
    try { runtime = chrome.runtime; } catch (e) {}
    if (!runtime) return;
    const listeners = [];
    // asked a turn after the first listener is added -- which is in the
    // worker's own script, where the rest are added too -- and once. With
    // nobody listening nothing is asked, so nothing is taken: what is owed
    // stays owed.
    let asked = false;
    const askOnce = () => {
      if (!listeners.length) { asked = false; return; }
      let asking;
      try { asking = Promise.resolve(ask(call, [])); } catch (e) { return; }
      asking.then((value) => {
        const args = argsOf(value);
        if (!args) return;
        for (const f of listeners.slice()) { try { f.apply(undefined, args); } catch (e) { try { console.error(e); } catch (x) {} } }
      }, () => {});
    };
    const event = listenedTo(listeners, () => { if (!asked) { asked = true; setTimeout(askOnce, 0); } });
    try { Object.defineProperty(runtime, name, { value: event, configurable: true, writable: true, enumerable: true }); made.push('runtime.' + name); }
    catch (e) { refused.push('runtime.' + name); }
  };
  const installInstalled = () => installOwed('onInstalled', 'vanilla.installed', (value) => {
    if (!value || typeof value !== 'object' || (value.reason !== 'install' && value.reason !== 'update')) return null;
    const details = { reason: value.reason };
    if (value.reason === 'update' && typeof value.previousVersion === 'string') details.previousVersion = value.previousVersion;
    return [details];
  });
  // (fired with no argument, as Chrome's is; only on a plain 'true'.)
  const installStartup = () => installOwed('onStartup', 'vanilla.startup', (value) => value === true ? [] : null);
)js";

const char *OPTIONS = R"js(
  const installOptions = () => {
    if (!hosted.has('tabs.create')) return;
    let runtime = null;
    try { runtime = chrome.runtime; } catch (e) { return; }
    if (!runtime || typeof runtime !== 'object') return;
    const openOptionsPage = (callback) => {
      const answer = Promise.resolve().then(() => {
        let page = '';
        try {
          const manifest = chrome.runtime.getManifest();
          page = (manifest.options_ui && manifest.options_ui.page) || manifest.options_page || '';
        } catch (e) {}
        if (typeof page !== 'string' || !page) throw new Error('Could not create an options page.');
        return ask('tabs.create', [{ url: '/' + page.replace(/^\/+/, '') }]).then(() => undefined);
      });
      return typeof callback === 'function' ? toCallback(answer, callback, true) : answer;
    };
    Object.defineProperty(runtime, 'openOptionsPage', { value: openOptionsPage, configurable: true, writable: true, enumerable: true });
    made.push('runtime.openOptionsPage');
  };
)js";

const char *IDENTITY = R"js(
  const installIdentity = (worker) => {
    if (EDGE) return;
    let id = '', asked = false;
    try {
      id = chrome.runtime.id;
      const manifest = chrome.runtime.getManifest();
      asked = Array.isArray(manifest.permissions) && manifest.permissions.includes('identity');
    } catch (e) { return; }
    if (!asked || typeof id !== 'string' || !id) return;
    const WAIT_MOST = 30 * 60 * 1000, KEEP_EVERY = 20000;
    const settle = (answer, callback) => {
      return typeof callback === 'function' ? toCallback(answer, callback) : answer;
    };
    const callbackOf = (args) => typeof args[args.length - 1] === 'function' ? args.pop() : null;
    // the worker's own calls, one every twenty seconds, while a flow waits.
    let waiting = 0, keeping = false;
    const keep = () => {
      if (!waiting) { keeping = false; return; }
      try { chrome.runtime.getPlatformInfo(() => { try { void chrome.runtime.lastError; } catch (e) {} }); } catch (e) {}
      try { setTimeout(keep, KEEP_EVERY); } catch (e) { keeping = false; }
    };
    const wait = (by) => {
      waiting = Math.max(0, waiting + by);
      if (waiting && !keeping) {
        keeping = true;
        try { setTimeout(keep, KEEP_EVERY); } catch (e) { keeping = false; }
      }
    };
    const members = {
      getRedirectURL(path) {
        return 'https://' + id + '.chromiumapp.org/' + (typeof path === 'string' ? path.replace(/^\/+/, '') : '');
      },
      launchWebAuthFlow(...args) {
        const callback = callbackOf(args);
        const answer = Promise.resolve().then(() => {
          // (asked of nobody where the copy was written without the key.)
          if (!hosted.has('tabs.query')) throw new Error('chrome.identity.launchWebAuthFlow is not available in this browser');
          const details = args[0];
          if (!details || typeof details !== 'object' || typeof details.url !== 'string')
            throw new TypeError('Error in invocation of identity.launchWebAuthFlow(identity.WebAuthFlowDetails details, optional function callback): No matching signature.');
          const given = { url: details.url };
          for (const key of ['interactive', 'abortOnLoadForNonInteractive', 'timeoutMsForNonInteractive'])
            if (key in details) given[key] = details[key];
          if (worker) wait(1);
          const done = () => { if (worker) wait(-1); };
          return ask('identity.launchWebAuthFlow', [given], WAIT_MOST).then((value) => { done(); return value; },
                                                                            (error) => { done(); throw error; });
        });
        return settle(answer, callback);
      },
      getProfileUserInfo(...args) { return settle(Promise.resolve({ email: '', id: '' }), callbackOf(args)); },
      getAuthToken(...args) { return settle(Promise.reject(new Error('The user is not signed in.')), callbackOf(args)); },
      removeCachedAuthToken(...args) { return settle(Promise.resolve(), callbackOf(args)); },
      clearAllCachedAuthTokens(...args) { return settle(Promise.resolve(), callbackOf(args)); }
    };
    ownAll('identity', members);
  };
)js";

const char *ALARMS = R"js(
  // (the alarms' and the idle's, below: what the engine said of the call
  //  whose callback this is.)
  const failedNow = () => { try { return chrome.runtime.lastError || null; } catch (e) { return null; } };
  const installAlarms = () => {
    if (EDGE) return;
    let real = null;
    try { real = chrome.alarms || null; } catch (e) {}
    if (!real || typeof real.getAll !== 'function' || typeof real.create !== 'function'
        || typeof real.clear !== 'function' || typeof real.clearAll !== 'function') return;
    const LOOK_EVERY = 20000, ANSWER_WAIT = 10000;
    // name -> { scheduledTime, periodInMinutes, fired, guess }: 'fired' is
    // the 'scheduledTime' it was last rung for; 'guess' is what 'create'
    // put in before any list had it.
    // and 'rungFor' the same, by name, for as long as the worker lives: what
    // leaves the table and comes back -- a 'create' the engine refused --
    // is not rung twice for one time (R-268 low 1).
    // 'lastOf' is the number of the last 'create' or 'clear' of a name,
    // 'lastAll' of the last 'clearAll'.
    const table = new Map(), rungFor = new Map(), lastOf = new Map(), listeners = [];
    let generation = 0, answering = 0, looking = false, again = false, soon = false, timer = null;
    let calls = 0, lastAll = 0;
    const listed = () => new Promise((resolve, reject) => {
      let done = false, wait = null;
      const end = (f, v) => { if (done) return; done = true; if (wait !== null) clearTimeout(wait); f(v); };
      wait = setTimeout(() => end(reject, new Error('no answer')), ANSWER_WAIT);
      try {
        const got = real.getAll((v) => { const failed = failedNow(); if (failed) end(reject, failed); else end(resolve, v); });
        if (got && typeof got.then === 'function') got.then((v) => end(resolve, v), (e) => end(reject, e));
      } catch (e) { end(reject, e); }
    });
    const ring = (name, entry) => {
      entry.fired = entry.scheduledTime;
      rungFor.set(name, entry.scheduledTime);
      const alarm = { name, scheduledTime: entry.scheduledTime };
      if (entry.periodInMinutes !== undefined) alarm.periodInMinutes = entry.periodInMinutes;
      for (const f of listeners.slice()) { try { f(Object.assign({}, alarm)); } catch (e) { try { console.error(e); } catch (x) {} } }
    };
    // (after a look which got no list, the next is twenty seconds on: an
    //  engine which fails at once is not asked again and again.)
    const arm = (failed) => {
      if (timer !== null) { clearTimeout(timer); timer = null; }
      if (!table.size || !listeners.length) return;
      const now = Date.now();
      let next = now + LOOK_EVERY;
      if (!failed) table.forEach((entry) => { if (entry.fired !== entry.scheduledTime) next = Math.min(next, Math.max(entry.scheduledTime, now)); });
      timer = setTimeout(() => { timer = null; look(); }, next - now);
    };
    const lookSoon = () => {
      if (soon) return;
      soon = true;
      setTimeout(() => { soon = false; look(); }, 0);
    };
    const look = () => {
      if (looking || answering) { again = true; return; }
      looking = true; again = false;
      const asked = generation;
      // (ended in the same turn as the answer is read, so that no turn
      //  goes by with the look over and nothing armed.)
      const end = (failed) => { looking = false; if (again) lookSoon(); else arm(failed); };
      listed().then((list) => { try { read(asked, list); } finally { end(false); } }, () => end(true)).catch(() => {});
    };
    const read = (asked, list) => {
      if (asked !== generation || answering) { again = true; return; }
      const there = new Map();
      if (Array.isArray(list)) for (const one of list)
        if (one && typeof one.name === 'string' && typeof one.scheduledTime === 'number') there.set(one.name, one);
      const now = Date.now();
      // (a) what is due rings, off the table as it stands: a listener
      // which clears or makes another is read as it goes.
      for (const [name, entry] of Array.from(table)) {
        if (table.get(name) !== entry) continue;
        if (entry.guess) {
          if (!there.has(name) && entry.periodInMinutes === undefined && entry.scheduledTime <= now
              && entry.fired !== entry.scheduledTime) ring(name, entry);
          continue;
        }
        if (entry.scheduledTime <= now && entry.fired !== entry.scheduledTime
            && (there.has(name) || entry.periodInMinutes === undefined)) ring(name, entry);
      }
      // (b) and the table is the engine's list -- unless a listener
      // asked the engine something meanwhile, which a look to come reads.
      if (asked !== generation || answering) { again = true; return; }
      for (const name of Array.from(table.keys())) if (!there.has(name)) table.delete(name);
      there.forEach((one, name) => {
        const had = table.get(name);
        const entry = had && !had.guess ? had : { fired: rungFor.has(name) ? rungFor.get(name) : null };
        entry.scheduledTime = one.scheduledTime;
        entry.periodInMinutes = typeof one.periodInMinutes === 'number' ? one.periodInMinutes : undefined;
        table.set(name, entry);
      });
    };
    // 'create', 'clear' and 'clearAll' go to the engine with what they
    // were given, and hand back what it does -- its promise, its value,
    // what it throws -- the callback called inside the engine's, where
    // 'lastError' is still to be read. What was given is read here only to
    // take what it replaces or clears out of the table first ('drop') and
    // to put what it made in once the engine said yes ('done').
    const through = (key, drop, done) => function (...args) {
      const mine = ++calls, at = Date.now();
      drop(args, mine);
      generation++; answering++;
      let answered = false, wait = null;
      const answer = () => {
        if (answered) return;
        answered = true;
        if (wait !== null) clearTimeout(wait);
        answering--; generation++;
        lookSoon();
      };
      wait = setTimeout(answer, ANSWER_WAIT);
      const last = args.length ? args[args.length - 1] : undefined;
      const given = args.slice();
      // (a yes after it was given up on puts nothing in and has a look
      //  come: what the engine did is read there.)
      const yes = () => {
        generation++;
        if (answered) { lookSoon(); return; }
        if (done) { try { done(given, mine, at); } catch (e) {} }
      };
      if (typeof last === 'function')
        args[args.length - 1] = function (...values) {
          if (!failedNow()) yes();
          try { return last.apply(this, values); } finally { answer(); }
        };
      let got;
      try { got = real[key].apply(real, args); } catch (e) { answer(); throw e; }
      if (got && typeof got.then === 'function') got.then(() => { yes(); answer(); }, answer);
      else if (typeof last !== 'function') { yes(); answer(); }
      return got;
    };
    const nameOf = (args) => typeof args[0] === 'string' ? args[0] : '';
    // (the time off the clock as it was asked: the engine's own is no
    //  sooner, so one it used up is due here too.)
    const guess = (args, mine, asked) => {
      const name = nameOf(args);
      if (lastOf.get(name) !== mine || lastAll > mine) return;
      const info = args[typeof args[0] === 'string' ? 1 : 0];
      if (!info || typeof info !== 'object') return;
      const period = typeof info.periodInMinutes === 'number' ? info.periodInMinutes : undefined;
      const delay = typeof info.delayInMinutes === 'number' ? info.delayInMinutes : period;
      const at = typeof info.when === 'number' ? info.when : asked + (typeof delay === 'number' ? delay : 0) * 60000;
      table.set(name, { scheduledTime: at, periodInMinutes: period, fired: null, guess: true });
    };
    const leaving = (args, mine) => { const name = nameOf(args); table.delete(name); lastOf.set(name, mine); };
    const members = {
      create: through('create', leaving, guess),
      clear: through('clear', leaving, null),
      clearAll: through('clearAll', (args, mine) => { table.clear(); lastOf.clear(); lastAll = mine; }, null),
      onAlarm: listenedTo(listeners, () => { if (listeners.length === 1) lookSoon(); },
                          () => { if (timer !== null) { clearTimeout(timer); timer = null; } })
    };
    ownAll('alarms', members);
  };
)js";

const char *IDLE = R"js(
  const installIdle = () => {
    if (EDGE) return;
    let real = null;
    try { real = chrome.idle || null; } catch (e) {}
    if (!real || typeof real.queryState !== 'function' || typeof real.setDetectionInterval !== 'function') return;
    const EVERY = 1000, ANSWER_WAIT = 10000, STATES = ['active', 'idle', 'locked'];
    const listeners = [];
    let interval = 60, last = 'active', timer = null, asking = null;
    const arm = () => {
      if (timer !== null || asking || !listeners.length) return;
      timer = setTimeout(() => { timer = null; query(); }, EVERY);
    };
    const query = () => {
      if (asking || !listeners.length) return;
      const mine = asking = {};
      let wait = null;
      const end = (state) => {
        if (asking !== mine) return;
        asking = null;
        if (wait !== null) clearTimeout(wait);
        if (listeners.length && STATES.includes(state) && state !== last) {
          last = state;
          for (const f of listeners.slice()) { try { f(state); } catch (e) { try { console.error(e); } catch (x) {} } }
        }
        arm();
      };
      wait = setTimeout(() => end(undefined), ANSWER_WAIT);
      try {
        const got = real.queryState(interval, (state) => end(failedNow() ? undefined : state));
        if (got && typeof got.then === 'function') got.then((state) => end(state), () => end(undefined));
      } catch (e) { end(undefined); }
    };
    const members = {
      // (the engine's, with what it was given; what it takes is what is
      //  asked with from then on.)
      setDetectionInterval(...args) {
        const got = real.setDetectionInterval.apply(real, args);
        if (typeof args[0] === 'number' && isFinite(args[0])) interval = Math.min(Math.max(Math.floor(args[0]), 15), 14400);
        return got;
      },
      onStateChanged: listenedTo(listeners, arm, () => { if (timer !== null) { clearTimeout(timer); timer = null; } })
    };
    ownAll('idle', members);
  };
)js";

const char *USER_SCRIPT_MESSAGES = R"js(
  const prepareUserScriptMessage = () => {
    if (EDGE || !hosted.has('userScripts.getScripts') || !eventToken) return null;
    let runtime = null;
    try { runtime = chrome.runtime; } catch (e) {}
    if (!runtime || typeof runtime !== 'object') return null;
    const NAME = 'runtime.onUserScriptMessage';
    SOUNDED.add(NAME);
    return () => {
      const inner = own.get(NAME);
      if (!inner) return;
      const mine = [];
      let named = false;
      const reply = (ticket, kind, value) => {
        // ('sendResponse()' answers 'undefined', and so does the sender: R-276 low 5.)
        const args = kind === 'value' && value !== undefined ? [ticket, eventToken, kind, value] : [ticket, eventToken, kind];
        try { Promise.resolve(ask('vanilla.userScriptReply', args)).catch(() => {}); } catch (e) {}
      };
      holders.set(NAME, (args) => {
        const message = args[0], sender = args[1], ticket = args[2];
        if (typeof ticket !== 'string') return true;
        if (!mine.length) { reply(ticket, 'nolistener'); return true; }
        let settled = false;
        const once = (...value) => {
          if (settled) return;
          settled = true;
          if (value.length) reply(ticket, 'value', value[0]); else reply(ticket, 'none');
        };
        decide(handAll(mine.slice(), message, sender, (value) => once(value)), once);
        return true;
      });
      const event = {
        addListener(f) {
          if (typeof f !== 'function' || mine.includes(f)) return;
          mine.push(f);
          inner.addListener(f);
          // (said once for this worker's subscription; said again if the
          //  application could not be told.)
          if (!named) {
            named = true;
            try { Promise.resolve(ask('vanilla.userScriptListen', [eventToken])).catch(() => { named = false; }); } catch (e) { named = false; }
          }
        },
        removeListener(f) {
          const at = mine.indexOf(f);
          if (at >= 0) mine.splice(at, 1);
          inner.removeListener(f);
        },
        hasListener(f) { return mine.includes(f); },
        hasListeners() { return mine.length > 0; }
      };
      Object.defineProperty(runtime, 'onUserScriptMessage', { value: event, configurable: true, writable: true, enumerable: true });
      made.push('runtime.onUserScriptMessage');
    };
  };
)js";

const char *USER_SCRIPT_PRELUDE = R"js((() => {
  const [ID, SECRET] = __VANILLA_WORLD_ARGS__;
  let root = null;
  try { root = self; } catch (e) { return; }
  if (!root || root.__vanillaUserScriptRuntime) return;
  try { Object.defineProperty(root, '__vanillaUserScriptRuntime', { value: true }); } catch (e) {}
  const post = root.fetch.bind(root), stringify = JSON.stringify, Failure = Error;
  const bare = Object.create, assign = Object.assign;
  const NO_RECEIVER = 'Could not establish connection. Receiving end does not exist.';
  const PORT_CLOSED = 'The message port closed before a response was received.';
  // the frame: 0 at the top, a number of this document's own below it.
  let frame = 0;
  try {
    if (root !== root.top) { const b = new Uint32Array(1); crypto.getRandomValues(b); frame = (b[0] & 0x7fffffff) || 1; }
  } catch (e) { frame = 1; }
  let lastError;
  const send = (message) => {
    let body;
    try { body = stringify({ message, url: String(location.href), frame }); }
    catch (e) { return Promise.reject(new Failure('Could not serialize message.')); }
    let asking;
    try {
      // (objects of no prototype: what 'fetch' looks for on them -- 'signal',
      //  'mode', how to read the headers -- is no getter a user script of
      //  this world may have put on 'Object.prototype'. R-276 mid 2.)
      const headers = assign(bare(null), { 'X-Vanilla-World': SECRET });
      const init = assign(bare(null), { method: 'POST', cache: 'no-store', credentials: 'omit', headers, body });
      asking = Promise.resolve(post('vanilla-extension://host/userScriptMessage', init));
    } catch (e) { return Promise.reject(new Failure(NO_RECEIVER)); }
    return asking.then((response) => response.json(), () => { throw new Failure(NO_RECEIVER); }).then((answer) => {
      if (answer && answer.ok === true) return answer.none === true ? { none: true } : { value: answer.value };
      throw new Failure(answer && typeof answer.error === 'string' ? answer.error : NO_RECEIVER);
    });
  };
  const runtime = {
    id: ID,
    get lastError() { return lastError; },
    sendMessage(...args) {
      const callback = typeof args[args.length - 1] === 'function' ? args.pop() : null;
      let to = ID, message;
      if (args.length >= 3) { to = args[0] == null ? ID : args[0]; message = args[1]; }
      else if (args.length === 2 && typeof args[0] === 'string') { to = args[0]; message = args[1]; }
      else message = args[0];
      const answer = to === ID ? send(message) : Promise.reject(new Failure(NO_RECEIVER));
      if (!callback) return answer.then((a) => (a.none ? undefined : a.value));
      const call = (error, value) => {
        lastError = error ? { message: error } : undefined;
        try { if (error) callback(); else callback(value); } finally { lastError = undefined; }
      };
      answer.then((a) => (a.none ? call(PORT_CLOSED) : call(null, a.value)), (e) => call(String(e && e.message || NO_RECEIVER)));
      return undefined;
    }
  };
  let host = null;
  try { host = root.chrome; } catch (e) {}
  if (!host || typeof host !== 'object') { host = {}; try { root.chrome = host; } catch (e) { return; } }
  try { Object.defineProperty(host, 'runtime', { value: runtime, configurable: true, enumerable: true, writable: true }); } catch (e) {}
})();
)js";

const char *EXTENSION_NS = R"js(
  const installExtension = () => {
    let has = null;
    try { has = chrome.extension; } catch (e) {}
    if (has) return;
    const no = (callback) => {
      if (typeof callback === 'function') { try { callback(false); } catch (e) {} return undefined; }
      return Promise.resolve(false);
    };
    chrome.extension = { inIncognitoContext: false, isAllowedFileSchemeAccess: no, isAllowedIncognitoAccess: no };
    made.push('extension');
  };
)js";

const char *I18N = R"js(
  const installMessages = (standing) => {
    // WebView2 has an 'i18n' which answers (D-379 addition 2, decision 8):
    // the messages the copy wrote are the Qt engine's want and nobody
    // else's, and standing in front of a working one would answer '' for
    // every name the copy has no entry for. The worker and a page know
    // which engine they are on by its brand; a content script does not ask
    // the brand at all and asks the ENGINE instead, below (D-379 addition
    // 4, R-197 mid 1).
    if (BY_BRAND && EDGE) return;
    let table = null;
    try { table = self.__vanillaMessages; } catch (e) { return; }
    if (!table || typeof table !== 'object' || !table.messages || typeof table.messages !== 'object') return;
    const messages = table.messages;
    // what is missing, asked for outright: this engine answers '' for every
    // name there is (D-313), so an engine which answers a name of the
    // copy's OWN table has an 'i18n' which works and is left alone. With no
    // table there is nothing to put in and this is never reached.
    if (!BY_BRAND) {
      let speaks = false;
      try {
        let first = null;
        for (const k in messages) { first = k; break; }
        if (first !== null) { const said = chrome.i18n.getMessage(first); speaks = typeof said === 'string' && said !== ''; }
      } catch (e) { speaks = false; }
      if (speaks) return;
    }
    const locale = typeof table.locale === 'string' ? table.locale : '';
    let id = '';
    try { id = String(chrome.runtime.id || ''); } catch (e) {}
    const reserved = { '@@extension_id': id, '@@ui_locale': locale, '@@bidi_dir': 'ltr', '@@bidi_reversed_dir': 'rtl',
                       '@@bidi_start_edge': 'left', '@@bidi_end_edge': 'right' };
    const has = (o, k) => Object.prototype.hasOwnProperty.call(o, k);
    const getMessage = (name, substitutions, options) => {
      if (typeof name !== 'string') return '';
      // more than nine substitutions is no message at all, whatever the
      // name: Chromium's 'i18n_hooks_delegate' answers 'undefined' to a
      // list of ten (D-404), before it looks the name up, and does not throw.
      const given = substitutions === undefined || substitutions === null ? []
                  : Array.isArray(substitutions) ? substitutions : [substitutions];
      if (given.length > 9) return undefined;
      const key = name.toLowerCase();
      let text;
      if (has(reserved, key)) text = reserved[key];
      else {
        const entry = has(messages, key) ? messages[key] : null;
        if (!entry || typeof entry !== 'object' || typeof entry.message !== 'string') return '';
        text = entry.message;
        const holders = entry.placeholders && typeof entry.placeholders === 'object' ? entry.placeholders : null;
        if (holders) text = text.replace(/\$([A-Za-z0-9_@]+)\$/g, (whole, holder) => {
          const one = has(holders, holder.toLowerCase()) ? holders[holder.toLowerCase()] : null;
          return one && typeof one.content === 'string' ? one.content : whole;
        });
      }
      if (options && typeof options === 'object' && options.escapeLt) text = text.replace(/</g, '&lt;');
      return text.replace(/\$([1-9$])/g, (whole, n) => {
        if (n === '$') return '$';
        const one = given[Number(n) - 1];
        return one === undefined || one === null ? '' : String(one);
      });
    };
    if (standing) {
      OWN.add('i18n.getMessage');
      own.set('i18n.getMessage', getMessage);
    } else {
      let target = null;
      try { target = chrome.i18n; } catch (e) {}
      if (!target || typeof target !== 'object') chrome.i18n = { getMessage };
      else Object.defineProperty(target, 'getMessage', { value: getMessage, configurable: true, writable: true, enumerable: true });
    }
    made.push('i18n');
  };
)js";

const char *STORAGE = R"js(
  let lookAgain = () => {};
  // What is put in for an engine which refuses every call of
  // 'chrome.storage.sync' and fires no change event: the corner of 'local'
  // which stands in for 'sync', and the looking which stands in for the
  // events. It is NOT put in unmeasured (D-379 addition 2, decision 8):
  // what it keeps under a prefix in 'local' is not what an extension
  // already has in a 'sync' which works, and the looking would tell of a
  // change the engine told of itself.
  const standInStorage = (polls) => {
    const storage = chrome.storage;
    if (!storage || !storage.local || typeof storage.local.get !== 'function') return;
    const PREFIX = '__vanilla_sync__:';
    const mine = (key) => typeof key === 'string' && key.startsWith(PREFIX);
    // a callback is answered the way the engine answers one.
    const done = (promise, callback) => {
      if (typeof callback !== 'function') return promise;
      promise.then((value) => { callback(value); },
                   (error) => { failing(new Error(String(error && error.message || error)), callback); });
      return undefined;
    };

    const everything = new Set(), each = { local: new Set(), session: new Set(), sync: new Set() };
    const listening = () => everything.size + each.local.size + each.session.size + each.sync.size;
    // whether anybody would hear of a change of 'area' ('sync' lives in 'local').
    const wanted = (area) => everything.size + each[area].size + (area === 'local' ? each.sync.size : 0) > 0;
    const tell = (changes, area) => {
      for (const f of Array.from(everything)) { try { f(changes, area); } catch (e) {} }
      for (const f of Array.from(each[area])) { try { f(changes); } catch (e) {} }
    };
    const report = (changes, area) => {
      if (area !== 'local') { tell(changes, area); return; }
      const synced = {}, rest = {};
      for (const k in changes) { if (mine(k)) synced[k.slice(PREFIX.length)] = changes[k]; else rest[k] = changes[k]; }
      if (Object.keys(rest).length) tell(rest, 'local');
      if (Object.keys(synced).length) tell(synced, 'sync');
    };

    const areas = ['local', 'session'].filter((a) => storage[a] && typeof storage[a].get === 'function');
    const read = {}, known = {}, asked = {}, taken = {};
    for (const area of areas) read[area] = storage[area].get.bind(storage[area]);
    const look = (area) => {
      const mine_ = asked[area] = (asked[area] || 0) + 1;
      let answer;
      try { answer = Promise.resolve(read[area](null)); } catch (e) { answer = Promise.reject(e); }
      return answer.then((now) => {
        if (mine_ < (taken[area] || 0)) return;
        taken[area] = mine_;
        const before = known[area], after = {}, changes = {};
        for (const k in now) {
          // (what has no JSON is kept as 'null': an 'undefined' here would
          // throw at the next look, and every look after it -- and so
          // would a value which throws right here.)
          let text;
          try { text = JSON.stringify(now[k]); } catch (e) {}
          after[k] = text === undefined ? 'null' : text;
          if (before && before[k] !== after[k]) changes[k] = (k in before) ? { oldValue: JSON.parse(before[k]), newValue: now[k] } : { newValue: now[k] };
        }
        if (before) for (const k in before) if (!(k in after)) changes[k] = { oldValue: JSON.parse(before[k]) };
        known[area] = after;
        if (Object.keys(changes).length) report(changes, area);
      }, () => {
        // a look which failed says nothing of what is there: taken for an
        // empty area, everything already in it would be told of as new at
        // the next look which did come back (invariant 5 -- and a ping
        // makes that easy to reach). The first look to COME BACK is what
        // later ones are compared with. 'session' alone is the exception,
        // and is meant to be: its refusal is the answer, because what is in
        // it once the worker opens it is what the page waits for (above).
        if (area === 'session' && !known[area]) known[area] = {};
      });
    };

    const begun = {};
    let timer = null, looks = 0;
    const visible = () => typeof document === 'undefined' || document.visibilityState === 'visible';
    const again = () => {
      timer = null;
      // (hidden, or with nobody listening, no timer is kept: 'visibilitychange'
      // and the next listener each set one.)
      if (!listening() || !visible()) return;
      for (const area of areas) if (wanted(area)) look(area);
      looks++;
      timer = setTimeout(again, looks < 12 ? 250 : looks < 24 ? 1000 : 3000);
    };
    const begin = () => {
      for (const area of areas) if (!begun[area] && wanted(area)) { begun[area] = true; look(area); }
      if (polls && timer === null && visible()) { looks = 0; timer = setTimeout(again, 250); }
    };
    if (polls && typeof document !== 'undefined') {
      // shown again, it is young again; and asks at once, unless the timer
      // from before it was hidden is still to come (three seconds at most).
      try {
        document.addEventListener('visibilitychange', () => {
          looks = 0;
          if (timer === null && listening() && visible()) timer = setTimeout(again, 0);
        });
      } catch (e) {}
    }
    // 'local' alone, and only where somebody would hear of it and the first
    // look of it has been taken: an area which is not read is not to begin
    // being read here either (D-352). 'session' is left out -- a content
    // script cannot write it, and what this context writes it is told of
    // already.
    const LOOK_AGAIN_EVERY = 2000;
    let lookedAgain = 0;
    lookAgain = () => {
      if (!begun.local || !wanted('local')) return;
      // one look for however many pings arrive together: a page links from
      // every frame it has, and they ping at once. The time is moved by
      // THIS look and by nothing else -- a context writing every few
      // seconds of its own (Vimium's worker does, to 'session') would
      // otherwise hold off the pings' look for as long as it kept writing.
      const at = Date.now();
      if (lookedAgain && at - lookedAgain < LOOK_AGAIN_EVERY) return;
      lookedAgain = at;
      look('local');
    };
    const eventOver = (listeners) => ({
      addListener(f) { if (typeof f === 'function') { listeners.add(f); begin(); } },
      removeListener(f) { listeners.delete(f); },
      hasListener(f) { return listeners.has(f); },
      hasListeners() { return listeners.size > 0; },
    });

    // what this context writes is told of as soon as it is written.
    for (const area of areas) {
      for (const verb of ['set', 'remove', 'clear']) {
        const real = storage[area][verb];
        if (typeof real !== 'function') continue;
        const wrapped = function (...args) {
          const callback = typeof args[args.length - 1] === 'function' ? args.pop() : null;
          let written;
          try { written = Promise.resolve(real.apply(storage[area], args)); } catch (e) { written = Promise.reject(e); }
          const told = written.then((value) => { if (begun[area]) look(area); return value; });
          return done(told, callback);
        };
        try { Object.defineProperty(storage[area], verb, { value: wrapped, configurable: true, writable: true, enumerable: true }); } catch (e) { refused.push('storage.' + area + '.' + verb); }
      }
      try { Object.defineProperty(storage[area], 'onChanged', { value: eventOver(each[area]), configurable: true, writable: true, enumerable: true }); } catch (e) { refused.push('storage.' + area + '.onChanged'); }
    }

    const local = storage.local;
    const strip = (items) => { const out = {}; for (const k in items) if (mine(k)) out[k.slice(PREFIX.length)] = items[k]; return out; };
    const under = (keys) => (Array.isArray(keys) ? keys : [keys]).map((k) => PREFIX + k);
    const sync = {
      get(keys, callback) {
        if (typeof keys === 'function') { callback = keys; keys = null; }
        let asking;
        if (keys === null || keys === undefined) asking = local.get(null).then(strip);
        else if (typeof keys === 'string' || Array.isArray(keys)) asking = local.get(under(keys)).then(strip);
        else { const defaults = {}; for (const k in keys) defaults[PREFIX + k] = keys[k]; asking = local.get(defaults).then(strip); }
        return done(asking, callback);
      },
      set(items, callback) { const prefixed = {}; for (const k in items) prefixed[PREFIX + k] = items[k]; return done(local.set(prefixed), callback); },
      remove(keys, callback) { return done(local.remove(under(keys)), callback); },
      clear(callback) { return done(local.get(null).then((all) => local.remove(Object.keys(all).filter(mine))), callback); },
      getBytesInUse(keys, callback) {
        if (typeof keys === 'function') callback = keys;
        return done(Promise.reject(new Error('chrome.storage.sync.getBytesInUse is not available in this browser')), callback);
      },
      onChanged: eventOver(each.sync),
      // Chrome's own numbers, as its documentation gives them, for an
      // extension which reads them to split what it writes. Not enforced.
      QUOTA_BYTES: 102400, QUOTA_BYTES_PER_ITEM: 8192, MAX_ITEMS: 512,
      MAX_WRITE_OPERATIONS_PER_HOUR: 1800, MAX_WRITE_OPERATIONS_PER_MINUTE: 120,
    };
    // (without asking the engine's own first: measured to refuse every call.
    // The day it does not, this is what hides an extension's real data.)
    try { Object.defineProperty(storage, 'sync', { value: sync, configurable: true, writable: true, enumerable: true }); made.push('storage.sync'); } catch (e) { refused.push('storage.sync'); }
    try { Object.defineProperty(storage, 'onChanged', { value: eventOver(everything), configurable: true, writable: true, enumerable: true }); made.push('storage.onChanged'); } catch (e) { refused.push('storage.onChanged'); }
  };
)js";

const char *STORAGE_LATCH = R"js(
  // A CONTENT SCRIPT does not ask the brand (D-379 addition 4, R-197 mid
  // 1): 'navigator.userAgentData' is a secure context's alone, and a
  // content script of an http page has none of it -- so the brand would
  // call that document the Qt engine while its own worker called itself
  // WebView2, and what the extension keeps in 'sync' would be split
  // between the copy's prefixed corner of 'local' here and the engine's
  // own store there.
  //
  // It asks the ENGINE instead, and not before the extension wants
  // something of it: the first call of 'storage.sync', or the first
  // listener of a change event, goes to the engine's own, and
  //   - one which ANSWERS keeps its 'sync' and its events, and nothing of
  //     ours ever stands in front of them;
  //   - one which refuses in the words it was measured to refuse with
  //     ('"sync" is not available in this instance of Chrome') is the
  //     engine the stand-in is for: it goes in, the call which found it out
  //     is made again through it, and what listened is moved onto the
  //     shim's own events -- off the engine's, which never fires.
  // Once, and never again: a document does not change engines.
  const latchStorage = (polls) => {
    const storage = chrome.storage;
    if (!storage || !storage.local || typeof storage.local.get !== 'function') return;
    let target = null;
    try { target = storage.sync; } catch (e) {}
    // with no 'sync' of the engine's there is nothing to try, and nothing
    // to hide: the stand-in goes in as it always did.
    if (!target || typeof target.get !== 'function' || typeof failing !== 'function') { standInStorage(polls); return; }
    // the stand-in will go HERE when it goes in, so an engine which will
    // not have it written is said so NOW, while the report is still being
    // made, and nothing is latched at all.
    try { Object.defineProperty(storage, 'sync', { value: target, configurable: true, writable: true, enumerable: true }); }
    catch (e) { refused.push('storage.sync'); return; }
    // the words this engine was measured to refuse with, and the only
    // answer which puts the stand-in in. A quota, a serialisation, or any
    // other failure of a 'sync' which WORKS is the extension's to see and
    // settles nothing.
    const refuses = (error) => {
      let said = '';
      try { said = String((error && error.message) || error || ''); } catch (e) {}
      return /is not available/i.test(said);
    };
    const VERBS = ['get', 'set', 'remove', 'clear', 'getBytesInUse'];
    const reals = {};
    for (const verb of VERBS) if (typeof target[verb] === 'function') reals[verb] = target[verb];
    let latched = '', stood = null, probing = false;
    // the events kept until the engine has answered: a listener goes on the
    // engine's own at once -- one which FIRES is not to be missed while
    // nobody has asked it anything -- and is remembered, so that it can be
    // moved to the shim's own if the engine turns out to be the one which
    // never fires.
    const wraps = [];
    // what a listener asks with, there being no call of its own to go by:
    // an empty read of the engine's 'sync', whose answer nobody wants.
    const probe = () => {
      if (latched || probing || !reals.get) return;
      probing = true;
      let answer;
      try { answer = Promise.resolve(reals.get.call(target, null)); } catch (e) { answer = Promise.reject(e); }
      answer.then(() => { toEngine(); }, (error) => { if (refuses(error)) toShim(); else probing = false; });
    };
    const wrap = (owner, get) => {
      if (!owner) return;
      let real = null;
      try { real = owner.onChanged; } catch (e) { return; }
      const kept = new Set();
      const event = {
        addListener(f) {
          if (typeof f !== 'function' || kept.has(f)) return;
          kept.add(f);
          try { if (real && typeof real.addListener === 'function') real.addListener(f); } catch (e) {}
          probe();
        },
        removeListener(f) {
          kept.delete(f);
          try { if (real && typeof real.removeListener === 'function') real.removeListener(f); } catch (e) {}
        },
        hasListener(f) { return kept.has(f); },
        hasListeners() { return kept.size > 0; },
      };
      try { Object.defineProperty(owner, 'onChanged', { value: event, configurable: true, writable: true, enumerable: true }); }
      catch (e) { refused.push('storage.onChanged'); return; }
      wraps.push({ owner, real, kept, get });
    };
    // the engine keeps its own. Every listener is on the engine's event
    // already, so the wrapper steps out of the way and is forgotten.
    const toEngine = () => {
      if (latched) return;
      latched = 'engine';
      for (const one of wraps) {
        try { Object.defineProperty(one.owner, 'onChanged', { value: one.real, configurable: true, writable: true, enumerable: true }); } catch (e) {}
        one.kept.clear();
      }
      wraps.length = 0;
    };
    const toShim = () => {
      if (latched) return;
      latched = 'shim';
      standInStorage(polls);
      try { stood = chrome.storage.sync; } catch (e) { stood = null; }
      if (stood === target) stood = null;
      for (const one of wraps) {
        let now = null;
        try { now = one.get(); } catch (e) {}
        for (const f of Array.from(one.kept)) {
          try { if (one.real && typeof one.real.removeListener === 'function') one.real.removeListener(f); } catch (e) {}
          try { if (now && typeof now.addListener === 'function') now.addListener(f); } catch (e) {}
        }
        one.kept.clear();
      }
      wraps.length = 0;
    };
    const asking = (verb, args) => {
      if (latched === 'shim' && stood) {
        const f = stood[verb];
        if (typeof f !== 'function') return Promise.reject(new Error('chrome.storage.sync.' + verb + ' is not available in this browser'));
        try { return Promise.resolve(f.apply(stood, args)); } catch (e) { return Promise.reject(e); }
      }
      let answer;
      try { answer = Promise.resolve(reals[verb].apply(target, args)); } catch (e) { answer = Promise.reject(e); }
      if (latched) return answer;
      return answer.then(
        (value) => { toEngine(); return value; },
        (error) => {
          // settled for the engine by another call meanwhile: this refusal
          // is the extension's to see. Settled for the stand-in -- by the
          // listener's probe, or a call made beside this one, whose refusal
          // came first -- it is the same refusal, and this call is made
          // again as the one which found it out is (Vimium listens and then
          // reads at once: its read was failed, and its settings with it).
          if (latched === 'engine' || !refuses(error)) throw error;
          toShim();
          // the call which found it out, made again through what went in.
          return asking(verb, args);
        });
    };
    for (const verb of VERBS) {
      if (!reals[verb]) continue;
      const wrapped = function (...args) {
        const callback = typeof args[args.length - 1] === 'function' ? args.pop() : null;
        // 'get(callback)' is everything, which is what 'get(null)' is: the
        // callback is off the end by now and would otherwise be no keys at
        // all.
        if (verb === 'get' && !args.length) args.push(null);
        const answer = asking(verb, args);
        if (!callback) return answer;
        answer.then((value) => { try { callback(value); } catch (e) {} },
                    (error) => { failing(new Error(String((error && error.message) || error)), callback); });
        return undefined;
      };
      try { Object.defineProperty(target, verb, { value: wrapped, configurable: true, writable: true, enumerable: true }); }
      catch (e) { refused.push('storage.sync'); return; }
    }
    wrap(storage, () => { try { return chrome.storage.onChanged; } catch (e) { return null; } });
    for (const area of ['local', 'session'])
      wrap(storage[area], ((a) => () => { try { return chrome.storage[a].onChanged; } catch (e) { return null; } })(area));
    made.push('storage.sync');
    made.push('storage.onChanged');
  };
  const installStorage = (polls) => {
    if (BY_BRAND) { if (!EDGE) standInStorage(polls); return; }
    latchStorage(polls);
  };
)js";

const char *FOLD = R"js(
  const replyOnce = (respond) => {
    let answered = false;
    return (value) => {
      if (answered) return;
      // (a throw is the engine refusing this answer -- the channel is
      //  gone, or something answered already. It does not close the gate
      //  on another listener, whose answer the engine may still take.)
      try { respond(value); answered = true; } catch (e) { try { console.error(e); } catch (x) {} }
    };
  };
  // every listener, in the order they came, and what Chrome makes of them
  // all. Chrome hands each its own 'sendResponse' and the first answer
  // wins; here they share one gate, and the first answer wins it.
  // EVERY promise is kept, not the first: a listener which passes on what
  // it does not handle, by being 'async' and returning, is the ordinary
  // shape in MV3, and the one that answers is as likely to be the second.
  const handAll = (list, message, sender, respond) => {
    let kept = false, only;
    const thenables = [];
    for (const f of list) {
      let said;
      try {
        said = f.call(undefined, message, sender, respond);
        // READING 'then' is reading a value that listener chose, so a
        // getter which throws is that listener's failure and nothing more
        // (R-141 A3): were it let out of here, the message would neither
        // be answered nor held, and whoever asked would wait for the
        // document to go (invariant 6).
        if (said === true) kept = true;
        else if (said && typeof said.then === 'function') thenables.push(said);
      } catch (e) { try { console.error(e); } catch (x) {} continue; }
      only = said;
    }
    return { kept, thenables, only, one: list.length === 1 };
  };
  // the one rule by which both paths end (D-356 addendum 4), so that a
  // document's first message and its later ones answer alike. A promise
  // which settles with a value answers, the first one there winning the
  // gate. One which settles with nothing, or breaks, only counts down:
  // the gate is closed with nothing once none of them answered AND nobody
  // kept the channel. One who returned 'true' and never answers holds it
  // open, as it does in Chrome -- and so does one whose promise never
  // settles. That is what a browser which waits on the returned promise
  // does, and not what this engine does with one (it closes the channel
  // in milliseconds: addendum 5); the cost is taken, and no deadline of
  // our own is put on these. The worker's LOOKUP_GIVE_UP bounds the host
  // lookup which came before, never them.
  // Each thenable counts once, however oddly it behaves: a made-up one
  // may call back and then throw, or call back twice, and a second count
  // would close the gate while another promise still means to answer.
  const decide = (said, once) => {
    let left = said.thenables.length;
    if (!left) { if (!said.kept) once(); return; }
    const counting = () => {
      let fired = false;
      return (value) => {
        if (fired) return;
        fired = true;
        left--;
        if (value !== undefined) once(value);
        else if (!left && !said.kept) once();
      };
    };
    for (const t of said.thenables) {
      const done = counting();
      try { t.then(done, () => done()); } catch (e) { done(); }
    }
  };
)js";

const char *ENVELOPE_WORKER = R"js(
  const installEnvelope = () => {
    if (!chrome.runtime || !chrome.runtime.onMessage) return;
    const real = chrome.runtime.onMessage, listeners = [];
    // which tab each named document is of, and what waits for the asking.
    // ('certain' in an entry is "ask nobody again", not "the host said so":
    //  a number the worker made for itself is certain too. 'chain' is which
    //  asking-again the entry belongs to, by the identity of an object.)
    const tabs = new Map(), waiting = new Map();
    const WAITS = [0, 30, 60, 120, 250, 500], LOOKUP_GIVE_UP = 3000, KEPT = 4096;
    // the documents a content script's shim has linked (S2b-5a), kept by the
    // identity of their port and never by a name: two documents may say the
    // same thing of themselves. What a link is is under 'joined'.
    const links = [], onConnects = [];
    const LINK_NAME = '__vanilla_link__', LINKS_KEPT = 4096, NAMING_WAIT = 5000;
    // how many unanswered questions one document may hold: a listener which
    // keeps the channel and never answers leaves its question until its
    // document goes, and a worker which does not sleep could be asked for
    // ever (R-141 A2).
    const PENDING_KEPT = 256;
    const NOBODY = 'Could not establish connection. Receiving end does not exist.';
    const CLOSED = 'The message port closed before a response was received.';
    // what a document is known by where it has no name of its own: the same
    // for the envelope it sends and for the port it opens, so that the two
    // are the same document to the worker.
    const keyOf = (nonce, frameId, tabUrl) => nonce || ('\u0000' + String(frameId) + '\u0000' + String(tabUrl));
    // a document which lost the race with '/bind' is asked after again, on
    // this timer and no other: never once per message, or the work the host
    // is given would follow how much the extension talks (D-356 addendum
    // 3). It backs off, and it stops: five rounds of six asks at most,
    // fifteen seconds of backing off and some twenty in all. What cannot be
    // named in that time -- a page with no stamp, a name the host has
    // forgotten, the child of an 'about:blank' -- is not to be named later
    // either (addendum 4).
    const BACKOFF = [1000, 2000, 4000, 8000], ROUNDS = 5;
    const asks = /^[0-9a-f]{64}$/.test(HOST_KEY) && typeof fetch === 'function';
    // kept away from the host's own numbers, which count up from a few: a
    // made-up one is not to be taken for a tab which exists.
    const randomTab = () => 0x40000000 + Math.floor(Math.random() * 0x40000000);
    // tabId comes from the host. frameId and tabUrl remain the content
    // script's account, and are not authoritative (outside S2b-2, D-356).
    // 'index' is READ from the table the host's answers fill, every time it
    // is asked for, because an extension keeps THIS object and reads it
    // long after (Vimium hands it on with 'Object.assign(request, { tab:
    // sender.tab })', and decides which tab to close by what it reads
    // then). One which copies the object instead takes the value of that
    // moment, as it would in Chrome. The cost: an extension which reads
    // 'index' with no 'tabs.query' before it sees where the tab was when
    // the host last said, and 0 for a tab the host never named. Keeping the
    // table by the events is S2b-5 (R-138 mid 1).
    const opened = (sender, from, tabId) => {
      const tab = { id: tabId };
      Object.defineProperty(tab, 'index', { get: () => indexes.has(tabId) ? indexes.get(tabId) : 0,
                                            enumerable: true, configurable: true });
      Object.assign(tab, { windowId: 1, url: from.tabUrl, title: '', active: true, highlighted: true,
                           pinned: false, incognito: false, status: 'complete', groupId: -1 });
      return Object.assign({}, sender, { frameId: from.frameId, documentLifecycle: 'active', tab });
    };
    // the same sender with no tab at all: what a page of this extension's
    // gets where the host cannot name it (D-370). 'opened' always makes a
    // tab, and a made-up one is not to be put on a document which may
    // truly be of no tab -- a popup is of none. The frame is the
    // envelope's, as it is above.
    const plain = (sender, from) => Object.assign({}, sender, { frameId: from.frameId, documentLifecycle: 'active' });
    const hand = (list, message, sender, respond) => {
      const once = replyOnce(respond);
      const said = handAll(list, message, sender, once);
      // with nothing pending the word goes to the engine as it is -- one
      // listener's is its own, several fold to whether any kept the channel
      // -- and the gate is NOT closed here. An engine told 'undefined'
      // closes the channel itself, and the sender then sees what it would
      // have without any shim: 'undefined' with a 'lastError', where an
      // answer of nothing would have reached it as 'null' (addendum 5).
      // What came in no envelope takes this path, and so passes through.
      if (!said.thenables.length) return said.one ? said.only : (said.kept ? true : undefined);
      // with something pending the shim is what waits, a lone listener's
      // promise as much as any: this engine does not wait on one, and the
      // sender is told the port closed within milliseconds (addendum 5).
      // Holding the channel with 'true' and answering from 'decide' is what
      // makes a document's first message and its later ones answer alike.
      decide(said, once);
      return true;
    };
    // what waited: the engine was told 'true' long ago, so whoever answers,
    // answers through this -- and if nobody will, it is closed with nothing.
    const late = (list, message, sender, respond) => {
      const once = replyOnce(respond);
      decide(handAll(list, message, sender, once), once);
    };
    // used now, so the oldest unused is the one which goes when the room
    // runs out: what an extension keeps talking about keeps its number.
    const recall = (key) => {
      const known = tabs.get(key);
      if (known) { tabs.delete(key); tabs.set(key, known); }
      return known;
    };
    const remember = (key, tabId, certain, chain) => {
      if (tabs.has(key)) tabs.delete(key);
      else if (tabs.size >= KEPT) tabs.delete(tabs.keys().next().value);
      tabs.set(key, { id: tabId, certain, chain });
      // the links of that document are to be reached by this number too:
      // one slot for a provisional number and one for a certain one, so
      // that a document reached by the first is still reached by the
      // second and neither grows. A number which is NEW to a document is
      // put down here and nowhere else; 'joined' only copies what is known
      // already -- from 'tabs', and from an older port of the same document
      // (R-142 mid 2, R-143).
      const piled = [];
      for (const link of links) if (link.key === key) {
        link.ids[certain ? 'certain' : 'provisional'] = tabId;
        if (link.nav.length || link.commit) piled.push(link);
      }
      // and what a document's navigations waited for a number for is
      // spoken of only once every slot is written, never while the list is
      // being walked: a listener is the extension's own code and may do
      // anything at all (S2b-5c).
      for (const link of piled) drainNav(link);
    };
    // an entry which has an asking-again behind it carries its chain between
    // rounds, the round carries it while it runs, and the round puts it
    // back: so a nonce has at most one chain asking after it, whatever the
    // room running out does in between. (One made up on the spot is certain
    // and carries none: nobody is to ask after it.)
    function settle(nonce, token, tabId, certain) {
      const pending = waiting.get(nonce);
      if (!pending || pending.token !== token) return;
      // 'once' is a round nobody is to ask after again, whatever it came
      // to: the top frame of a page of this extension's, which was not
      // waited for either (D-370). What it came to is what it keeps.
      const done = certain || !!pending.once;
      remember(nonce, tabId, done, pending.chain);
      waiting.delete(nonce);
      if (!done && pending.round + 1 < ROUNDS)
        setTimeout(() => again(nonce, pending.chain, pending.round + 1), BACKOFF[Math.min(pending.round, BACKOFF.length - 1)]);
      // with no number of the host's, a content script falls back to the
      // number the worker made ('tabId') and a page of this extension's to
      // no tab at all, which is 'null' here and nowhere else (D-370).
      for (const h of pending.held)
        late(h.list, h.message, tabId === null ? plain(h.sender, h.from) : opened(h.sender, h.from, tabId), h.respond);
    }
    // the asking again, which no message of anybody's started: gone,
    // answered for good, or taken over by a newer chain (the entry went and
    // a message began one), it does nothing. 'waiting.has' is a guard and
    // not a case of its own: a round settles once and settling schedules
    // one asking again, so no two of them carry the same chain, and the
    // only round a chain has waiting is the one this line would set.
    // Nothing was found which reaches it; were a way found, it belongs
    // written here.
    function again(nonce, chain, round) {
      const known = tabs.get(nonce);
      if (!known || known.certain || known.chain !== chain || waiting.has(nonce)) return;
      waiting.set(nonce, { held: [], token: null, round, chain });
      resolve(nonce, known.id);
    }
    // a round is at most one ladder of asks. 'token' is that round's, so a
    // deadline or an answer from an earlier one settles nothing here.
    function resolve(nonce, own) {
      const pending = waiting.get(nonce), token = {};
      if (!pending) return;
      pending.token = token;
      let attempt;
      const failed = (i) => {
        const now = waiting.get(nonce);
        if (!now || now.token !== token) return;
        if (i + 1 < WAITS.length) setTimeout(() => attempt(i + 1), WAITS[i + 1]);
        else settle(nonce, token, own, false);
      };
      attempt = (i) => {
        const now = waiting.get(nonce);
        if (!now || now.token !== token) return;
        // a Tab, never a bare number: the host answers 'vanilla.tabOf' with
        // '{id, index}', and what is not of that shape is not an answer --
        // the same as no answer at all, which is what the ladder is for.
        ask('vanilla.tabOf', [nonce]).then(
          (tab) => { if (isTab(tab)) settle(nonce, token, tab.id, true); else failed(i); },
          () => failed(i));
      };
      // EVERY round is bounded, not the first alone: a later one holds
      // messages too where the entry went while it ran, and what it holds
      // is to move even if no ask ever comes back. A settled round leaves
      // its deadline to find another token, or none, and do nothing.
      setTimeout(() => settle(nonce, token, own, false), LOOKUP_GIVE_UP);
      attempt(0);
    }
)js";

const char *RELAY_PORT = R"js(
  const RELAY_PAGE = 'vanilla_relay.html', RELAY_PORT_NAME = '__vanilla_relay__';
  const WANT_EVERY = 1000;
  let relayPort = null, wantedAt = 0;
  // the extension's own 'sendMessage', taken here rather than read at the
  // call: by then it may be the extension's own or the envelope's, and this
  // word is the shim's to its own page.
  const relaySend = (() => { try { return chrome.runtime && chrome.runtime.sendMessage; } catch (e) { return null; } })();
  // FIRE AND FORGET: there may be nobody listening, which this engine says
  // with a throw or with a rejection, and there is nothing to be done about
  // either -- the page connects when it loads, and tries again of itself
  // every few minutes. At most one a second, so that a burst of calls asks
  // once.
  wantRelay = () => {
    if (!RELAYED || relayPort || typeof relaySend !== 'function') return;
    let when = 0;
    try { when = Date.now(); } catch (e) {}
    if (wantedAt && when - wantedAt < WANT_EVERY) return;
    wantedAt = when || 1;
    try {
      const asked = relaySend.call(chrome.runtime, { __vanillaRelay: 'wanted' });
      if (asked && typeof asked.then === 'function') asked.then(() => {}, () => {});
    } catch (e) {}
  };
  const isRelay = (port) => {
    try {
      if (!RELAYED || !port || port.name !== RELAY_PORT_NAME) return false;
      const sender = port.sender;
      if (!sender || sender.id !== chrome.runtime.id) return false;
      // the ADDRESS and nothing else, which the engine proves and a page
      // cannot claim: that file is never web-accessible, so a document at
      // it is this extension's relay page and no other. Whether it is of a
      // tab says nothing -- measured on WebView2 runtime 153: the relay
      // page is given a tab of its own there ('TAB:vanilla_relay.html:...'
      // in 'runtime.getContexts', and the port's sender carried that
      // number), where the Qt engine's hidden page has none.
      return typeof chrome.runtime.getURL === 'function' && sender.url === chrome.runtime.getURL(RELAY_PAGE);
    } catch (e) { return false; }
  };
  const takeRelay = (port) => {
    if (!isRelay(port)) return false;
    // what goes over it: the ticket a call waits under, and the call. A
    // port which will not take one fails that call where it is sent.
    const send = (ticket, call) => { port.postMessage({ ticket, call }); };
    const said = (m) => {
      if (relayPort !== port || !m || typeof m !== 'object') return;
      if (!Number.isInteger(m.ticket) || !m.answer || typeof m.answer !== 'object') return;
      relayAnswer(m.ticket, m.answer);
    };
    const gone = () => {
      if (relayPort !== port) return;
      relayPort = null;
      // the transport is let go FIRST and what waited is failed after
      // (R-198). Failing a call gives this world the turn back, and a
      // round of the events which folds there and begins again would find
      // the dead port still attached: one send over it for nothing, which
      // throws where it is called and fails that call instead of leaving
      // it to wait for the next relay.
      setTransport(null);
      relayFail();
    };
    try {
      port.onMessage.addListener(said);
      port.onDisconnect.addListener(gone);
    } catch (e) { try { port.disconnect(); } catch (x) {} return true; }
    const older = relayPort;
    relayPort = port;
    if (older && older !== port) { try { older.disconnect(); } catch (e) {} relayFail(); }
    setTransport(send);
    return true;
  };
  // and once at the start, ahead of the extension's first line: the relay
  // page's port went when this worker was killed, and the page waits to be
  // asked rather than keep asking.
  wantRelay();
)js";

const char *ENVELOPE_LINK = R"js(
    /* --- the way to a content script: a port of the ENGINE's (S2b-5a) --- */
    // Nothing of this goes near the host: the engine itself says whose port
    // it is ('sender.id', which it will not let a page forge), a page's own
    // world has no 'chrome.runtime' to open one with, and another
    // extension's 'connect' does not reach 'onConnect' at all (it goes to
    // 'onConnectExternal': measured 2026-09-21). So a port whose name is
    // ours and whose sender is this extension is the shim's, and everything
    // else is the extension's own business.
    const shut = (port) => { try { port.disconnect(); } catch (e) {} };
    // done with a link told it is linked, whether a carrier's document is
    // wanted here at all, and what to do when room is made again: set by
    // the registered content scripts' piece (D-410). A worker which cannot
    // register (WebView2's, or one without the permission) tells a carrier
    // so, and that document stops trying (R-232 mid 1).
    let arrived = () => {}, carrying = false, relieved = () => {}, retired = () => {};
    // Whose tab a document is, asked after once and in one place: by the
    // greeting which wakes the worker, and by a port which names a document
    // this worker has never heard of -- a worker which was restarted knows
    // nothing, and a link with no number is nobody's destination. One chain
    // per nonce, bounded where D-356 addendum 4 left it; with nobody to ask
    // (no key, no name of its own) a number of the worker's own making is
    // put down, the same one the document's envelopes get.
    const look = (key, nonce) => {
      if (recall(key)) return;
      if (!nonce || !asks) { remember(key, randomTab(), true); return; }
      if (waiting.has(nonce)) return;
      waiting.set(nonce, { held: [], token: null, round: 0, chain: {} });
      resolve(nonce, randomTab());
    };
    // which numbers reach a link: either slot, so a document reached by the
    // provisional number is reached by the certain one as well. A slot
    // nobody has filled is 'null' and is reached by nothing: an extension
    // which asks for tab 0 -- Stands does, where it finds no current tab --
    // is to reach nobody, not everybody whose other slot is still empty
    // (R-141 A1). Nor is any number which is not a tab's.
    const reaches = (link, tabId) => Number.isInteger(tabId) && tabId > 0
                                     && (link.ids.provisional === tabId || link.ids.certain === tabId);
    // a link which goes takes its unanswered questions with it: whoever
    // waited is told nobody is there, which is not the same as being told
    // nothing (invariant 6: every 'tabs.sendMessage' ends).
    const retire = (link, gone) => {
      const at = links.indexOf(link);
      if (at >= 0) links.splice(at, 1);
      retired(link);
      if (!gone) shut(link.port);
      // what its navigations were waiting for a number for goes with it:
      // there is no document left for an event to be about.
      link.nav.length = 0;
      link.commit = null;
      const waited = Array.from(link.pending.values());
      link.pending.clear();
      for (const reply of waited) reply.gone();
      const running = Array.from(link.runs.values());
      link.runs.clear();
      for (const run of running) run.gone();
    };
    // what a port must say of its document with its first message, and
    // nothing else will do: an address it does not claim (the engine's own
    // 'port.sender.url' is what is believed), and 'since' is when its shim
    // began, which is the order frames are told of in.
    const wellNamed = (claim) => !!claim && typeof claim === 'object'
      && (claim.nonce === null || (typeof claim.nonce === 'string' && /^[0-9a-f]{32}$/.test(claim.nonce)))
      && Number.isInteger(claim.frameId) && claim.frameId >= 0
      && typeof claim.tabUrl === 'string'
      && typeof claim.since === 'number' && isFinite(claim.since);
    const joined = (port, claim) => {
      const nonce = claim.nonce || null, key = keyOf(nonce, claim.frameId, claim.tabUrl);
      let url = '';
      try { url = port.sender && typeof port.sender.url === 'string' ? port.sender.url : ''; } catch (e) {}
      // ('nav' is what its navigations wait in until it has a number of
      //  some kind: an event with no tabId is one no extension can use,
      //  and is not to be made up. S2b-5c.)
      const link = { port, key, nonce, frameId: claim.frameId, url, since: claim.since,
                     ids: { provisional: null, certain: null }, pending: new Map(), runs: new Map(), next: 0, nav: [], commit: null, readyAt: 0, given: new Set() };
      // what is known of that document already, in the same two slots.
      // (Read, not asked for: naming a port is no message of the
      //  extension's and does not touch the room's order.)
      const known = tabs.get(key);
      if (known) link.ids[known.certain ? 'certain' : 'provisional'] = known.id;
      // a nonce IS one document, so an older link of the same nonce is one
      // which has not noticed it was gone, and goes. A fallback key is not
      // a document's own -- two top frames of one address share it -- and
      // is no ground for retiring anybody, which would have them take turns
      // kicking each other out (R-140 mid 1). The content shim drops its
      // own port before it tries again, so this is the safety net.
      //
      // What the older link was reached BY comes across with it. The table
      // keeps one number for a document, the newest, so a document which
      // connects again would be left with that one alone -- and an
      // extension holding the provisional number would stop reaching it
      // (R-142 mid 2). What this link already has of its own stands.
      if (nonce) for (const other of links.slice()) if (other.nonce === nonce) {
        if (link.ids.provisional === null) link.ids.provisional = other.ids.provisional;
        if (link.ids.certain === null) link.ids.certain = other.ids.certain;
        retire(other);
      }
      links.push(link);
      return link;
    };
    // an answer is matched by the PAIR (port, number): a number counts up
    // in one link and is nobody else's, so another link's answer to it is
    // no answer at all.
    const fromLink = (link, m) => {
      if (!m || typeof m !== 'object') return;
      // what a run of ours came to (S2b-14): a value, or the error. A
      // style sheet put in or taken out (D-410) answers by the same table.
      const done = typeof m.ran === 'number' ? m.ran : typeof m.styled === 'number' ? m.styled : null;
      if (done !== null) {
        const run = link.runs.get(done);
        if (!run) return;
        link.runs.delete(done);
        if (typeof m.error === 'string') run.error(m.error);
        else run.value('value' in m ? m.value : undefined);
        return;
      }
      if (typeof m.re !== 'number') return;
      const reply = link.pending.get(m.re);
      if (!reply) return;
      link.pending.delete(m.re);
      if (m.nobody === 1) { reply.nobody(); return; }
      if ('value' in m) { reply.value(m.value); return; }
      reply.empty();
    };
    const taking = (port) => {
      if (links.length >= LINKS_KEPT) { shut(port); return; }
      let mine = null, over = false;
      const refuse = () => { over = true; shut(port); };
      const said = (m) => {
        if (over) return;
        // a named port says four kinds of thing and they are told apart
        // by their words: an answer to a question of ours ('re'), a
        // navigation of its own document's ('nav', S2b-5c), how far it is
        // loaded ('ready', D-452), and a ping, which says none of them and
        // is nothing but the message itself.
        if (mine) {
          if (m && typeof m === 'object' && m.nav !== undefined) { navigated(mine, m.nav); return; }
          // how far the document is loaded (D-452): those two words and
          // nothing else, so that no answer is taken for one (R-298 low 1).
          if (m && typeof m === 'object' && Object.keys(m).length === 2 && 'ready' in m && 'from' in m) { readied(mine, m.from, m.ready); return; }
          // a ping -- that word ALONE, so that a message which carries
          // anything else is none -- is when this worker looks at
          // 'storage.local' again (D-364). It is handled as it always was.
          if (m && typeof m === 'object' && m.ping === 1 && Object.keys(m).length === 1) { try { lookAgain(); } catch (e) {} }
          fromLink(mine, m);
          return;
        }
        if (!m || typeof m !== 'object' || !wellNamed(m.link)) { refuse(); return; }
        mine = joined(port, m.link);
        look(mine.key, mine.nonce);
        // the answer carries the address this worker believes the document
        // to be at -- the engine's 'port.sender.url', which does NOT follow
        // a 'pushState' (measured 2026-09-21: a document which pushed still
        // names itself to a new port by the address it was loaded at). The
        // content side needs it to know what this side is missing, and no
        // other side can tell it (R-149). '' where the engine said nothing.
        const told = { linked: 1, url: mine.url };
        if (m.link.carrier === 1) told.carrying = carrying ? 1 : 0;
        try { port.postMessage(told); }
        catch (e) { const dead = mine; mine = null; over = true; retire(dead); return; }
        arrived(mine);
        // a document's FIRST link is its commit, as far as this side can
        // know one (D-426): after 'linked' and after what is registered is
        // run in it, so a listener's own calls come behind both.
        if (mine && m.link.fresh === 1) committed(mine);
        // and after the commit, how far it was loaded when it named itself
        // (D-452).
        if (mine && m.link.ready !== undefined) readied(mine, m.link.readyFrom, m.link.ready);
      };
      try {
        port.onMessage.addListener(said);
        port.onDisconnect.addListener(() => { over = true; if (mine) { const dead = mine; mine = null; retire(dead, true); } });
      } catch (e) { refuse(); return; }
      // a port which never names itself is nobody's document.
      setTimeout(() => { if (!over && !mine) refuse(); }, NAMING_WAIT);
    };
)js";

const char *ENVELOPE_LINK_SEND = R"js(
    // 'tabs.sendMessage' to every document it is for at once. It ends --
    // always, invariant 6 -- by these rules and no others: the first answer
    // WITH a value wins and the rest are forgotten; a listener which was
    // there and answered nothing leaves 'undefined'; nobody there at all is
    // Chrome's 'Receiving end does not exist'; and a lone document which
    // went before it answered is Chrome's 'The message port closed'.
    // One who returned 'true' and never answers holds it open until its
    // document goes, as it does in Chrome.
    const sendTo = (targets, message) => new Promise((resolve, reject) => {
      if (!targets.length) { reject(new Error(NOBODY)); return; }
      const mine = [], only = targets.length === 1;
      let left = targets.length, over = false, spoke = false, closed = false;
      const forget = () => { for (const e of mine) e.link.pending.delete(e.id); };
      const value = (v) => { if (over) return; over = true; forget(); resolve(v); };
      const count = () => {
        if (over || --left > 0) return;
        over = true;
        forget();
        if (spoke) resolve(undefined);
        else if (closed && only) reject(new Error(CLOSED));
        else reject(new Error(NOBODY));
      };
      for (const link of targets) {
        // a document with no room left is counted below, the same as one
        // which went before it answered: it is not asked, and nothing is
        // left waiting on it.
        if (link.pending.size >= PENDING_KEPT) continue;
        const id = ++link.next;
        mine.push({ link, id });
        link.pending.set(id, { value, empty: () => { spoke = true; count(); },
                               nobody: () => { count(); },
                               gone: () => { closed = true; count(); } });
      }
      for (let full = targets.length - mine.length; full > 0; full--) { closed = true; count(); }
      for (const entry of mine) {
        if (over) break;
        try { entry.link.port.postMessage({ id: entry.id, message }); }
        catch (e) { if (entry.link.pending.delete(entry.id)) { closed = true; count(); } }
      }
    });
    const sendMessage = (...args) => {
      const callback = typeof args[args.length - 1] === 'function' ? args.pop() : null;
      const tabId = args[0], message = args[1], options = args.length > 2 ? args[2] : null;
      const asked = options && typeof options === 'object' ? options : null;
      // what was asked for and cannot be answered is said so, never read as
      // something else: a 'frameId' of '0' (a string) read as "no frameId"
      // would turn one frame's message into every frame's (R-141 A4).
      const named = asked && asked.frameId !== undefined && asked.frameId !== null;
      let answer;
      if (asked && asked.documentId !== undefined)
        answer = Promise.reject(new Error('chrome.tabs.sendMessage does not take a documentId in this browser'));
      else if (named && !(Number.isInteger(asked.frameId) && asked.frameId >= 0))
        answer = Promise.reject(new Error('chrome.tabs.sendMessage: frameId must be a whole number of zero or more'));
      else {
        // one frame if one was asked for, else every document of that tab
        // -- the sending frame's own among them, which is what a hint
        // coordinator counts on hearing back from (Vimium).
        const frameId = named ? asked.frameId : null;
        answer = sendTo(links.filter((l) => reaches(l, tabId) && (frameId === null || l.frameId === frameId)), message);
      }
      return callback ? toCallback(answer, callback) : answer;
    };
    const getAllFrames = (...args) => {
      const callback = typeof args[args.length - 1] === 'function' ? args.pop() : null;
      const details = args[0], tabId = details && typeof details === 'object' ? details.tabId : undefined;
      // Chrome takes a tabId and nothing else. An empty list for want of
      // one would read as "that tab has no frames", which is an answer
      // nobody gave (R-141 A4).
      if (!Number.isInteger(tabId)) {
        const error = new Error('chrome.webNavigation.getAllFrames: a tabId is required');
        if (callback) { failing(error, callback); return undefined; }
        return Promise.reject(error);
      }
      const mine = links.filter((l) => reaches(l, tabId));
      // the top first, the rest oldest shim first (an extension which walks
      // to the next frame walks this order). A real 'frameId' is not to be
      // had, and neither is a grandchild's real parent: every frame but the
      // top is said to be the top's child, which is KNOWN to be wrong for a
      // nested one (the two extensions read 'frameId' and 'url' alone).
      const frames = mine.filter((l) => l.frameId === 0)
        .concat(mine.filter((l) => l.frameId !== 0).sort((a, b) => a.since - b.since))
        .map((l) => ({ frameId: l.frameId, parentFrameId: l.frameId === 0 ? -1 : 0, url: l.url,
                       errorOccurred: false, processId: -1, documentLifecycle: 'active',
                       frameType: l.frameId === 0 ? 'outermost_frame' : 'sub_frame' }));
      const answer = Promise.resolve(frames);
      if (!callback) return answer;
      answer.then((result) => { try { callback(result); } catch (e) {} });
      return undefined;
    };
    const installLink = () => {
      own.set('tabs.sendMessage', sendMessage);
      own.set('webNavigation.getAllFrames', getAllFrames);
      // with no 'onConnect' nothing can link, and 'tabs.sendMessage' fails
      // with 'nobody there' as it would for a tab which has no content
      // script. Nothing else changes.
      const realConnect = chrome.runtime.onConnect;
      if (!realConnect || typeof realConnect.addListener !== 'function') return;
      realConnect.addListener((port) => {
        // the relay page's port first, which is no more the extension's to
        // see than a link is (E-2b-1). On the Qt engine this takes nothing.
        if (takeRelay(port)) return;
        let ours = false;
        try { ours = !!port && port.name === LINK_NAME && !!port.sender && port.sender.id === chrome.runtime.id; } catch (e) {}
        if (ours) { taking(port); return; }
        const list = onConnects.slice();
        if (!list.length) { shut(port); return; }
        for (const f of list) { try { f.call(undefined, port); } catch (e) { try { console.error(e); } catch (x) {} } }
      });
      const onConnect = listenedTo(onConnects);
      try { Object.defineProperty(chrome.runtime, 'onConnect', { value: onConnect, configurable: true, writable: true, enumerable: true }); made.push('runtime.onConnect'); }
      catch (e) { refused.push('runtime.onConnect'); }
    };
)js";

const char *ENVELOPE_WORKER_NAV = R"js(
    /* --- what a document did to its own history (S2b-5c) --- */
    const NAV_KEPT = 8, NAV_URL_MAX = 8192;
    const NAV_HISTORY = 'webNavigation.onHistoryStateUpdated', NAV_FRAGMENT = 'webNavigation.onReferenceFragmentUpdated';
    const NAV_COMMITTED = 'webNavigation.onCommitted', COMMITS_KEPT = 64;
    // the two a document tells of its own loading (D-452), and the one no
    // document here ever has to tell of: a tab is a node of the tree, and
    // its number is never handed to another.
    const NAV_LOADED = 'webNavigation.onDOMContentLoaded', NAV_COMPLETED = 'webNavigation.onCompleted';
    const NAV_REPLACED = 'webNavigation.onTabReplaced', READIES_KEPT = 64;
    // the nonces whose commit was told, oldest first (D-426).
    const commits = [];
    // and how far each document's loading was told, by nonce, the last
    // sixty-four (D-452).
    const readies = new Map();
    const navListeners = new Map();
    navListeners.set(NAV_HISTORY, []);
    navListeners.set(NAV_FRAGMENT, []);
    navListeners.set(NAV_COMMITTED, []);
    navListeners.set(NAV_LOADED, []);
    navListeners.set(NAV_COMPLETED, []);
    navListeners.set(NAV_REPLACED, []);
    const stamp = () => { try { return Date.now(); } catch (e) { return 0; } };
    // the scheme and the authority, and nothing which has none of its own:
    // 'about:', 'blob:' and 'data:' match nothing at all, a blob's 'new
    // URL(...).origin' being the origin it INHERITED and not what the
    // engine proved of this document. Nor is an origin the browser itself
    // calls 'null' one to match with.
    //
    // Where there is no 'URL' to ask, only the two schemes whose origin
    // can be read off the text are read: the authority of any other
    // ('foo://a/x') is no origin at all to a browser, and taking it for one
    // would have two such match here where nothing matches there
    // (R-149 low 1). Comparing the text does tell 'https://a.example:443'
    // from 'https://a.example', which 'URL' does not -- and a mismatch is
    // the side to be wrong on.
    const HAS_HOST = /^[a-z][a-z0-9+.-]*:\/\/[^/?#]+/;
    const WEB_HOST = /^https?:\/\/[^/?#]+/;
    const originOf = (text) => {
      if (typeof text !== 'string' || !HAS_HOST.test(text.toLowerCase())) return null;
      if (typeof URL === 'function') {
        try {
          const one = new URL(text).origin;
          return typeof one === 'string' && one && one !== 'null' ? one : null;
        } catch (e) { return null; }
      }
      const web = WEB_HOST.exec(text.toLowerCase());
      return web ? web[0] : null;
    };
    const sameOrigin = (a, b) => {
      const one = originOf(a), two = originOf(b);
      return one !== null && one === two;
    };
    // which number this document is reached by, the certain one first: an
    // event of the provisional one is an event whose tabId later changes,
    // which is the known property a port's two slots have always had
    // (R-142 mid 2).
    const whole = (which) => Number.isInteger(which) && which > 0;
    const numberOf = (link) => whole(link.ids.certain) ? link.ids.certain
                             : (whole(link.ids.provisional) ? link.ids.provisional : null);
    // 'at' is when the document SAID it, which is not when this is handed
    // out: what waits for a number waits as long as the host takes
    // (R-148 low 4).
    const fire = (name, tabId, link, url, at) => {
      // 'parentFrameId' and 'frameType' by the rule 'getAllFrames' goes
      // by, and wrong for a grandchild frame in the same way. NO
      // 'transitionType' and no 'transitionQualifiers': what is not known
      // is not invented (Stands keeps the 'undefined' it reads).
      const details = { tabId, frameId: link.frameId, url, timeStamp: at, processId: -1,
                        parentFrameId: link.frameId === 0 ? -1 : 0,
                        frameType: link.frameId === 0 ? 'outermost_frame' : 'sub_frame',
                        documentLifecycle: 'active' };
      // the listeners of this moment, copied: one which a listener adds
      // does not hear of the event it was added from, and one it removes
      // still does (the rule 'demux' and 'deliver' both keep).
      for (const f of navListeners.get(name).slice()) {
        try { f.call(undefined, details); } catch (e) { try { console.error(e); } catch (x) {} }
      }
    };
    // what waited for a number, in the order it was heard. Called where
    // 'remember' writes a link's slots and nowhere else, so that 'remember'
    // is still the one place a document's new number is put down. (Nothing
    // a listener of ours can do retires a link -- only the engine, through
    // a port's message or its going -- so what was collected to be drained
    // is still a link of this list.)
    const drainNav = (link) => {
      if (!link.nav.length && !link.commit) return;
      const tabId = numberOf(link);
      if (tabId === null) return;
      // the commit first: it came before anything the document did with
      // its history, and it waits in a place of its own, which no number
      // of those can push out (R-241 mid 1). Its nonce is put down as it
      // is HANDED OUT, so one thrown away with a retired link is not taken
      // for one told (R-241 low 3).
      if (link.commit) {
        const one = link.commit;
        link.commit = null;
        if (link.nonce) {
          commits.push(link.nonce);
          if (commits.length > COMMITS_KEPT) commits.shift();
        }
        fire(NAV_COMMITTED, tabId, link, one.url, one.at);
      }
      if (!link.nav.length) return;
      const waited = link.nav.slice();
      link.nav.length = 0;
      for (const one of waited) {
        // a stage of the loading is put down as it is handed out, as a
        // commit is (D-452).
        if (one.stage && link.nonce) {
          readies.delete(link.nonce);
          readies.set(link.nonce, one.stage);
          if (readies.size > READIES_KEPT) readies.delete(readies.keys().next().value);
        }
        fire(one.name, tabId, link, one.url, one.at);
      }
    };
    // what waits for a number: the document's own navigations eight at
    // most, the oldest of THOSE going when the room runs out, and the two
    // stages of its loading, which none of them pushes out (D-452 addendum
    // 1). One pile, so that what comes out comes out in the order it was
    // heard in.
    const moved = (one) => !one.stage;
    const pile = (link, one) => {
      if (moved(one) && link.nav.filter(moved).length >= NAV_KEPT) link.nav.splice(link.nav.findIndex(moved), 1);
      link.nav.push(one);
    };
    const navigated = (link, nav) => {
      // a link which has been retired speaks for nobody, whatever number
      // it still holds: its port was closed for a newer one of the same
      // nonce and may not have noticed. This is the one place that is
      // asked, so nothing is written to such a link and nothing is piled
      // up for it either.
      if (links.indexOf(link) < 0) return;
      if (!nav || typeof nav !== 'object') return;
      const name = nav.kind === 'history' ? NAV_HISTORY : nav.kind === 'fragment' ? NAV_FRAGMENT : null;
      const url = nav.url;
      // a shape which is none of ours, or an address of another origin or
      // of no origin at all, is that message dropped -- and the link
      // stays: a document is not turned out for saying something which
      // made no sense (it is not the port which is in doubt; the engine
      // named it).
      if (!name || typeof url !== 'string' || !url || url.length > NAV_URL_MAX) return;
      if (!sameOrigin(link.url, url)) return;
      // the address is the ONE thing this writes: 'key', 'nonce',
      // 'frameId', 'since', 'ids' and 'pending' are not the document's to
      // say. So 'getAllFrames' tells of where the document is NOW, which
      // it could not do before (D-359's known inaccuracy).
      link.url = url;
      const list = navListeners.get(name);
      if (!list.length) return;
      // when it was HEARD, kept with what waits: the clock of the moment a
      // pile is handed out is the clock of the host's answer, which is no
      // time the document navigated at (R-148 low 4).
      const at = stamp();
      // piled up FIRST and looked at after: what has no number yet is
      // nothing an extension could use, and the pile is what keeps the
      // order for when one arrives (R-147 mid 5). The oldest goes when the
      // room runs out; nothing waits on a timer.
      pile(link, { name, url, at });
      drainNav(link);
    };
    // how far a document is loaded, as it tells of it (D-452): 'to' is the
    // stage it is at -- 1 when its DOM is in, 2 when it has loaded -- and
    // 'from' the stage it had told of before, so that only what lies
    // between is made (a nonce which has gone out of the table is not
    // told of the DOM a second time: R-298 mid 3). Each stage once for a
    // document of the last sixty-four, and only where somebody listens at
    // the moment it is heard; the address is the one the link holds.
    const readied = (link, from, to) => {
      if (links.indexOf(link) < 0) return;
      if ((from !== 0 && from !== 1) || (to !== 1 && to !== 2) || from >= to) return;
      const url = link.url;
      if (typeof url !== 'string' || !url || url.length > NAV_URL_MAX) return;
      const told = Math.max(from, link.readyAt, (link.nonce && readies.get(link.nonce)) || 0);
      if (to > link.readyAt) link.readyAt = to;
      const at = stamp();
      for (let stage = told + 1; stage <= to; stage++) {
        const name = stage === 1 ? NAV_LOADED : NAV_COMPLETED;
        if (navListeners.get(name).length) pile(link, { name, url, at, stage });
      }
      drainNav(link);
    };
    // a document's first link (D-426): the content side says 'fresh' until
    // it is told it is linked, so a document is committed once for its
    // life -- and a claim said again because 'linked' was lost is known by
    // its nonce, the last sixty-four told (a document with no nonce is not
    // known again, and may be told twice). The address is the ENGINE's
    // ('port.sender.url', where the document was loaded), not the page's.
    const committed = (link) => {
      if (links.indexOf(link) < 0) return;
      if (link.nonce && commits.includes(link.nonce)) return;
      const url = link.url;
      if (typeof url !== 'string' || !url || url.length > NAV_URL_MAX) return;
      if (!navListeners.get(NAV_COMMITTED).length) return;
      link.commit = { url, at: stamp() };
      drainNav(link);
    };
    // said once: a listener which was not registered is not to be reported
    // again for every frame of every page.
    let filtered = false;
    const navEvent = (name) => {
      const list = navListeners.get(name);
      return {
        // a URL filter (the second argument) cannot be evaluated here, and
        // a listener taken as though it had been would be handed events it
        // did not ask for. So it is NOT registered, and 'hasListener' says
        // so. 'undefined' and 'null' are "no filter" and are registered --
        // which is what the two extensions hand over.
        addListener(f, filters) {
          if (typeof f !== 'function' || list.includes(f)) return;
          if (filters !== undefined && filters !== null) {
            if (!filtered) {
              filtered = true;
              try { console.warn('Vanilla: the compatibility layer cannot evaluate a URL filter, so a listener of chrome.' + name + ' was not registered'); } catch (e) {}
            }
            return;
          }
          list.push(f);
        },
        removeListener(f) { const at = list.indexOf(f); if (at >= 0) list.splice(at, 1); },
        hasListener(f) { return list.includes(f); },
        hasListeners() { return list.length > 0; },
        // (the stand-in has these, so an extension which calls one of them
        //  is not failed by the event's becoming real. No rule is kept.)
        addRules() {}, removeRules() {}, getRules() {},
      };
    };
    own.set(NAV_HISTORY, navEvent(NAV_HISTORY));
    own.set(NAV_FRAGMENT, navEvent(NAV_FRAGMENT));
    own.set(NAV_COMMITTED, navEvent(NAV_COMMITTED));
    own.set(NAV_LOADED, navEvent(NAV_LOADED));
    own.set(NAV_COMPLETED, navEvent(NAV_COMPLETED));
    // (kept, and never fired: D-452.)
    own.set(NAV_REPLACED, navEvent(NAV_REPLACED));
)js";

const char *ENVELOPE_WORKER_RUN = R"js(
    // (the sizes are in UTF-16 code units, which is what a string's
    //  'length' counts. 4 MiB of code is above what a port was measured
    //  to carry (3 MiB, D-359) and well above what SingleFile sends
    //  (1.1 MiB for its nine files); what waits on one link is bounded
    //  in bytes as well as in number.)
    const RUN_WAIT = 30000, FILES_KEPT = 64, FILES_BYTES = 16 * 1024 * 1024, CODE_LIMIT = 4 * 1024 * 1024, LINK_BYTES = 8 * 1024 * 1024;
    const files = new Map(), reading = new Map();
    let filesBytes = 0;
    // the text of a function as the engine writes it, kept here while
    // nothing of the extension's has run: the function's own 'toString' is
    // never asked, as what it says of itself could be other code than the
    // one being injected, or could throw.
    const functionText = Function.prototype.toString;
    const fileRoot = () => 'chrome-extension://' + chrome.runtime.id + '/';
    // a segment the URL parser reads as a step upwards, which is written
    // more ways than one: '..', and the same with either dot
    // percent-encoded. One decoding is what the parser compares, so a
    // '%252e%252e' is a segment of that name and no step at all.
    const upward = (s) => { let one = s; try { one = decodeURIComponent(s); } catch (e) {} return one === '..'; };
    // the name a file is read and kept under, or null for a name which is
    // no file of this extension's. The name given is asked segment by
    // segment BEFORE the engine resolves it (it resolves a '%2e%2e' away,
    // leaving nothing to look at), and what the engine made of it must
    // still be under this extension's own root: that is what turns away
    // '//x.js', which names another host, and an address of anywhere else.
    // The resolved name is what the reading is kept under, so './a/x.js'
    // and 'a/x.js' are one file and read once.
    const filePath = (f) => {
      if (typeof f !== 'string' || !f.length || f.includes('\\') || f.slice(0, 2) === '//') return null;
      if (/^[a-z][a-z0-9+.-]*:/i.test(f) || f.split('/').some(upward)) return null;
      const root = fileRoot();
      let url = null;
      try { if (typeof chrome.runtime.getURL === 'function') url = chrome.runtime.getURL(f); } catch (e) {}
      if (typeof url !== 'string') url = root + f.replace(/^\/+/, '');
      if (url.slice(0, root.length) !== root) return null;
      const path = url.slice(root.length);
      if (!path.length || path.includes('\\') || path.split('/').some(upward)) return null;
      return path;
    };
    // a file is read once at a time: a second ask while it is on its way
    // waits for the same reading. What could not be read is not kept.
    const readFile = (f) => {
      if (files.has(f)) return Promise.resolve(files.get(f));
      if (reading.has(f)) return reading.get(f);
      const unreadable = () => new Error("Could not load file: '" + f + "'.");
      let asking;
      try { asking = Promise.resolve(fetch(fileRoot() + f)); } catch (e) { return Promise.reject(unreadable()); }
      const read = asking.then((r) => { if (!r || !r.ok) throw 0; return r.text(); }).then((text) => {
        if (typeof text !== 'string') throw 0;
        if (text.length <= FILES_BYTES) {
          while (files.size && (files.size >= FILES_KEPT || filesBytes + text.length > FILES_BYTES)) {
            const oldest = files.keys().next().value;
            filesBytes -= files.get(oldest).length;
            files.delete(oldest);
          }
          files.set(f, text);
          filesBytes += text.length;
        }
        return text;
      }, () => { throw unreadable(); });
      reading.set(f, read);
      const forget = () => { if (reading.get(f) === read) reading.delete(f); };
      read.then(forget, forget);
      return read;
    };
    const REMOVED = 'The frame was removed.';
    // what waits on a link, answers and runs together, is bounded in
    // number and in bytes; a link which has no room is one which cannot
    // be run in now.
    const inFlight = (link) => { let bytes = 0; link.runs.forEach((run) => { bytes += run.bytes; }); return bytes; };
    // and over every link together (R-230 mid 4): a page which makes frames
    // by the hundred is not to have this worker post to each of them the
    // whole of what was registered, in one turn. 'flying' is the bytes of
    // every run and every style sheet on their way, over all the links.
    const ALL_BYTES = 32 * 1024 * 1024;
    let flying = 0;
    // one thing sent over a link and waited for: a run of code (S2b-14) or
    // a style sheet (D-410), which answer by the one table 'runs', so that
    // a link going ends the waiting for either (R-230 mid 5).
    // (a refusal for want of room carries a mark the registered scripts
    //  read, so that such a document is tried again when room is made:
    //  R-232 mid 3. To a caller it reads as the frame gone, as before.)
    const waitOn = (link, bytes, message) => new Promise((resolve, reject) => {
      if (link.pending.size + link.runs.size >= PENDING_KEPT || inFlight(link) + bytes > LINK_BYTES || flying + bytes > ALL_BYTES) {
        reject(Object.assign(new Error(REMOVED), { starved: true }));
        return;
      }
      const id = ++link.next;
      let timer = null, over = false;
      flying += bytes;
      // (once, whichever way it ends: 'retire' empties the table itself
      //  before it says 'gone', so the bytes are given back here and not
      //  by what the table still holds.)
      const done = () => {
        if (over) return;
        over = true;
        link.runs.delete(id);
        flying -= bytes;
        try { if (timer !== null) clearTimeout(timer); } catch (e) {}
        relieved();
      };
      link.runs.set(id, { bytes,
                          value: (v) => { done(); resolve(v); },
                          error: (e) => { done(); reject(new Error(e)); },
                          gone: () => { done(); reject(new Error(REMOVED)); } });
      timer = setTimeout(() => { if (link.runs.has(id)) { done(); reject(new Error(REMOVED)); } }, RUN_WAIT);
      try { link.port.postMessage(message(id)); }
      catch (e) { done(); reject(new Error(REMOVED)); }
    });
    const runIn = (link, codes, extra) => waitOn(link, codes.reduce((n, c) => n + c.length, 0),
                                                 (id) => Object.assign({ run: id, codes }, extra || {}));
    const styleIn = (link, change) => waitOn(link, (typeof change.add === 'string' ? change.add : change.remove).length,
                                             (id) => Object.assign({ style: id }, change));
    // the links a target names: the top frame alone unless 'allFrames' or
    // 'frameIds' say otherwise, every frame looked for before anything is
    // sent to any, the top first and the rest in the order they began.
    const targetsOf = (target, tabId, bad) => {
      if (target.allFrames === true && target.frameIds !== undefined)
        throw bad("Cannot specify both 'allFrames' and 'frameIds'.");
      const mine = links.filter((l) => reaches(l, tabId));
      let targets;
      if (target.allFrames === true) targets = mine.slice();
      else if (target.frameIds !== undefined) {
        if (!Array.isArray(target.frameIds) || !target.frameIds.length || !target.frameIds.every((f) => Number.isInteger(f) && f >= 0))
          throw bad("Property 'target.frameIds': expected a list of frame ids.");
        targets = [];
        for (const f of target.frameIds) {
          // the NEWEST document of that frame: one which has just come may
          // stand beside the one it replaced until that one's port is seen
          // to go, and the new one is what a commit speaks of (R-241 mid 3).
          // Newest by when its shim began, not by when it linked: an old
          // document which links again joins the list at its end (R-242
          // low 3). A tie goes to the later link.
          const link = mine.filter((l) => l.frameId === f).reduce((a, b) => (a && a.since > b.since ? a : b), null);
          if (!link) throw new Error('No frame with id ' + f + ' in tab with id ' + tabId + '.');
          if (!targets.includes(link)) targets.push(link);
        }
      } else targets = mine.filter((l) => l.frameId === 0);
      if (!targets.length) throw new Error(mine.length ? 'No frame with id 0 in tab with id ' + tabId + '.' : 'No tab with id: ' + tabId + '.');
      targets.sort((a, b) => (a.frameId === 0 ? -1 : b.frameId === 0 ? 1 : a.since - b.since));
      return targets;
    };
    // one target: its failure is the call's. More than one: the ones which
    // failed are left out, and every one of them failing is an answer of
    // nothing rather than a failure (Chrome's rule).
    const gathered = (targets, one) => Promise.all(targets.map((link) => one(link).then((value) => ({ value }), (error) => ({ failed: error }))))
      .then((outcomes) => {
        if (outcomes.length === 1 && outcomes[0].failed) throw outcomes[0].failed;
        return outcomes.filter((o) => !o.failed).map((o) => o.value);
      });
    const executeScript = (injection, callback) => {
      const answer = Promise.resolve().then(() => {
        const bad = (what) => new Error('Invalid value for argument 1. ' + what);
        if (!injection || typeof injection !== 'object' || !injection.target || typeof injection.target !== 'object') throw bad("Property 'target': expected an object.");
        const target = injection.target, tabId = target.tabId;
        if (!Number.isInteger(tabId)) throw new Error('No tab with id: ' + String(tabId) + '.');
        const hasFiles = injection.files !== undefined, hasFunc = injection.func !== undefined;
        if (hasFiles === hasFunc) throw new Error("Exactly one of 'files' and 'func' must be specified.");
        if (hasFiles && (!Array.isArray(injection.files) || !injection.files.length || !injection.files.every((f) => filePath(f) !== null)))
          throw bad("Property 'files': expected a list of paths inside the extension.");
        if (hasFunc && typeof injection.func !== 'function') throw bad("Property 'func': expected a function.");
        // its text, read here and not through the function itself. One
        // whose text cannot be read at all is refused before anything is
        // sent, in the words of an invalid argument (what Chrome says of
        // such a one is unmeasured).
        let text = '';
        if (hasFunc) {
          try { text = functionText.call(injection.func); } catch (e) { text = null; }
          if (typeof text !== 'string') throw bad("Property 'func': expected a function.");
        }
        if (injection.world !== undefined && injection.world !== 'ISOLATED')
          throw new Error(injection.world === 'MAIN' ? 'world MAIN is not available in this browser' : "Invalid value for argument 1. Property 'world'.");
        // the arguments as Chrome hands them: each one JSON, put into the
        // call as it is written ('(f)(1, "a")'), and nothing which JSON
        // has no word for, however deep.
        let args = '';
        if (injection.args !== undefined) {
          if (!hasFunc) throw new Error("'args' may only be specified with 'func'.");
          if (!Array.isArray(injection.args)) throw bad("Property 'args': expected an array.");
          try {
            const list = JSON.stringify(injection.args, (key, value) => {
              if (typeof value === 'function' || typeof value === 'symbol' || typeof value === 'bigint' || value === undefined) throw 0;
              return value;
            });
            if (typeof list !== 'string') throw 0;
            args = list.slice(1, -1);
          } catch (e) { throw bad("Property 'args': must be JSON-serializable."); }
        }
        const targets = targetsOf(target, tabId, bad);
        // the code, one piece per file (each its own script, as Chrome runs
        // them), or the one call of the function.
        const pieces = hasFunc
          ? Promise.resolve(['(' + text + ')(' + args + ')'])
          : Promise.all(injection.files.map((f) => readFile(filePath(f))));
        return pieces.then((codes) => {
          if (codes.reduce((n, c) => n + c.length, 0) > CODE_LIMIT) throw new Error('The script is too large to run.');
          return gathered(targets, (link) => runIn(link, codes).then(
            (value) => { const one = { frameId: link.frameId }; if (value !== undefined) one.result = value; return one; }));
        });
      });
      return typeof callback === 'function' ? toCallback(answer, callback) : answer;
    };
    // only for an extension whose manifest asks for 'scripting': the
    // others keep the stand-in which fails, as Chrome keeps the namespace
    // from them.
    let scripting = false;
    try { scripting = (chrome.runtime.getManifest().permissions || []).includes('scripting'); } catch (e) {}
    if (engineScripting.size) {
      // WebView2's own, with the tab looked up again (E-3b, D-386): the
      // link's 'executeScript' above is not put in, and neither writes
      // over the other.
      //
      // The engine's number of a tab of ours: the newest link (the list is
      // in the order they joined) whose number is the application's
      // CERTAIN one and whose port the engine said was of a tab. A number
      // held only provisionally is the worker's guess, and a port of no
      // tab has no engine's number to give.
      const enginesTab = (tabId) => {
        for (let i = links.length - 1; i >= 0; i--) {
          const link = links[i];
          if (link.ids.certain !== tabId) continue;
          let theirs = null;
          try { const tab = link.port.sender ? link.port.sender.tab : null; theirs = tab ? tab.id : null; } catch (e) {}
          if (Number.isInteger(theirs) && theirs > 0) return theirs;
        }
        return null;
      };
      engineScripting.forEach((call, key) => {
        own.set('scripting.' + key, (...args) => {
          const injection = args[0];
          const target = injection && typeof injection === 'object' ? injection.target : undefined;
          // no tab id to look up is the engine's to refuse, in its words.
          if (!target || typeof target !== 'object' || !Number.isInteger(target.tabId)) return call(...args);
          const callback = typeof args[args.length - 1] === 'function' ? args.pop() : null;
          const linked = enginesTab(target.tabId);
          const answer = onTheirTab(linked !== null ? Promise.resolve(linked) : askedTab(target.tabId), injection, call);
          return callback ? toCallback(answer, callback) : answer;
        });
      });
    } else if (scripting) {
      OWN.add('scripting.executeScript');
      own.set('scripting.executeScript', executeScript);
    }
)js";

const char *ENVELOPE_WORKER_MAIN = R"js(
    // the list of the page's own world the host was last seen to take
    // (null: not known -- at the start, and after a refusal), the one it
    // refused (not sent again as it is), and who waits for it to be taken.
    let mainTaken = null, mainRefused = null, mainFlying = false, mainSoon = false, registering = 0;
    const mainWaiting = [];
    const mainList = () => {
      const out = [];
      registered.forEach((entry) => {
        // (one of style sheets alone has nothing for the page's own world,
        //  and the host takes no registration without a script: R-246 low 2.)
        if (entry.told.world !== 'MAIN' || !entry.js.length) return;
        const one = { id: entry.id, js: entry.js.map((f) => f.path), matches: entry.told.matches.slice(),
                      allFrames: entry.told.allFrames, runAt: entry.told.runAt,
                      // (what the host keeps for the next run: D-421.)
                      persistAcrossSessions: entry.told.persistAcrossSessions };
        if (entry.told.excludeMatches && entry.told.excludeMatches.length) one.excludeMatches = entry.told.excludeMatches.slice();
        out.push(one);
      });
      return JSON.stringify(out);
    };
    const mainSettle = (waiting) => { for (const resolve of waiting) { try { resolve(); } catch (e) {} } };
    const mainFlush = () => {
      mainSoon = false;
      if (mainFlying || registering) return;
      const now = mainList();
      if (now === mainTaken || now === mainRefused) { mainSettle(mainWaiting.splice(0)); return; }
      mainFlying = true;
      const waiting = mainWaiting.splice(0);
      let asking;
      try { asking = Promise.resolve(ask('vanilla.mainScripts', [JSON.parse(now)])); } catch (e) { asking = Promise.reject(e); }
      asking.then(() => { mainTaken = now; mainRefused = null; }, (e) => {
        mainTaken = null; mainRefused = now;
        try { console.warn('scripting.registerContentScripts: the scripts of the page\'s own world were not taken: ' + ((e && e.message) || e)); } catch (x) {}
      }).then(() => { mainFlying = false; mainSettle(waiting); mainLater(); });
    };
    const mainLater = () => {
      if (mainSoon) return;
      mainSoon = true;
      try { setTimeout(mainFlush, 0); } catch (e) { mainSoon = false; }
    };
    const mainWait = () => new Promise((resolve) => { mainWaiting.push(resolve); mainLater(); });
    // a registration, counted while it is on its way, and answered once the
    // host has the list it changed ('registerNow' is the next part's).
    const registerContentScripts = (list) => {
      // (counted from the call, not from its first turn: what uBOL calls
      //  right after 'unregister' has answered is on its way before the
      //  list which that changed is sent.)
      registering++;
      const done = registerNow(list);
      done.then(() => {}, () => {}).then(() => { registering--; mainLater(); });
      return done.then((main) => main ? mainWait() : undefined);
    };
)js";

const char *ENVELOPE_WORKER_SCRIPTS = R"js(
    const SCRIPTS_KEPT = 256, RUN_ATS = ['document_start', 'document_end', 'document_idle'], WORLDS = ['ISOLATED', 'MAIN'];
    const registered = new Map();
    // the documents which could not be sent a registration for want of
    // room (R-232 mid 3), tried again -- once, together, a turn after room
    // is made -- by 'relieved' below.
    const STARVED_KEPT = 64;
    const starved = new Set();
    let relieving = false;
    // an address, taken apart strictly: what is not of the shape
    // 'scheme://host/path' is matched by no pattern. (The world's own 'URL'
    // is not leaned on: a worker has one, but this is what the tests run.)
    const address = (text) => {
      const m = /^([a-z][a-z0-9+.-]*):\/\/([^\/?#]*)([^?#]*)(\?[^#]*)?/i.exec(String(text));
      if (!m) return null;
      let authority = m[2];
      const at = authority.lastIndexOf('@');
      if (at >= 0) authority = authority.slice(at + 1);
      const hp = /^(\[[^\]]*\]|[^:]*)(?::(\d*))?$/.exec(authority);
      if (!hp) return null;
      return { scheme: m[1].toLowerCase(), host: hp[1].toLowerCase(), port: hp[2] || '', path: (m[3] || '/') + (m[4] || '') };
    };
    // a match pattern as Chrome reads one, made into a test of an address,
    // or null where it is no pattern. '<all_urls>' is every address of the
    // schemes below; a scheme of '*' is http or https; a host of '*' is
    // any, '*.x' is x and what is under it, 'x' is x alone, and a port may
    // follow; the path is matched whole, with the query, '*' standing for
    // anything. Not stricter than Chrome (R-230 low 1): uBOL registers
    // everything in one call, and one pattern refused would be all of it.
    const SCHEMES = ['http', 'https', 'file', 'ftp', 'ws', 'wss'];
    const glob = (s) => new RegExp('^' + s.split('*').map((piece) => piece.replace(/[.*+?^${}()|[\]\\\/]/g, '\\$&')).join('.*') + '$');
    const pattern = (text) => {
      if (typeof text !== 'string') return null;
      if (text === '<all_urls>') return (u) => SCHEMES.includes(u.scheme);
      const m = /^(\*|[a-z][a-z0-9+.-]*):\/\/([^\/]*)(\/.*)$/i.exec(text);
      if (!m) return null;
      const scheme = m[1].toLowerCase(), path = glob(m[3]);
      if (scheme !== '*' && !SCHEMES.includes(scheme)) return null;
      const hp = /^(\[[^\]]*\]|[^:]*)(?::(\*|\d+))?$/.exec(m[2].toLowerCase());
      if (!hp) return null;
      const host = hp[1], port = hp[2] === undefined ? null : hp[2];
      if (scheme === 'file' ? host !== '' : !(host === '*' || /^(\*\.)?[a-z0-9_-]+(\.[a-z0-9_-]+)*\.?$/.test(host) || /^\[[0-9a-f:.]+\]$/.test(host))) return null;
      const under = host.slice(0, 2) === '*.' ? host.slice(2) : null;
      return (u) => {
        if (scheme === '*' ? !(u.scheme === 'http' || u.scheme === 'https') : u.scheme !== scheme) return false;
        if (scheme !== 'file' && !(host === '*' || u.host === host
                                   || (under !== null && (u.host === under || u.host.slice(-under.length - 1) === '.' + under)))) return false;
        if (port !== null && port !== '*' && (u.port || (u.scheme === 'http' ? '80' : u.scheme === 'https' ? '443' : '')) !== port) return false;
        return path.test(u.path);
      };
    };
    // one registration as Chrome reads it, in Chrome's words where they are
    // known. Nothing is kept yet: 'told' is what 'getRegisteredContentScripts'
    // gives back, the defaults filled in and the file names as given.
    const read = (script, seen) => {
      if (!script || typeof script !== 'object') throw new Error('Invalid value for argument 1. Expected a list of scripts.');
      const id = script.id;
      if (typeof id !== 'string' || !id.length) throw new Error("Script's ID must not be empty");
      if (id[0] === '_') throw new Error("Script's ID '" + id + "' must not start with '_'");
      if (seen.has(id) || registered.has(id)) throw new Error("Duplicate script ID '" + id + "'");
      seen.add(id);
      const wrong = (what) => new Error("Script with ID '" + id + "' has invalid value for " + what + ".");
      const files = (key) => {
        const list = script[key];
        if (list === undefined) return [];
        if (!Array.isArray(list) || !list.every((f) => typeof f === 'string')) throw wrong("'" + key + "'");
        return list.map((f) => {
          const path = filePath(f);
          if (path === null) throw new Error('Could not load ' + (key === 'js' ? 'javascript' : 'css') + " '" + f + "' for content script.");
          return { name: f, path };
        });
      };
      const js = files('js'), css = files('css');
      if (!js.length && !css.length) throw new Error("Script with ID '" + id + "' must specify at least one js or css file.");
      const patterns = (key, required) => {
        const list = script[key];
        if (list === undefined) { if (required) throw new Error("Script with ID '" + id + "' must specify 'matches'."); return []; }
        if (!Array.isArray(list)) throw wrong("'" + key + "'");
        if (required && !list.length) throw new Error("Script with ID '" + id + "' must specify at least one match.");
        return list.map((p, i) => { const test = pattern(p); if (!test) throw wrong(key + '[' + i + ']'); return test; });
      };
      const tests = patterns('matches', true), excludes = patterns('excludeMatches', false);
      const flag = (key) => { const v = script[key]; if (v !== undefined && typeof v !== 'boolean') throw wrong("'" + key + "'"); return v === true; };
      const runAt = script.runAt === undefined ? 'document_idle' : script.runAt;
      if (!RUN_ATS.includes(runAt)) throw wrong("'runAt'");
      const world = script.world === undefined ? 'ISOLATED' : script.world;
      if (!WORLDS.includes(world)) throw wrong("'world'");
      const persist = script.persistAcrossSessions === undefined ? true : script.persistAcrossSessions;
      if (typeof persist !== 'boolean') throw wrong("'persistAcrossSessions'");
      const told = { id, matches: script.matches.slice(), allFrames: flag('allFrames'), matchOriginAsFallback: flag('matchOriginAsFallback'),
                     runAt, world, persistAcrossSessions: persist };
      if (script.excludeMatches !== undefined) told.excludeMatches = script.excludeMatches.slice();
      if (js.length) told.js = js.map((f) => f.name);
      if (css.length) told.css = css.map((f) => f.name);
      return { id, told, tests, excludes, js, css, codes: [], styles: [] };
    };
    // whether a registration is for a link's document: the world this shim
    // runs in (a page's own world is not to be had: D-376), the top frame
    // unless 'allFrames', an address one pattern matches and no exclusion
    // does, and one the manifest's host permissions reach (Chrome injects
    // nowhere else, whatever 'matches' says: R-232 low 4). A link the
    // engine gave no address is nobody's.
    // (a host permission's path is not read by Chrome -- 'https://a.example/'
    //  grants the whole host -- so it is read as '/*' here: R-233 low 4.)
    const hostPattern = (text) => typeof text === 'string' && text !== '<all_urls>' ? text.replace(/^([^:]+:\/\/[^\/]*)\/.*$/, '$1/*') : text;
    const hostTests = (() => {
      let list = [];
      try { list = chrome.runtime.getManifest().host_permissions || []; } catch (e) {}
      return (Array.isArray(list) ? list : []).map((t) => pattern(hostPattern(t))).filter((t) => t !== null);
    })();
    const fits = (entry, link) => {
      if (entry.told.world !== 'ISOLATED' || (link.frameId !== 0 && !entry.told.allFrames)) return false;
      const u = address(link.url);
      return !!u && hostTests.some((t) => t(u)) && entry.tests.some((t) => t(u)) && !entry.excludes.some((t) => t(u));
    };
    // sent with its id, which the document keeps. Nothing waits on the
    // value; what could not be sent, or was not answered, is unmarked so
    // that the next registration's round tries it again (the document
    // drops what it ran already).
    // (each style sheet with its place in the registration, which is what
    //  the document keeps it by: R-232 mid 2.)
    //
    // All of a registration or none of it (R-233 mid 1): the room for the
    // whole of it is asked for before any piece is sent, so that a piece
    // sent and a piece refused never leave a document being sent the same
    // piece again and again. One which no empty link has room for is not
    // for this link at all, and is marked given so that it is not asked
    // about again; one there is no room for NOW leaves the link starved,
    // to be tried once room is made.
    const sizeOf = (entry) => ({ bytes: entry.codes.reduce((n, c) => n + c.length, 0) + entry.styles.reduce((n, s) => n + s.length, 0),
                                 count: entry.styles.length + (entry.codes.length ? 1 : 0) });
    const starve = (link) => {
      if (starved.has(link)) return;
      if (starved.size >= STARVED_KEPT) starved.delete(starved.values().next().value);
      starved.add(link);
    };
    const give = (link, entry) => {
      const size = sizeOf(entry);
      if (size.bytes > LINK_BYTES || size.bytes > ALL_BYTES || size.count > PENDING_KEPT) { link.given.add(entry.id); return; }
      if (link.pending.size + link.runs.size + size.count > PENDING_KEPT || inFlight(link) + size.bytes > LINK_BYTES || flying + size.bytes > ALL_BYTES) { starve(link); return; }
      link.given.add(entry.id);
      const sent = entry.styles.map((text, nth) => styleIn(link, { add: text, script: entry.id, nth }));
      if (entry.codes.length) sent.push(runIn(link, entry.codes, { at: entry.told.runAt, script: entry.id }));
      Promise.all(sent).catch((e) => { link.given.delete(entry.id); if (e && e.starved) starve(link); });
    };
    const inject = (link) => {
      if (!link.url) return;
      registered.forEach((entry) => { if (!link.given.has(entry.id) && fits(entry, link)) give(link, entry); });
    };
    const relieve = () => {
      if (!starved.size || relieving) return;
      relieving = true;
      setTimeout(() => {
        relieving = false;
        const again = Array.from(starved);
        starved.clear();
        for (const link of again) if (links.includes(link)) inject(link);
      }, 0);
    };
    const withCallback = (f) => (...args) => {
      const callback = typeof args[args.length - 1] === 'function' ? args.pop() : null;
      let answer;
      try { answer = f(...args); } catch (e) { answer = Promise.reject(e); }
      return callback ? toCallback(answer, callback) : answer;
    };
    const TOO_MANY = 'Too many content scripts are registered.';
    const registerNow = (list) => Promise.resolve().then(() => {
      if (!Array.isArray(list)) throw new Error('Invalid value for argument 1. Expected a list of scripts.');
      const seen = new Set();
      const entries = list.map((s) => read(s, seen));
      if (registered.size + entries.length > SCRIPTS_KEPT) throw new Error(TOO_MANY);
      // the files, read now and kept with the registration (R-230 mid 1):
      // what a document is sent later is sent in one turn out of what is
      // here. One which cannot be read fails the call and keeps nothing.
      const unreadable = (key, f) => () => { throw new Error('Could not load ' + key + " '" + f.name + "' for content script."); };
      const filled = (entry) => Promise.all([
        Promise.all(entry.js.map((f) => readFile(f.path).catch(unreadable('javascript', f)))),
        Promise.all(entry.css.map((f) => readFile(f.path).catch(unreadable('css', f))))
      ]).then((both) => {
        // (what the page's own world runs is read by the host out of the
        //  copy, D-420: here it is only made sure of.)
        if (entry.told.world === 'MAIN') return;
        entry.codes = both[0]; entry.styles = both[1];
      });
      return Promise.all(entries.map(filled)).then(() => {
        // asked again as they are put down: another call may have been
        // answered while the files were read (invariant (d)). Put down
        // and handed to the linked documents in this one turn.
        for (const entry of entries) if (registered.has(entry.id)) throw new Error("Duplicate script ID '" + entry.id + "'");
        if (registered.size + entries.length > SCRIPTS_KEPT) throw new Error(TOO_MANY);
        for (const entry of entries) registered.set(entry.id, entry);
        for (const link of links.slice()) inject(link);
        return entries.some((entry) => entry.told.world === 'MAIN');
      });
    });
    const idsOf = (filter) => {
      if (filter === undefined || filter === null) return null;
      if (typeof filter !== 'object') throw new Error('Invalid value for argument 1. Expected an object.');
      if (filter.ids === undefined) return null;
      if (!Array.isArray(filter.ids) || !filter.ids.every((i) => typeof i === 'string')) throw new Error("Invalid value for argument 1. Property 'ids': expected a list of strings.");
      return filter.ids;
    };
    const unregisterContentScripts = (filter) => Promise.resolve().then(() => {
      const ids = idsOf(filter);
      if (ids === null) { registered.clear(); mainLater(); return; }
      for (const id of ids) if (!registered.has(id)) throw new Error("Nonexistent script ID '" + id + "'");
      for (const id of ids) registered.delete(id);
      mainLater();
    });
    const getRegisteredContentScripts = (filter) => Promise.resolve().then(() => {
      const ids = idsOf(filter), out = [];
      registered.forEach((entry) => { if (ids === null || ids.includes(entry.id)) out.push(JSON.parse(JSON.stringify(entry.told))); });
      return out;
    });
    // a style sheet into the targets, or out of them: the text, or the
    // files read here. 'origin' is taken and not acted on: the document
    // side has an author's sheet to put in and no user's (D-410).
    const styling = (remove) => (injection) => Promise.resolve().then(() => {
      const bad = (what) => new Error('Invalid value for argument 1. ' + what);
      if (!injection || typeof injection !== 'object' || !injection.target || typeof injection.target !== 'object') throw bad("Property 'target': expected an object.");
      const tabId = injection.target.tabId;
      if (!Number.isInteger(tabId)) throw new Error('No tab with id: ' + String(tabId) + '.');
      const hasCss = injection.css !== undefined, hasFiles = injection.files !== undefined;
      if (hasCss === hasFiles) throw new Error("Exactly one of 'css' and 'files' must be specified.");
      if (hasCss && typeof injection.css !== 'string') throw bad("Property 'css': expected a string.");
      if (hasFiles && (!Array.isArray(injection.files) || !injection.files.length || !injection.files.every((f) => filePath(f) !== null)))
        throw bad("Property 'files': expected a list of paths inside the extension.");
      if (injection.origin !== undefined && injection.origin !== 'AUTHOR' && injection.origin !== 'USER') throw bad("Property 'origin'.");
      const targets = targetsOf(injection.target, tabId, bad);
      const texts = hasCss ? Promise.resolve([injection.css]) : Promise.all(injection.files.map((f) => readFile(filePath(f))));
      return texts.then((list) => gathered(targets, (link) => list.reduce(
        (chain, text) => chain.then(() => styleIn(link, remove ? { remove: text } : { add: text })), Promise.resolve()))).then(() => undefined);
    });
    if (scripting && !engineScripting.size && !RELAYED) {
      arrived = inject;
      carrying = true;
      relieved = relieve;
      retired = (link) => { starved.delete(link); };
      for (const pair of [['registerContentScripts', registerContentScripts], ['unregisterContentScripts', unregisterContentScripts],
                          ['getRegisteredContentScripts', getRegisteredContentScripts], ['insertCSS', styling(false)], ['removeCSS', styling(true)]]) {
        OWN.add('scripting.' + pair[0]);
        own.set('scripting.' + pair[0], withCallback(pair[1]));
      }
    }
)js";

const char *ENVELOPE_WORKER_REST = R"js(
    /* --- what a page of this extension's has the worker send (S2b-4b) --- */
    // Said once, and it is a failure of the shim's: the extension asked for
    // nothing of the sort and its own words would be a lie.
    const NO_RELAY = 'chrome.tabs.sendMessage: the compatibility layer could not relay this call';
    // a popup is commonly gone before the answer, and this engine throws
    // where a closed channel is answered. Nothing is retried and nothing is
    // kept: there is nobody left to tell.
    const answering = (respond, answer) => { try { respond(answer); } catch (e) { try { console.error(e); } catch (x) {} } };
    // Which messages may be relayed is what the ENGINE proved of them, and
    // nothing the message says of itself: 'sender.origin' is the sending
    // frame's committed origin, which no page can forge. This extension's
    // own origin and no other.
    const relaying = (message, sender) =>
      !!message && typeof message === 'object' && message.__vanillaRelay === 1
      // one which wears both words is an envelope, and is read as one: what
      // a content script sends is never relayed, whatever else it carries.
      && message.__vanillaEnvelope !== 1
      && !!sender && sender.id === chrome.runtime.id
      && typeof sender.origin === 'string' && sender.origin === 'chrome-extension://' + chrome.runtime.id;
    // The arguments are read here and handed to 'own' as the extension
    // itself would have handed them over: a tabId which is a tab's, at most
    // one object of options, and nothing else. What does not answer that
    // description is said to be no call of ours, never guessed at -- a
    // 'tabs.remove' relayed to 'tabs.sendMessage' would do something the
    // page did not ask for.
    // and the word which ASKS for the relay, which a page of this
    // extension's may send back as easily as hear (the worker sends it to
    // its own pages). It is the shim's own and is not handed to the
    // extension's listeners; nothing is answered, nobody waiting for one.
    const wanting = (message, sender) =>
      !!message && typeof message === 'object' && message.__vanillaRelay === 'wanted'
      && !!sender && sender.id === chrome.runtime.id;
    // and the word of the page the application opens to WAKE this worker
    // (D-446): it has done what it was for by being sent. The extension's
    // own page of that name, in no tab, and nobody else's.
    const waking = (message, sender) =>
      !!message && typeof message === 'object' && message.__vanillaWake === 1
      && !!sender && sender.id === chrome.runtime.id && !sender.tab
      && sender.url === 'chrome-extension://' + chrome.runtime.id + '/vanilla_wake.html';
    // (the relay page's, as the engine names the sender: D-469. The engine
    //  gives that page a 'tab' of its own -- it is the top of a WebView2 --
    //  so the tab is not read: measured.)
    const menuChoice = (message, sender) =>
      !!message && typeof message === 'object' && message.__vanillaMenuChosen === 1 && Array.isArray(message.args)
      && !!sender && sender.id === chrome.runtime.id
      && sender.url === 'chrome-extension://' + chrome.runtime.id + '/vanilla_relay.html';
    // and 'scripting' (D-422): a page of the extension's has none on the
    // Qt engine, and uBOL's popup has its tools put into the tab with it.
    // One object of injection, the worker's own call taking it as the
    // extension's worker would have handed it over; no 'func', which no
    // message carries.
    const RELAYABLE = ['tabs.sendMessage', 'scripting.executeScript', 'scripting.insertCSS', 'scripting.removeCSS'];
    const relay = (message, respond) => {
      const args = message.args, api = message.api;
      const to = RELAYABLE.includes(api) ? own.get(api) : undefined;
      const fits = !Array.isArray(args) ? false
        : api === 'tabs.sendMessage'
          ? args.length >= 2 && args.length <= 3 && Number.isInteger(args[0]) && args[0] > 0
            && (args.length < 3 || (typeof args[2] === 'object' && args[2] !== null))
          : args.length === 1 && typeof args[0] === 'object' && args[0] !== null && !Array.isArray(args[0]) && args[0].func === undefined;
      // (said in the name of the call the page made, where it is one of
      //  ours: R-249 low 1.)
      const refusal = RELAYABLE.includes(api) && api !== 'tabs.sendMessage'
        ? 'chrome.' + api + ': the compatibility layer could not relay this call' : NO_RELAY;
      if (!fits || typeof to !== 'function') { answering(respond, { ok: false, error: refusal }); return; }
      let answer;
      try { answer = Promise.resolve(to(...args)); } catch (e) { answer = Promise.reject(e); }
      // 'e.message' and not 'String(e)': the latter is 'Error: Error: ...'
      // by the time the page has made an Error of it again.
      answer.then((value) => { answering(respond, { ok: true, value }); },
                  (e) => { answering(respond, { ok: false, error: (e && e.message) || String(e) }); });
    };
    const demux = function (message, sender, respond) {
      // The arrival-time snapshot is deliberate: one added while a message
      // waits does not receive it; one removed while it waits still does.
      const list = listeners.slice();
      // a page of this extension's asking the worker to send for it is not
      // the extension's message and is not handed on. Read first, so that
      // the words are told apart in one order and one only: relayed, an
      // envelope, or the extension's own.
      if (wanting(message, sender) || waking(message, sender)) return undefined;
      // a choice of the button's menu, which the application sends through
      // the extension's own relay page (D-469): handed to the menu's
      // listeners, and to nobody else.
      if (menuChoice(message, sender)) { try { menuChosen(fromHost(message.args)); } catch (e) {} return undefined; }
      if (relaying(message, sender)) { relay(message, respond); return true; }
      if (!(message && typeof message === 'object' && message.__vanillaEnvelope === 1 && message.from && typeof message.from === 'object' &&
            sender && sender.id === chrome.runtime.id))
        return hand(list, message, sender, respond);
      const from = message.from, inner = message.message;
      const nonce = typeof from.nonce === 'string' && /^[0-9a-f]{32}$/.test(from.nonce) ? from.nonce : null;
      // With no name of its own a document is still not every other one:
      // what it says of itself is all there is to keep them apart. Nobody
      // is asked for these, and nothing is claimed of the number beyond
      // that two documents do not share it for want of a name.
      const key = keyOf(nonce, from.frameId, from.tabUrl);
      // A greeting is the content shim's own, sent to WAKE this worker
      // before it opens a port ('connect' does not wake one: measured).
      // It is not handed to the extension's listeners -- the extension
      // never asked for it, and would be answering a message it cannot
      // read -- and it is answered at once, so that the shim knows
      // somebody is awake. Asking whose tab that document is starts
      // here, so that the answer is in hand by the time the port names
      // itself. (An envelope from anybody but this extension does not
      //  reach here: the test above is the sender's id.)
      if (message.link === 1) {
        look(key, nonce);
        const told = { linked: 1 };
        if (message.carrier === 1) told.carrying = carrying ? 1 : 0;
        try { respond(told); } catch (e) { try { console.error(e); } catch (x) {} }
        return undefined;
      }
      let known = recall(key);
      // a page of this extension's own (D-370), told apart by what the
      // ENGINE proved of the sender and by nothing the message says of
      // itself. Such a document may truly be of no tab -- a popup is of
      // none -- so nothing is made up for it: its entry is 'null' until
      // the host names it, and 'null' is what its messages are handed over
      // with. (A content script is a web origin and takes none of this.)
      const mine = !!sender && typeof sender.origin === 'string'
                   && sender.origin === 'chrome-extension://' + chrome.runtime.id;
      // a number which is not the host's is used as it is: the asking again
      // is the timer's work, never this message's.
      if (!known && (!nonce || !asks)) { remember(key, mine ? null : randomTab(), true); known = tabs.get(key); }
      if (known) return hand(list, inner, known.id === null ? plain(sender, from) : opened(sender, from, known.id), respond);
      // nothing is held for the TOP frame of a page of this extension's: a
      // popup is opened in a window of its own, which no interceptor of a
      // view stamps, so its '/bind' is never bound and its every first
      // message would wait out the ladder for nothing (R-165 mid 1). It is
      // asked after ONCE -- an options page opened in a tab is named, and
      // its second message has the number -- and this one goes now with no
      // tab.
      if (mine && from.frameId === 0) {
        if (!waiting.has(nonce)) {
          waiting.set(nonce, { held: [], token: null, round: 0, chain: {}, once: true });
          resolve(nonce, null);
        }
        return hand(list, inner, plain(sender, from), respond);
      }
      const held = { list, message: inner, sender, from, respond };
      if (waiting.has(nonce)) { waiting.get(nonce).held.push(held); return true; }
      waiting.set(nonce, { held: [held], token: null, round: 0, chain: {} });
      // an embedded page of this extension's -- the vomnibar, the HUD --
      // waits as a content script does, '/bind' racing its first message
      // being what the ladder is for, and falls back to no tab.
      resolve(nonce, mine ? null : randomTab());
      return true;
    };
    const onMessage = listenedTo(listeners);
    // The engine is told of the demux when the shim goes in, whatever the
    // extension listens to, and is never told to forget it: a content
    // script's greeting is to be heard by a worker whose extension has
    // not listened yet, or has stopped. What finds no listener of the
    // extension's is answered with nothing, as it was before -- the
    // sender's call then fails with 'the port closed' rather than
    // 'nobody there', which is the known difference. (This is the taking
    // back of D-356 addendum 4 (1), decided in R-140.)
    try { real.addListener(demux); } catch (e) { refused.push('runtime.onMessage'); }
    try { Object.defineProperty(chrome.runtime, 'onMessage', { value: onMessage, configurable: true, writable: true, enumerable: true }); made.push('runtime.onMessage'); } catch (e) { refused.push('runtime.onMessage'); }
    installLink();
  };
)js";

const char *ENVELOPE_CONTENT = R"js(
  const installEnvelope = () => {
    if (!chrome.runtime || typeof chrome.runtime.sendMessage !== 'function') return;
    const now = () => { try { return Date.now(); } catch (e) { return 0; } };
    const top = (() => { try { return window === window.top; } catch (e) { return false; } })();
    const frameId = top ? 0 : 1 + Math.floor(Math.random() * 0x7ffffffe);
    // this document's name to the application, which knows the view it is
    // said from (D-356). Said when the document first speaks, and not
    // before: one which never does asks nothing of anybody. It goes in a
    // header -- never an address, which the page's own timings could tell
    // of -- and nothing of it is put in the DOM.
    const nonce = (() => {
      try {
        const bytes = new Uint8Array(16);
        crypto.getRandomValues(bytes);
        return Array.from(bytes, (b) => (b < 16 ? '0' : '') + b.toString(16)).join('');
      } catch (e) { return null; }
    })();
    // what the bind came to, settled either way: the worker which asks
    // this document for its nonce asks the host next, which knows it only
    // once the bind is in (D-434).
    let named = false, bound = null;
    const name = () => {
      if (named || !nonce || typeof fetch !== 'function') return bound;
      named = true;
      try {
        bound = Promise.resolve(fetch('vanilla-extension://host/bind', { method: 'POST', cache: 'no-store', credentials: 'omit',
                                                                         headers: { 'X-Vanilla-Nonce': nonce } })).then(() => true, () => false);
      } catch (e) { bound = Promise.resolve(false); }
      return bound;
    };
    const tabUrl = () => {
      if (top) return location.href;
      try { return window.top.location.href; } catch (e) {}
      try {
        const above = location.ancestorOrigins, origin = above && above.length ? above[above.length - 1] : null;
        if (typeof origin === 'string' && origin !== 'null') return origin + '/';
      } catch (e) {}
      return location.href;
    };
    const real = chrome.runtime.sendMessage;
    const send = function (...args) {
      // (message), (message, callback), (message, options[, callback]). With
      // an extension's id in front it is for somebody else, and goes as it is.
      const toAnother = typeof args[0] === 'string' && args.length >= 2 && typeof args[1] !== 'function';
      if (!toAnother && args.length >= 1) {
        name();
        args[0] = { __vanillaEnvelope: 1, from: { nonce, frameId, tabUrl: tabUrl() }, message: args[0] };
      }
      return real.apply(chrome.runtime, args);
    };
    try { Object.defineProperty(chrome.runtime, 'sendMessage', { value: send, configurable: true, writable: true, enumerable: true }); made.push('runtime.sendMessage'); } catch (e) { refused.push('runtime.sendMessage'); }

)js";

const char *ENVELOPE_CONTENT_LINK = R"js(
    /* --- the way back: one port to the worker, while somebody listens --- */
    const GREET_WAIT = 5000, LINK_WAIT = 2000, PING = 20000;
    const BACKOFF = [1000, 2000, 4000, 8000, 16000], TRIES = 5;
    const HELD_LONG = 25000, SHOWN_AGAIN = 30000, HIDDEN_AGAIN = 45000;
    // when this shim began, which is the order the worker tells of frames
    // in. It says nothing of the document but that.
    const since = now();
    // the extension's own listeners, kept here: nothing is registered with
    // the engine's 'onMessage', there being no way in this engine for
    // anything to reach a content script's (not measured -- what was
    // measured is that the worker cannot send to one at all, which is what
    // the port below is for). Nothing happens until the FIRST of them is
    // added -- a content script which listens to nothing greets nobody,
    // opens no port and keeps no timer -- and the last one to go closes the
    // port again.
    const listeners = [];
    // a carrier's document (ROADMAP A-32, D-410): the copy's own content
    // script, put in every frame of the addresses the extension may
    // register content scripts for, so that what its worker registers has
    // somewhere to run. It links whether or not anybody listens, and stays
    // linked when the last listener goes; the document of any other
    // content script is as before. The mark is the copy's, written ahead
    // of this shim in that one file, and read once.
    let carried = (() => { try { return self.__vanillaCarrier === 1; } catch (e) { return false; } })();
    // told 'carrying: 0' (nothing is registered its way): a carrier so told
    // is wanted by nobody of its own, and stops when no listener holds it
    // either -- now, or when the last one goes (R-233 low 3).
    let refused = false;
    const wanted = () => (carried && !refused) || listeners.length > 0;
    const canLink = typeof chrome.runtime.connect === 'function';
    // 'idle' (nobody listening) -> 'greeting' -> 'connecting' (waiting to be
    // told 'linked') -> 'linked' -> 'down'. 'gen' is which try we are in, so
    // that an answer to an older one settles nothing.
    let state = 'idle', gen = 0, port = null;
    let failures = 0, warned = false, linkedAt = 0, shownAt = 0;
    // whether this document has been told it is linked, ever: until then
    // its claim says 'fresh', which the worker takes for its commit
    // (D-426). For the document's whole life, and never set back.
    let linkedOnce = false;
    // whether the try which is running greeted (woke the worker) or only
    // opened a port. A port which nobody was awake to hear of is not worth
    // waiting out once the document is seen (R-141 B5).
    let greeted = false;
    // one timer for the trying and one for the ping, and never two of
    // either: each is put back rather than kept alongside. The token is for
    // a world where 'clearTimeout' is not there or does not take -- the one
    // which fires then finds it is nobody's and does nothing.
    let tryingTimer = null, tryingToken = 0, pingTimer = null, pingToken = 0;
    // what cannot be read is not 'visible': the safe side of invariant 4 is
    // to greet nobody and ping nobody, so that a worker still sleeps where
    // this shim cannot tell (R-141 B6).
    const seen = () => { try { return document.visibilityState === 'visible'; } catch (e) { return false; } };
    // spread out: every frame of a tab tries at once, and a hundred of them
    // greeting in the same millisecond is a thundering herd.
    const soon = () => 200 + Math.floor(Math.random() * 801);
    const later = () => HIDDEN_AGAIN + Math.floor(Math.random() * 5000);
    const clear = (timer) => { try { if (timer !== null && typeof clearTimeout === 'function') clearTimeout(timer); } catch (e) {} };
    const arm = (ms) => {
      clear(tryingTimer);
      const mine = ++tryingToken;
      tryingTimer = setTimeout(() => {
        if (mine !== tryingToken) return;
        tryingTimer = null;
        moment();
      }, ms);
    };
    const disarm = () => { clear(tryingTimer); tryingTimer = null; tryingToken++; };
    const hush = () => { clear(pingTimer); pingTimer = null; pingToken++; };
    const beat = () => {
      if (!port) return false;
      const mine = port;
      try { mine.postMessage({ ping: 1 }); } catch (e) { lost(mine); return false; }
      return true;
    };
    const ping = (ms) => {
      clear(pingTimer);
      const mine = ++pingToken;
      pingTimer = setTimeout(() => {
        if (mine !== pingToken) return;
        pingTimer = null;
        // hidden, or no longer linked: no timer is kept. Being shown again
        // is what puts one back.
        if (state !== 'linked' || !seen()) return;
        if (!beat()) return;
        ping(PING);
      }, ms);
    };
    const drop = () => {
      if (!port) return;
      const old = port;
      port = null;
      try { old.disconnect(); } catch (e) {}
    };
    // what the worker asked, handed to the listeners which are there at
    // THIS moment (one added while the answer is pending does not receive
    // it), and what they make of it answered by the pair (port, number).
    // Nobody there is said so: the sender is to be told nobody is there,
    // not left waiting.
    const deliver = (mine, id, message) => {
      const list = listeners.slice();
      if (!list.length) { try { mine.postMessage({ re: id, nobody: 1 }); } catch (e) {} return; }
      // the answer goes out THROUGH the gate, and a throw is let out of
      // here into it: a value which will not go over a port has not been
      // answered with, and 'replyOnce' leaves the gate open for the next
      // listener (R-141 B2). It is the gate which swallows it, so nothing
      // comes out of the shim.
      const once = replyOnce((value) => { mine.postMessage(value === undefined ? { re: id } : { re: id, value }); });
      decide(handAll(list, message, { id: chrome.runtime.id }, once), once);
    };
    // code of the worker's, run in this world's global scope (S2b-14,
    // D-376): an indirect eval. The value goes back if it will clone; a
    // throw goes back as the error.
)js";

const char *ENVELOPE_CONTENT_RUN = R"js(
    // what a thrown value is called. A value which will not say -- a
    // 'message' which throws, or a value which will not become a string
    // -- is answered for all the same: were no reply sent, the worker
    // would wait out its 30 s and call this frame gone.
    const said = (e) => { try { return String(e && e.message !== undefined ? e.message : e); } catch (t) { return 'Error'; } };
    // the registered content scripts this document ran (D-410), by the id
    // the worker sent them under, for the document's whole life: a link
    // made again brings them again, and they are not to run twice.
    const RAN_KEPT = 4096;
    const ranIds = new Set();
    // once and once only: a key seen before, or a table which is full, is
    // not run (R-232 low 5). A script's key and a sheet's cannot be one
    // another's, whatever the id holds.
    const firstTime = (key) => {
      if (ranIds.has(key) || ranIds.size >= RAN_KEPT) return false;
      ranIds.add(key);
      return true;
    };
    const scriptKey = (script) => 'js:' + script;
    const sheetKey = (script, nth) => 'css:' + String(nth) + ':' + script;
    const ran = (mine, id, codes, at, script) => {
      const post = (reply) => { try { mine.postMessage(reply); } catch (e) {} };
      if (!Array.isArray(codes) || !codes.length || !codes.every((c) => typeof c === 'string')) { post({ ran: id, error: 'no code to run' }); return; }
      // a registered content script: answered at once -- nothing waits for
      // its value, and the bytes are given back at the worker's end -- run
      // once in this document whatever link brought it, each file on its
      // own as Chrome runs them, and not before the document is parsed
      // where its 'runAt' says so.
      if (typeof script === 'string') {
        post({ ran: id });
        if (!firstTime(scriptKey(script))) return;
        let went = false;
        const go = () => {
          if (went) return;
          went = true;
          for (const code of codes) { try { (0, eval)(code); } catch (e) { try { console.error(e); } catch (x) {} } }
        };
        let loading = false;
        try { loading = at !== 'document_start' && document.readyState === 'loading'; } catch (e) {}
        if (!loading) { go(); return; }
        // told by the event, and by a clock of this world's as well: the
        // page's own listener can stop the event before this world hears
        // it, and cannot stop the clock (R-232 low 3).
        try { document.addEventListener('DOMContentLoaded', go, { once: true }); } catch (e) {}
        const tick = () => {
          if (went) return;
          let still = false;
          try { still = document.readyState === 'loading'; } catch (e) {}
          if (!still) { go(); return; }
          setTimeout(tick, 50);
        };
        setTimeout(tick, 50);
        return;
      }
      let last;
      try { for (const code of codes) last = (0, eval)(code); }
      catch (e) { post({ ran: id, error: said(e) }); return; }
      // (a promise is waited for, as Chrome waits for one.)
      Promise.resolve(last).then((v) => {
        let value;
        try { value = v === undefined ? undefined : JSON.parse(JSON.stringify(v)); } catch (e) { value = undefined; }
        post(value === undefined ? { ran: id } : { ran: id, value });
      }, (e) => { post({ ran: id, error: said(e) }); });
    };
    // a style sheet of the worker's into this document, or out of it
    // (D-410): a constructed sheet adopted by the document, which the
    // page's own CSP has no say over (uBOL puts its own in the same way).
    // Kept by its text so that the same text takes it out again; a world
    // without 'CSSStyleSheet' or 'adoptedStyleSheets' answers the error, as
    // a page which threw would. One of a registration's is put in once for
    // the document's life, by the registration and its place in it, as a
    // registration's code is run once (R-232 mid 2).
    const SHEETS_KEPT = 1024;
    const sheets = new Map();
    let sheetCount = 0;
    const styled = (mine, id, m) => {
      const post = (reply) => { try { mine.postMessage(reply); } catch (e) {} };
      try {
        if (typeof m.add === 'string') {
          // (a registration's: answered and not put in where it is in
          //  already; marked once it IS in, so that one which failed is
          //  tried again -- R-233 low 5.)
          const key = typeof m.script === 'string' ? sheetKey(m.script, m.nth) : null;
          if (key !== null && (ranIds.has(key) || ranIds.size >= RAN_KEPT)) { post({ styled: id }); return; }
          if (sheetCount >= SHEETS_KEPT) throw new Error('too many style sheets');
          const sheet = new CSSStyleSheet();
          sheet.replaceSync(m.add);
          document.adoptedStyleSheets.push(sheet);
          if (!sheets.has(m.add)) sheets.set(m.add, []);
          sheets.get(m.add).push(sheet);
          sheetCount++;
          if (key !== null) ranIds.add(key);
        } else if (typeof m.remove === 'string') {
          const list = sheets.get(m.remove);
          if (list && list.length) {
            const sheet = list.pop();
            if (!list.length) sheets.delete(m.remove);
            sheetCount--;
            document.adoptedStyleSheets = document.adoptedStyleSheets.filter((s) => s !== sheet);
          }
        } else throw new Error('no style sheet to put in or take out');
        post({ styled: id });
      } catch (e) { post({ styled: id, error: said(e) }); }
    };
)js";

const char *ENVELOPE_CONTENT_LINK_REST = R"js(
    const heard = (mine, m) => {
      if (port !== mine || !m || typeof m !== 'object') return;
      if (typeof m.run === 'number') { ran(mine, m.run, m.codes, m.at, m.script); return; }
      if (typeof m.style === 'number') { styled(mine, m.style, m); return; }
      if (m.linked === 1) {
        if (state !== 'connecting') return;
        // (told, the worker has taken the commit, whatever this document
        //  does next.)
        linkedOnce = true;
        if (readyClaimed > readyTold) readyTold = readyClaimed;
        readyClaimed = 0;
        if (unwanted(m)) return;
        state = 'linked';
        linkedAt = now();
        disarm();
        // the answer says what the WORKER believes this document's address
        // to be ('port.sender.url' at its end), and that one does not
        // follow a 'pushState' (measured: R-149). So the last word is the
        // worker's word, and everything it is missing -- what the page
        // pushed before this port, and what it pushed while the answer was
        // on its way -- is caught up with in ONE message. A document which
        // is where the worker thinks it is says nothing.
        //
        // With no word at all (an older worker, or an engine which said
        // nothing of the sender) nothing is claimed: the address is taken
        // to be this document's own and nothing is caught up with, which
        // is what this did before there was a word (the safe side).
        told = typeof m.url === 'string' && m.url ? m.url : (here() || told);
        catchUp();
        // and what it loaded while the answer was on its way.
        readyUp();
        if (seen()) ping(PING);
        return;
      }
      if (typeof m.id !== 'number') return;
      deliver(mine, m.id, m.message);
    };
    // the port went (the worker stopped, or the document was taken over).
    // A link which HELD is not a worker which cannot be reached, so the
    // count starts over; one which broke at once is counted. A port which
    // never linked AT ALL was never a link, whatever the clock says of a
    // 'linkedAt' nothing ever wrote: counting those would have a visible
    // document wake the worker for ever (R-141 B1).
    const lost = (mine) => {
      if (port !== mine) return;
      const held = state === 'linked' && now() - linkedAt >= HELD_LONG;
      port = null;
      state = 'down';
      hush();
      if (held) { failures = 0; warned = false; }
      step(soon());
    };
    const failed = () => {
      drop();
      state = 'down';
      hush();
      step(null);
    };
    // A hidden document tries for ever: it wakes nobody, so it costs
    // nothing but its own timer. A visible one tries five times over
    // thirty-one seconds and then says so, once, where an extension's
    // errors are read. The count comes back only by the rules below.
    const step = (ms) => {
      // whatever this try had on its way goes first, so that giving up
      // really is the end of it.
      disarm();
      if (!wanted()) return;
      if (!seen()) { arm(later()); return; }
      failures++;
      if (failures > TRIES) {
        if (!warned) {
          warned = true;
          try { console.warn('Vanilla: the compatibility layer could not reach this extension\'s worker from ' + location.href + '; what it sends to this frame will not arrive'); } catch (e) {}
        }
        return;
      }
      arm(ms === null ? BACKOFF[failures - 1] : ms);
    };
    // a carrier's document told, by the worker's answer to its greeting or
    // its naming, that nothing is registered its way (WebView2's worker,
    // or one without the permission: R-232 mid 1): it stops being one,
    // and is then a document like any other, which does nothing until
    // somebody listens. Only a carrier with no listener stops: a listener's
    // link is its own.
    const unwanted = (word) => {
      if (!carried || refused || !word || word.carrying !== 0) return false;
      refused = true;
      if (listeners.length) return false;
      drop();
      disarm();
      hush();
      state = 'idle';
      return true;
    };
    const connect = () => {
      let mine = null;
      try { mine = chrome.runtime.connect({ name: '__vanilla_link__' }); } catch (e) { mine = null; }
      if (!mine || !mine.onMessage || !mine.onDisconnect) { failed(); return; }
      port = mine;
      state = 'connecting';
      try {
        mine.onMessage.addListener((m) => { heard(mine, m); });
        mine.onDisconnect.addListener(() => { lost(mine); });
        // this document names itself with the port's first message: which
        // frame it is, what it says its tab is (the same words the envelope
        // carries, so that the worker knows them for one document) and when
        // it began. Its address it does not claim: the engine tells the
        // worker that one.
        const claim = { nonce, frameId, tabUrl: tabUrl(), since };
        if (carried && !refused) claim.carrier = 1;
        if (!linkedOnce) claim.fresh = 1;
        // how far this document is loaded, where that is further than the
        // worker was told and the document is seen (D-452).
        readyClaimed = readyClaim();
        if (readyClaimed) { claim.ready = readyClaimed; claim.readyFrom = readyTold; }
        mine.postMessage({ link: claim });
      } catch (e) { failed(); return; }
      arm(LINK_WAIT);
    };
    const attempt = () => {
      if (!canLink || !wanted()) return;
      watchReady();
      gen++;
      // whatever was open goes first, always: a port the worker never heard
      // of will never answer, and two of them would have it retire one as
      // the other's document.
      drop();
      hush();
      // this document is named to the HOST here, greeting or no greeting:
      // '/bind' is a request of the application's own scheme and wakes no
      // worker, and without it nobody can be told whose tab a document
      // which only ever opens a port is (the port's naming has the worker
      // ask, and the host answers from this).
      name();
      // hidden, the port is tried on its own: a greeting would wake the
      // worker, which is the one thing a document nobody looks at must not
      // do (invariant 4).
      if (!seen()) { greeted = false; connect(); return; }
      greeted = true;
      state = 'greeting';
      arm(GREET_WAIT);
      const mine = gen;
      // the greeting goes through the engine's own 'sendMessage', not the
      // wrapping one: it IS the envelope, and it is what wakes the worker.
      let asking;
      const hello = { __vanillaEnvelope: 1, from: { nonce, frameId, tabUrl: tabUrl() }, link: 1 };
      if (carried && !refused) hello.carrier = 1;
      try { asking = Promise.resolve(real.call(chrome.runtime, hello)); }
      catch (e) { asking = Promise.reject(e); }
      asking.then((answer) => {
        if (mine !== gen || state !== 'greeting') return;
        // only the shim's own word counts: 'undefined' and 'null' are what
        // a worker with no shim, or none awake, leaves behind (D-356
        // addendum 5), and the registering turn is known to swallow a
        // message whole.
        if (answer && answer.linked === 1) { if (!unwanted(answer)) connect(); } else failed();
      }, () => { if (mine === gen && state === 'greeting') failed(); });
    };
    // whatever the one trying timer was for, read off the state it finds.
    const moment = () => {
      if (!wanted()) return;
      if (state === 'greeting' || state === 'connecting') { failed(); return; }
      if (state === 'down') attempt();
    };
    // (the first listener is also where this document begins to hear of
    //  its own navigations, which it does once for its whole life: 'watch'
    //  keeps its own mark, 'begin' running at every 0 -> 1.)
    // (a carrier is linked, or trying, from its start: the first listener
    //  brings the watching and nothing else there -- unless the carrier
    //  gave up, with no try on its way, in which case the listener's count
    //  starts it again as it would any document: R-232 low 7.)
    const begin = () => { failures = 0; warned = false; watch(); if (!carried || refused || (state === 'down' && tryingTimer === null)) attempt(); };
    const stop = () => { disarm(); hush(); hushNav(); drop(); state = 'idle'; };
    // shown again: the count comes back, but once in thirty seconds at
    // most, so that a tab switched to and fro does not try for ever.
    const shown = () => {
      if (!wanted()) return;
      if (!seen()) {
        hush();
        // hidden with nothing on its way -- the count ran out while it was
        // seen -- would never try again until it was seen once more. From
        // here the port alone is tried, which wakes nobody (R-141 B3). The
        // count does NOT come back: only the rules below bring it back.
        if (state === 'down') arm(later());
        return;
      }
      const at = now();
      if (at - shownAt >= SHOWN_AGAIN) { shownAt = at; failures = 0; warned = false; }
      // seen again: where a hidden document's page took it is caught up
      // with in ONE message, because only a seen document may put the
      // worker's idle timer back (R-147 high 1).
      catchUp();
      readyUp();
      if (state === 'linked') { if (beat()) ping(PING); return; }
      // a try which did not greet was aimed at a worker which may never
      // have been awake to hear of the port, and that port will not answer
      // even once the worker is up (measured). Now that the document is
      // seen, greet instead of waiting the port out; nothing failed, so
      // nothing is counted (R-141 B5).
      if (state === 'connecting' && !greeted) { drop(); state = 'down'; arm(soon()); return; }
      // and what was waiting while it was hidden is NOT left to run as a
      // seen document: that try would greet, and waking the worker once for
      // every hiding and showing is what the count ran out to stop
      // (R-142 mid 1). With the count run out there is nothing to try until
      // one of the rules above brings it back.
      if (state === 'down') { if (failures <= TRIES) arm(soon()); else disarm(); }
    };
    // back from the page cache: a document the user has just asked for, so
    // the count comes back. Whether a port survives the freeze is NOT
    // measured; a dead one reaches 'lost' the way any other does, by
    // 'onDisconnect' or by the next ping's 'postMessage' throwing.
    const back = () => {
      if (!wanted()) return;
      failures = 0;
      warned = false;
      if (state === 'linked' || state === 'greeting' || state === 'connecting') return;
      arm(seen() ? soon() : later());
    };
)js";

const char *ENVELOPE_CONTENT_NAV = R"js(
    /* --- what the page did to its own history (S2b-5c) --- */
    // a page which calls 'pushState' in a loop is ONE message: Vimium's
    // 'onURLChange' sends a message back to the frame for every event it
    // hears of, so what is not thinned out here is a round trip per call.
    // 'URL_MAX' is the bound the worker's side keeps as well, and what is
    // longer is not sent at all (R-148 low 3).
    const BUNDLE = 100, URL_MAX = 8192;
    // the state this adds, and no more: whether it is listening at all,
    // the destination of the navigation which is being committed (used up
    // by the 'currententrychange' which matches it), which kind the
    // bundle settled on, which kind could not be sent and at which
    // address, and where the last word went.
    let watching = false, remembered = null, which = null, unsent = null;
    let navTimer = null, navToken = 0;
    const here = () => { try { return String(location.href); } catch (e) { return ''; } };
    // the address the shim began at: a 'currententrychange' at the start of
    // a document (not measured) says nothing, because nothing changed.
    let told = here();
    const bare = (u) => { const at = u.indexOf('#'); return at < 0 ? u : u.slice(0, at); };
    // one timer for the bundling and never two: it is put back, with the
    // generation mark the trying timer uses, so that in a world where
    // 'clearTimeout' is not there the one which was put back finds it is
    // nobody's and does nothing.
    const bundle = (hash) => {
      // nobody listening, nothing at all: the handlers below are the
      // document's for its whole life, so THIS is where a content script
      // whose extension has stopped listening stops keeping state and
      // timers for what the page does (invariant 3 of D-359, R-148 mid 2).
      if (!listeners.length) return;
      if (hash !== null) which = hash;
      clear(navTimer);
      const mine = ++navToken;
      navTimer = setTimeout(() => {
        if (mine !== navToken) return;
        navTimer = null;
        moved();
      }, BUNDLE);
    };
    // the last listener going: the timer is taken back and what was
    // half-heard of a navigation is forgotten, so that a memory of one the
    // extension never asked about cannot colour a later one (R-148 mid 2).
    const hushNav = () => { clear(navTimer); navTimer = null; navToken++; remembered = null; which = null; unsent = null; };
    const moved = () => {
      const at = here();
      // what the last 'navigate' of this bundle said -- or, where an
      // earlier bundle could not send it and the document has not moved
      // since, what THAT one said. Without it, a 'pushState' which changes
      // only the fragment would be read off the two addresses and called a
      // fragment navigation, which is exactly what it is not (R-149 mid 1).
      const hash = which !== null ? which : (unsent !== null && unsent.url === at ? unsent.hash : null);
      which = null;
      unsent = null;
      // the same address is no navigation to tell of. (Chrome fires for a
      // 'pushState' to the address the document is already at, and for
      // 'updateCurrentEntry': the known difference.)
      if (!at || at === told) return;
      // an address the other end would drop is not sent -- and the last
      // word moves all the same, so that the same one is not bundled over
      // and over (R-148 low 3).
      if (at.length > URL_MAX) { told = at; return; }
      // not linked, or not seen: nothing goes, nothing is piled up, and
      // the last word does not move -- so being shown, or being linked,
      // still finds there is something to say. Only WHICH KIND it was is
      // kept, with the address it was at, for that one message.
      if (state !== 'linked' || !port || !seen()) {
        if (hash !== null) unsent = { url: at, hash };
        return;
      }
      const mine = port;
      const kind = (hash === null ? bare(at) === bare(told) : hash) ? 'fragment' : 'history';
      try { mine.postMessage({ nav: { kind, url: at } }); }
      catch (e) { if (hash !== null) unsent = { url: at, hash }; lost(mine); return; }
      told = at;
    };
    // the navigation which is about to be committed. NOT 'preventDefault',
    // NOT 'intercept', and nothing thrown out of here: what the page does
    // with its own history is not this shim's business.
    const navigating = (e) => {
      // (and with nobody listening, not even the memory is kept: see
      //  'bundle'.)
      if (!listeners.length) return;
      try {
        if (!e || !e.destination || e.destination.sameDocument !== true) { remembered = null; return; }
        remembered = { url: String(e.destination.url), hash: !!e.hashChange };
      } catch (x) { remembered = null; }
    };
    // committed. The memory is used UP, whether it fitted or not: one
    // navigation may begin inside another, a handler of the page's may
    // cancel one, and 'navigation.updateCurrentEntry()' fires this with no
    // 'navigate' before it at all (R-147 mid 1).
    const committed = () => {
      try {
        const memo = remembered;
        remembered = null;
        bundle(memo !== null && memo.url === here() ? memo.hash : null);
      } catch (x) {}
    };
    const watch = () => {
      if (watching) return;
      watching = true;
      let api = null;
      try {
        if (typeof navigation === 'object' && navigation && typeof navigation.addEventListener === 'function') api = navigation;
      } catch (e) {}
      if (api) {
        try { api.addEventListener('navigate', navigating); api.addEventListener('currententrychange', committed); } catch (e) {}
        return;
      }
      // without it, what the window itself is told of: a fragment for
      // 'hashchange', and nothing known for 'popstate'. 'pushState' and
      // 'replaceState' fire neither, which is the known gap.
      try {
        window.addEventListener('hashchange', () => { bundle(true); });
        window.addEventListener('popstate', () => { bundle(null); });
      } catch (e) {}
    };
    // being seen again: one message, of where the document is now, with its
    // kind read off the two addresses (no 'navigate' of it was kept).
    const catchUp = () => { if (here() !== told) bundle(null); };
    /* --- how far the document is loaded (D-452) --- */
    // 0 while it is parsed, 1 once its DOM is in, 2 once it has loaded; what
    // cannot be read is 0, which tells of nothing.
    const readyOf = () => {
      try {
        const said = document.readyState;
        return said === 'complete' ? 2 : said === 'interactive' ? 1 : 0;
      } catch (e) { return 0; }
    };
    // the stage the worker has been told of, for the document's whole life
    // and never set back, and the one the claim on its way carries: that
    // one counts as told only once 'linked' comes (R-298 mid 2). Told only
    // by a document which is SEEN, the one kind of document which may put
    // the worker's idle timer back (R-298 high 1): what a hidden one
    // loaded is told of when it is shown.
    let readyTold = 0, readyClaimed = 0, readyWatching = false;
    const readyClaim = () => {
      if (!seen()) return 0;
      const at = readyOf();
      return at > readyTold ? at : 0;
    };
    // at most two such messages for a document's life, the stage never
    // going back. One which would not go is not counted as told.
    const readyUp = () => {
      if (state !== 'linked' || !port || !seen()) return;
      const at = readyOf();
      if (at <= readyTold) return;
      const mine = port;
      try { mine.postMessage({ ready: at, from: readyTold }); } catch (e) { lost(mine); return; }
      readyTold = at;
    };
    // heard from the first try on, once: a content script which listens to
    // nothing has not tried, and does nothing (invariant 3 of D-359). The
    // one event is told of both stages, each just before the page's own
    // 'DOMContentLoaded' and 'load'.
    const watchReady = () => {
      if (readyWatching) return;
      readyWatching = true;
      try { document.addEventListener('readystatechange', () => { readyUp(); }); } catch (e) {}
    };
    // The worker of WebView2 asking which tab this is (D-434): its own
    // question over the ENGINE's 'tabs.sendMessage' -- the one way it has
    // to a document which holds no link -- answered with the nonce once
    // the bind is in, or after 'WHO_BIND_WAIT' whatever became of it --
    // shorter than the worker waits for the answer ('WHO_ANSWER'), so that
    // an answer is never late for a question still being asked.
    // The engine's 'onMessage' is taken before it is replaced below, and
    // the question goes to none of the extension's listeners. Only the
    // top frame answers, only this extension's worker asks (no
    // 'sender.tab': a document in a tab -- a content script, or a page of
    // the extension's, which has one on WebView2 -- is not answered), and a
    // web page cannot reach this world at all.
    const WHO_BIND_WAIT = 800;
    try {
      const engines = chrome.runtime.onMessage;
      if (engines && typeof engines.addListener === 'function') engines.addListener((message, sender, respond) => {
        if (!message || typeof message !== 'object' || message.__vanillaWho !== 1) return false;
        if (!top || !nonce || !sender || sender.id !== chrome.runtime.id || sender.tab) return false;
        const given = name();
        if (!given) return false;
        let done = false;
        const reply = () => { if (done) return; done = true; try { respond({ nonce }); } catch (e) {} };
        setTimeout(reply, WHO_BIND_WAIT);
        given.then(reply, reply);
        return true;
      });
    } catch (e) {}
    const onMessage = {
      addListener(f) {
        if (typeof f !== 'function' || listeners.includes(f)) return;
        listeners.push(f);
        if (listeners.length === 1) begin();
      },
      removeListener(f) {
        const at = listeners.indexOf(f);
        if (at < 0) return;
        listeners.splice(at, 1);
        if (!wanted()) stop();
      },
      hasListener(f) { return listeners.includes(f); },
      hasListeners() { return listeners.length > 0; },
    };
    try { Object.defineProperty(chrome.runtime, 'onMessage', { value: onMessage, configurable: true, writable: true, enumerable: true }); made.push('runtime.onMessage'); } catch (e) { refused.push('runtime.onMessage'); }
    try { document.addEventListener('visibilitychange', shown); } catch (e) {}
    try { window.addEventListener('pageshow', back); } catch (e) {}
    // a carrier links now, for the registered scripts' sake alone: its own
    // navigations are not watched for that (they are the listeners', as
    // before: 'begin' does both when the first is added).
    if (carried) attempt();
  };
)js";

const char *PAGE_RELAY = R"js(
  // Chrome's own words for "there was nobody to ask", which is what the
  // worker not being there comes to.
  const NOBODY = 'Could not establish connection. Receiving end does not exist.';
  // the extension's own 'runtime.sendMessage', taken at the shim's start
  // and never read at the call: by then the envelope (D-370) is what that
  // name answers, and this word is the SHIM's own to the worker -- one the
  // demux is to read as a relay and not to open as an envelope. Which is
  // why this piece goes in before the envelope does.
  const realSend = (() => { try { return chrome.runtime && chrome.runtime.sendMessage; } catch (e) { return null; } })();
  const relayed = (api, args) => {
    let asked;
    // a 'sendMessage' which throws where it is called is the same nothing
    // as one which rejects: the shim tells of neither as a success.
    try { asked = Promise.resolve(realSend.call(chrome.runtime, { __vanillaRelay: 1, api, args })); }
    catch (e) { return Promise.reject(new Error(NOBODY)); }
    return asked.then((answer) => {
      if (!answer || typeof answer !== 'object') throw new Error(NOBODY);
      if (answer.ok === true) return answer.value;
      if (answer.ok === false && typeof answer.error === 'string') throw new Error(answer.error);
      throw new Error(NOBODY);
    }, () => { throw new Error(NOBODY); });
  };
  // what Chrome refuses the call itself for. Said here rather than sent:
  // the worker takes a tabId, a message and at most one object of options,
  // and a call with more than that is not to be cut down to fit -- a fourth
  // argument dropped on the way would make what the worker refuses into
  // something it answers (R-156 low 1).
  const WRONG_COUNT = 'chrome.tabs.sendMessage takes a tabId, a message, and at most one object of options';
  const toTab = (...args) => {
    const callback = typeof args[args.length - 1] === 'function' ? args.pop() : null;
    // counted before anything is dropped, and a failure of the call and not
    // of the name: the stand-in's 'not available in this browser' would say
    // the browser has no such call, which is not what is wrong here.
    if (args.length < 2 || args.length > 3) {
      const error = new Error(WRONG_COUNT);
      if (callback) { failing(error, callback); return undefined; }
      return Promise.reject(error);
    }
    // 'options' nobody gave is not an argument: the worker reads how many
    // there are, and an 'undefined' third one would be a third one.
    const asked = args.length === 3 && args[2] !== undefined && args[2] !== null ? args : [args[0], args[1]];
    const answer = relayed('tabs.sendMessage', asked);
    return callback ? toCallback(answer, callback) : answer;
  };
  // with no 'sendMessage' to relay over there is nobody to ask, and the
  // name is left the stand-in which fails -- never a silent nothing.
  try { if (typeof realSend === 'function') { own.set('tabs.sendMessage', toTab); made.push('tabs.sendMessage'); } }
  catch (e) { refused.push('tabs.sendMessage'); }

  // 'scripting' the same way (D-422), where the engine has none of its own
  // in a page (Qt; read here, before the stand-ins stand in front of it)
  // and the manifest asks for it. uBOL's popup puts its element zapper and
  // picker into the tab with 'executeScript' and closes at once: the
  // message is on its way before it does. Files only: a 'func' is no
  // thing a message carries, and is refused here in words which say so --
  // save on WebView2 with the key (D-435), where the page looks the tab up
  // itself as the worker does ('askedTab', D-434) and hands the engine's
  // own 'executeScript' the engine's number, 'func' and all. A page which
  // closes before that is done takes the injection with it: that is why
  // what CAN be relayed still is.
  {
    let engines = false, wanted = false;
    try { engines = !!chrome.scripting && typeof chrome.scripting.executeScript === 'function'; } catch (e) {}
    try { const p = chrome.runtime.getManifest().permissions; wanted = Array.isArray(p) && p.includes('scripting'); } catch (e) {}
    // (taken now, before the stand-ins stand in front of 'chrome.tabs'.)
    if (EDGE && engines && wanted && hosted.size) {
      try { engineScripting.set('executeScript', chrome.scripting.executeScript.bind(chrome.scripting)); } catch (e) {}
      takeEngineTabs();
    }
    const inPage = (injection) => {
      const run = engineScripting.get('executeScript'), target = injection.target;
      // no tab id to look up is the engine's to refuse, in its words.
      if (!target || typeof target !== 'object' || !Number.isInteger(target.tabId)) return Promise.resolve().then(() => run(injection));
      return onTheirTab(askedTab(target.tabId), injection, run);
    };
    // On WebView2 the page's own is there and knows the ENGINE's tabs
    // only: handed ours it fails (D-434), so the worker, which looks the
    // tab up again, is asked there too.
    if ((!engines || EDGE) && wanted && typeof realSend === 'function') for (const key of ['executeScript', 'insertCSS', 'removeCSS']) {
      const call = (...args) => {
        const callback = typeof args[args.length - 1] === 'function' ? args.pop() : null;
        const injection = args[0];
        let answer;
        if (args.length !== 1 || !injection || typeof injection !== 'object' || Array.isArray(injection))
          answer = Promise.reject(new Error('Invalid value for argument 1. Expected an object.'));
        else if (injection.func !== undefined && key === 'executeScript' && engineScripting.has('executeScript'))
          answer = inPage(injection);
        else if (injection.func !== undefined)
          answer = Promise.reject(new Error("chrome.scripting." + key + ": 'func' is not available from an extension's page in this browser"));
        else answer = relayed('scripting.' + key, [injection]);
        return callback ? toCallback(answer, callback) : answer;
      };
      try { own.set('scripting.' + key, call); if (engines) OWN.add('scripting.' + key); made.push('scripting.' + key); } catch (e) { refused.push('scripting.' + key); }
    }
  }
)js";

const char *ENVELOPE_PAGE_END = R"js(
  };
  try { installEnvelope(); } catch (e) { refused.push('runtime.sendMessage'); }
)js";

const char *RELAY = R"js(
(() => {
  const HOST_KEY = '__VANILLA_HOST_KEY__';
  // with the word and no key nobody is to be asked: the copy writes the key
  // only into a file which cannot be read from outside the extension.
  if (!/^[0-9a-f]{64}$/.test(HOST_KEY) || typeof fetch !== 'function') return;
  const PORT_NAME = '__vanilla_relay__', CALL = 'vanilla-extension://host/call';
  const EVENTS = 'vanilla.events', ABORT = 'vanilla.abort', WANTED = 'wanted';
  const TICKETS_MOST = 64, DEADLINE = 10000, SAFETY_WAIT = 300000;
  const unavailable = (api) => 'chrome.' + api + ' is not available in this browser';
  // the calls which are out, under the ticket the worker asked with: which
  // port asked, what will stop a held one, and the subscription it is of.
  const out = new Map();
  let port = null, timer = null;
  const post = (m) => { try { if (port) port.postMessage(m); } catch (e) {} };
  // one request of the host, made the way a document of this extension has
  // always made one: POST, nothing in the body, the key and what is asked
  // in headers (the engine's reader of a body is not to be fed).
  const asking = (call, signal) => {
    const options = { method: 'POST', cache: 'no-store', credentials: 'omit',
                      headers: { 'X-Vanilla-Key': HOST_KEY,
                                 'X-Vanilla-Call': encodeURIComponent(JSON.stringify({ api: call.api, args: call.args })) } };
    if (signal) options.signal = signal;
    else if (typeof AbortSignal === 'function' && typeof AbortSignal.timeout === 'function') {
      try { options.signal = AbortSignal.timeout(DEADLINE); } catch (e) {}
    }
    try { return Promise.resolve(fetch(CALL, options)); } catch (e) { return Promise.reject(e); }
  };
  // what the host's JSON comes to, read as the shims read it: a success is
  // the value, the application's refusal is its own words, and whatever
  // went wrong on the way is the failure a call nobody answered ends in.
  const answerOf = (json, api) => {
    if (json && json.ok === true) return { ok: true, value: json.value };
    if (json && typeof json.error === 'string') return { ok: false, error: json.error };
    return { ok: false, error: unavailable(api) };
  };
  // the host told to let a subscription's held call go. Its answer is
  // nobody's: the worker it was for is not listening any more.
  const abort = (token) => { try { asking({ api: ABORT, args: [token] }, null).then(() => {}, () => {}); } catch (e) {} };
  const heard = (mine, m) => {
    if (mine !== port || !m || typeof m !== 'object') return;
    const ticket = m.ticket, call = m.call;
    if (!Number.isInteger(ticket) || out.has(ticket)) return;
    if (!call || typeof call !== 'object' || typeof call.api !== 'string' || !Array.isArray(call.args)) return;
    // what is out is bounded, as what waits at the worker's end is: the one
    // over the bound is refused where it was asked, in the words a call
    // nobody answers ends in.
    if (out.size >= TICKETS_MOST) { post({ ticket, answer: { ok: false, error: unavailable(call.api) } }); return; }
    // the events are the one call which is held until there is something to
    // say, and the one this page keeps a way to stop; its first argument is
    // the subscription the host is to be told of.
    const held = call.api === EVENTS;
    let stopper = null;
    if (held && typeof AbortController === 'function') { try { stopper = new AbortController(); } catch (e) { stopper = null; } }
    const entry = { port: mine, stopper, token: held && typeof call.args[0] === 'string' ? call.args[0] : null, stopped: false };
    out.set(ticket, entry);
    const done = (answer) => {
      if (out.get(ticket) !== entry) return;
      out.delete(ticket);
      // over the port which asked and over no other: another port is
      // another worker, whose tickets are its own.
      if (entry.port !== port) return;
      post({ ticket, answer });
    };
    asking(call, stopper ? stopper.signal : null).then((response) => response.json()).then(
      (json) => done(answerOf(json, call.api)),
      () => done(entry.stopped ? { ok: false, error: 'aborted' } : { ok: false, error: unavailable(call.api) }));
  };
  const lost = (mine) => {
    if (mine !== port) return;
    port = null;
    // every held call of that worker is given up on where this world can
    // give one up, and the host is told of each. The rest are left to end
    // as they will: their answers are nobody's now.
    for (const entry of Array.from(out.values())) {
      if (!entry.stopper || entry.stopped) continue;
      entry.stopped = true;
      try { entry.stopper.abort(); } catch (e) {}
      if (entry.token) abort(entry.token);
    }
    later();
  };
  // the safety net, and the only timer here: while there is no port, one
  // try every five minutes and no oftener. It is for a word which was lost
  // -- the worker asked before this page was listening -- and not for
  // keeping a port: a port nothing goes over does not keep the worker
  // alive, so opening one oftener would wake it for nothing.
  function later(){
    if (timer !== null || port) return;
    try { timer = setTimeout(() => { timer = null; if (!port) connect(); }, SAFETY_WAIT); } catch (e) { timer = null; }
  }
  function connect(){
    let fresh = null;
    try { fresh = chrome.runtime.connect({ name: PORT_NAME }); } catch (e) { fresh = null; }
    if (!fresh) { later(); return; }
    try {
      fresh.onMessage.addListener((m) => heard(fresh, m));
      fresh.onDisconnect.addListener(() => lost(fresh));
    } catch (e) { later(); return; }
    port = fresh;
  }
  // the worker asking for a port. Whose word it is, the engine says, and
  // this extension's own id is the whole of the question (R-198): a TAB is
  // no reason to refuse, because WebView2 attaches one to senders Chrome
  // would not -- the relay page itself is given a tab there (measured:
  // D-379) -- and whether a service worker's message carries one there is
  // NOT measured. Believing it costs nothing either way: another page of
  // this extension's saying the word would only connect this extension's
  // own relay, which is harmless. Nothing is answered.
  const asked = (m, sender) => {
    if (port || !m || typeof m !== 'object' || m.__vanillaRelay !== WANTED) return;
    try { if (!sender || sender.id !== chrome.runtime.id) return; } catch (e) { return; }
    connect();
  };
  try { chrome.runtime.onMessage.addListener(asked); } catch (e) {}
  connect();
})()
)js";

const char *REPORT = R"js(
  if (refused.length) { try { console.warn('Vanilla: the compatibility layer could not put in ' + refused.join(', ')); } catch (e) {} }
)js";

}

QString WorkerShim(){
    return QStringLiteral("(() => {\n"
                          "  if (self.__vanillaShim) return 'already';\n"
                          "  self.__vanillaShim = 1;\n"
                          "  const made = [], refused = [];\n"
                          "  try {\n"
                          "    if (typeof self.skipWaiting === 'function' && typeof self.addEventListener === 'function') {\n"
                          "      self.addEventListener('install', () => { try { Promise.resolve(self.skipWaiting()).catch(() => {}); } catch (e) {} });\n"
                          "      self.addEventListener('activate', (event) => { try { event.waitUntil(Promise.resolve(self.clients.claim()).catch(() => {})); } catch (e) {} });\n"
                          "    }\n"
                          "  } catch (e) { refused.push('skipWaiting'); }\n")
        + QString::fromUtf8(FAILING)
        + QString::fromUtf8(ENGINE)
        + QStringLiteral("  const BY_BRAND = true;\n")
        + QStringLiteral("  const RELAYED = EDGE;\n")
        + QString::fromUtf8(HOST)
        + QString::fromUtf8(EVENTS)
        + QString::fromUtf8(STAND_INS)
        + QString::fromUtf8(GRANTS)
        + QString::fromUtf8(ACTION)
        + QString::fromUtf8(CONTEXTS)
        + QString::fromUtf8(OPTIONS)
        + QString::fromUtf8(IDENTITY)
        + QString::fromUtf8(ALARMS)
        + QString::fromUtf8(IDLE)
        + QString::fromUtf8(USER_SCRIPT_MESSAGES)
        + QString::fromUtf8(DOWNLOADS)
        + QString::fromUtf8(MENU_MIRROR)
        + QString::fromUtf8(MENU_TAB)
        + QString::fromUtf8(SCRIPTING_TAB)
        + QString::fromUtf8(EXTENSION_NS)
        + QString::fromUtf8(INSTALLED)
        + QString::fromUtf8(I18N)
        + QString::fromUtf8(STORAGE)
        + QString::fromUtf8(STORAGE_LATCH)
        + QString::fromUtf8(FOLD)
        + QString::fromUtf8(RELAY_PORT)
        + QString::fromUtf8(ENVELOPE_WORKER)
        + QString::fromUtf8(ENVELOPE_LINK)
        + QString::fromUtf8(ENVELOPE_LINK_SEND)
        + QString::fromUtf8(ENVELOPE_WORKER_NAV)
        + QString::fromUtf8(ENVELOPE_WORKER_RUN)
        + QString::fromUtf8(ENVELOPE_WORKER_MAIN)
        + QString::fromUtf8(ENVELOPE_WORKER_SCRIPTS)
        + QString::fromUtf8(ENVELOPE_WORKER_REST)
        + QStringLiteral("  try { installExtension(); } catch (e) { refused.push('extension'); }\n"
                         "  try { installMessages(true); } catch (e) { refused.push('i18n'); }\n"
                         "  try { installAction(); } catch (e) { refused.push('action.setIcon'); }\n"
                         "  try { installContexts(); } catch (e) { refused.push('runtime.getContexts'); }\n"
                         "  try { installOptions(); } catch (e) { refused.push('runtime.openOptionsPage'); }\n"
                         "  try { installIdentity(true); } catch (e) { refused.push('identity'); }\n"
                         "  try { installAlarms(); } catch (e) { refused.push('alarms'); }\n"
                         "  try { installIdle(); } catch (e) { refused.push('idle'); }\n"
                         "  try { installDownloads(); } catch (e) { refused.push('downloads.download'); }\n"
                         "  try { installMenuTab(); } catch (e) { refused.push('contextMenus.onClicked'); }\n"
                         "  try { installScriptingTab(); } catch (e) { refused.push('scripting'); }\n"
                         "  try { installStandIns(); } catch (e) { refused.push('the stand-ins'); }\n"
                         "  try { installStorage(false); } catch (e) { refused.push('storage'); }\n"
                         "  try { installEnvelope(); } catch (e) { refused.push('runtime.onMessage'); }\n"
                         "  let finishUserScriptMessage = null;\n"
                         "  try { finishUserScriptMessage = prepareUserScriptMessage(); } catch (e) { refused.push('runtime.onUserScriptMessage'); }\n"
                         "  try { installEvents(own); } catch (e) { refused.push('the tab events'); }\n"
                         "  try { if (finishUserScriptMessage) finishUserScriptMessage(); } catch (e) { refused.push('runtime.onUserScriptMessage'); }\n"
                         "  try { installInstalled(); } catch (e) { refused.push('runtime.onInstalled'); }\n"
                         "  try { installStartup(); } catch (e) { refused.push('runtime.onStartup'); }\n")
        + QString::fromUtf8(REPORT)
        + QStringLiteral("  return made.join(',');\n"
                         "})()\n");
}

QString UserScriptPrelude(const QString &extensionId, const QByteArray &secret){
    const QByteArray args = QJsonDocument(QJsonArray{ extensionId, QString::fromLatin1(secret) }).toJson(QJsonDocument::Compact);
    return QString::fromUtf8(USER_SCRIPT_PRELUDE).replace(QStringLiteral("__VANILLA_WORLD_ARGS__"), QString::fromUtf8(args));
}

QString MenuChosenScript(const QJsonArray &args){
    QString data = QString::fromUtf8(QJsonDocument(args).toJson(QJsonDocument::Compact));
    data.replace(QChar(0x2028), QStringLiteral("\\u2028")).replace(QChar(0x2029), QStringLiteral("\\u2029"));
    return QStringLiteral("try { chrome.runtime.sendMessage({ __vanillaMenuChosen: 1, args: ")
        + data + QStringLiteral(" }).catch(() => {}); } catch (e) {}");
}

QString WakeScript(){
    return QStringLiteral("try { chrome.runtime.sendMessage({ __vanillaWake: 1 }).catch(() => {}); } catch (e) {}\n");
}

QString RelayScript(){
    return QString::fromUtf8(RELAY);
}

QString ContentShim(){
    return QStringLiteral("(() => {\n"
                          "  try { if (typeof chrome !== 'object' || !chrome.runtime || !chrome.runtime.id) return 'not an extension'; } catch (e) { return 'not an extension'; }\n"
                          "  if (self.__vanillaContentShim) return 'already';\n"
                          "  self.__vanillaContentShim = 1;\n"
                          "  const made = [], refused = [];\n")
        + QString::fromUtf8(FAILING)
        + QStringLiteral("  const BY_BRAND = false, EDGE = false;\n")
        + QString::fromUtf8(EXTENSION_NS)
        + QString::fromUtf8(I18N)
        + QString::fromUtf8(STORAGE)
        + QString::fromUtf8(STORAGE_LATCH)
        + QString::fromUtf8(FOLD)
        + QString::fromUtf8(ENVELOPE_CONTENT)
        + QString::fromUtf8(ENVELOPE_CONTENT_LINK)
        + QString::fromUtf8(ENVELOPE_CONTENT_RUN)
        + QString::fromUtf8(ENVELOPE_CONTENT_LINK_REST)
        + QString::fromUtf8(ENVELOPE_CONTENT_NAV)
        + QStringLiteral("  try { installExtension(); } catch (e) { refused.push('extension'); }\n"
                         "  try { installMessages(false); } catch (e) { refused.push('i18n'); }\n"
                         "  try { installStorage(true); } catch (e) { refused.push('storage'); }\n"
                         "  try { installEnvelope(); } catch (e) { refused.push('runtime.sendMessage'); }\n")
        + QString::fromUtf8(REPORT)
        + QStringLiteral("  return made.join(',');\n"
                         "})()\n");
}

QString PageShim(){
    return QStringLiteral("(() => {\n"
                          "  try {\n"
                          "    if (typeof chrome !== 'object' || !chrome.runtime || !chrome.runtime.id) return 'not an extension';\n"
                          "    if (typeof location !== 'object' || !location || location.protocol !== 'chrome-extension:') return 'not an extension';\n"
                          "    if (typeof document !== 'object' || !document) return 'not an extension';\n"
                          "  } catch (e) { return 'not an extension'; }\n"
                          "  if (self.__vanillaPageShim) return 'already';\n"
                          "  self.__vanillaPageShim = 1;\n"
                          "  const made = [], refused = [];\n")
        + QString::fromUtf8(FAILING)
        + QString::fromUtf8(ENGINE)
        + QStringLiteral("  const BY_BRAND = true;\n")
        + QStringLiteral("  const RELAYED = false;\n")
        + QString::fromUtf8(HOST)
        + QStringLiteral("  // the tab events are the worker's (D-360) and are not subscribed to\n"
                         "  // from here: the stand-ins read this set, and empty it leaves those\n"
                         "  // names the stand-in which never fires.\n"
                         "  const SOUNDED = new Set();\n")
        + QString::fromUtf8(STAND_INS)
        + QString::fromUtf8(GRANTS)
        + QString::fromUtf8(ACTION)
        + QString::fromUtf8(OPTIONS)
        + QString::fromUtf8(IDENTITY)
        + QString::fromUtf8(EXTENSION_NS)
        + QString::fromUtf8(I18N)
        + QString::fromUtf8(SCRIPTING_TAB)
        + QString::fromUtf8(PAGE_RELAY)
        + QString::fromUtf8(STORAGE)
        + QString::fromUtf8(STORAGE_LATCH)
        + QStringLiteral("  try { installExtension(); } catch (e) { refused.push('extension'); }\n"
                         "  try { installMessages(true); } catch (e) { refused.push('i18n'); }\n"
                         "  try { installAction(); } catch (e) { refused.push('action.setIcon'); }\n"
                         "  try { installOptions(); } catch (e) { refused.push('runtime.openOptionsPage'); }\n"
                         "  try { installIdentity(false); } catch (e) { refused.push('identity'); }\n"
                         "  try { installStandIns(); } catch (e) { refused.push('the stand-ins'); }\n"
                         "  try { installStorage(true); } catch (e) { refused.push('storage'); }\n")
        + QString::fromUtf8(ENVELOPE_CONTENT)
        + QString::fromUtf8(ENVELOPE_PAGE_END)
        + QString::fromUtf8(REPORT)
        + QStringLiteral("  return made.join(',');\n"
                         "})()\n");
}

}

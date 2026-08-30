'use strict';

/*

  The directory settings page.

  Everything it knows comes from '/api/tree': the chain of directories above
  the directory this page's own tab sits in, the table of tokens there are
  controls for, and the words this page says for itself. The tokens'
  meanings live in the application; this page only edits them, and after
  every edit it asks for the chain again.

  The request says nothing about which directory it is: the address carries
  no name, and the application answers out of where this tab is. An address
  can be saved with the session and outlive the thing it named; where a tab
  sits cannot.

  The chain is shown the way it is read: the root first, the nearest
  directory last, each one overriding the ones above it. Directories
  elsewhere in the tree are not shown -- their settings do not reach the
  node this page is about.

  No English is written here, for the reason settings.js gives: the page
  runs inside Chromium and cannot reach the application's translations, so
  anything it displays has to arrive already translated.

  A change is sent as it is made. There is no save button because a change
  is a rename of the directory, and a rename is already immediate. The
  rename box in the tree is the other way round: it edits the name and puts
  the tokens back untouched, so this page and the checkbox menu are the only
  places the settings themselves are written (D-120).

*/

const state = {
    data: null,
};

const el = {
    chain: document.getElementById('chain'),
    inheritance: document.getElementById('inheritance'),
    empty: document.getElementById('empty'),
    toast: document.getElementById('toast'),
};

let toastTimer = 0;

function toast(message) {
    el.toast.textContent = message;
    el.toast.hidden = false;
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => { el.toast.hidden = true; }, 2400);
}

async function api(endpoint, body) {
    const response = await fetch('/api/' + endpoint, body ? {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(body),
    } : undefined);
    if (!response.ok) throw new Error(endpoint + ': ' + response.status);
    return response.json();
}

// ------------------------------------------------------------------ tokens

// the same reading the application does: the negation wins when both are
// present, and neither present means the setting is inherited.
//
// 'inverted' rows are turned round here, as 'DirectoryPage::TokenState' does
// it on the other side: the token behind "Auto load" is 'NoAutoLoad', so the
// word being there is the control being off.
function tokenState(tokens, sw) {
    const on = new RegExp('^' + sw.pattern + '$');
    const off = new RegExp('^!' + sw.pattern + '$');
    let state = -1;
    if (tokens.some(t => off.test(t))) state = 0;
    else if (tokens.some(t => on.test(t))) state = 1;
    return (sw.inverted && state !== -1) ? 1 - state : state;
}

// a token none of the controls claims: 'Proxy ...', 'Encoding ...', or
// anything this page does not know. These ride in the free field.
function unclaimed(tokens) {
    return tokens.filter(t =>
        !state.data.switches.some(sw =>
            new RegExp('^!?' + sw.pattern + '$').test(t)));
}

// ------------------------------------------------------------------ render

function buildRow(labelText, control, hintText) {
    const row = document.createElement('div');
    row.className = 'row';
    const label = document.createElement('label');
    label.className = 'label';
    label.textContent = labelText;
    if (hintText) {
        const hint = document.createElement('span');
        hint.className = 'hint';
        hint.textContent = hintText;
        label.appendChild(hint);
    }
    row.appendChild(label);
    row.appendChild(control);
    return row;
}

function buildDirectory(node, depth) {
    const strings = state.data.strings;

    const card = document.createElement('div');
    card.className = 'dir';
    // the indent is a custom property rather than a margin so that the
    // stylesheet can take it back when the directories above are folded
    // away: one card on its own has nothing to be indented from.
    card.style.setProperty('--depth', depth);

    const head = document.createElement('div');
    head.className = 'head';

    const profile = document.createElement('span');
    profile.className = 'profile';
    profile.textContent = strings.profile + ' ' + node.profile;

    if (node.root) {
        const name = document.createElement('strong');
        name.textContent = node.name;
        head.appendChild(name);
        head.appendChild(profile);
        card.appendChild(head);

        const note = document.createElement('p');
        note.className = 'note';
        note.textContent = strings.rootLocked;
        card.appendChild(note);
        return card;
    }

    const controls = [];   // read back by 'submit', in this order.

    const name = document.createElement('input');
    name.type = 'text';
    name.spellcheck = false;
    name.value = node.name;
    head.appendChild(name);
    head.appendChild(profile);
    card.appendChild(head);

    for (const sw of state.data.switches) {
        const st = tokenState(node.tokens, sw);
        // what saying nothing comes to here: the answer the directories
        // above have given, or -- if none of them said anything either --
        // the fixed meaning of the token's absence ('autoload' is off since
        // D-078). Without this the row reads "Default" and gives no sign of
        // what was just set further up.
        const inherited = (node.inherited && sw.key in node.inherited)
            ? node.inherited[sw.key] : -1;
        const settled = inherited !== -1 ? inherited
            : (typeof sw.absence === 'number' ? sw.absence : -1);
        let control;
        // a tick this directory did not write itself: shown, but not saved
        // back into its name (see 'submit').
        let borrowed = false;
        if (sw.off) {
            control = document.createElement('select');
            const dflt = settled === 1
                ? strings.stateDefault + ' (' + strings.stateOn + ')'
                : settled === 0
                ? strings.stateDefault + ' (' + strings.stateOff + ')'
                : strings.stateDefault;
            for (const [value, label] of [['default', dflt],
                                          ['on', strings.stateOn],
                                          ['off', strings.stateOff]]) {
                const option = document.createElement('option');
                option.value = value;
                option.textContent = label;
                control.appendChild(option);
            }
            control.value = st === 1 ? 'on' : st === 0 ? 'off' : 'default';
        } else {
            // 'ID' and 'Private' have no word for their off state, so a tick
            // inherited from above cannot be taken back here: it is shown,
            // and the box is left alone rather than springing back.
            control = document.createElement('input');
            control.type = 'checkbox';
            borrowed = st === -1 && settled === 1;
            control.checked = st === 1 || borrowed;
            control.disabled = borrowed;
        }
        // a row the global settings have taken out of use is shown with what
        // it says now, and cannot be answered: the word it would write is
        // ignored while that setting stands. The hint says which one.
        if (sw.blocked) control.disabled = true;
        controls.push({sw: sw, node: control, borrowed: borrowed});
        card.appendChild(buildRow(sw.label, control, sw.hint));
    }

    const extra = document.createElement('input');
    extra.type = 'text';
    extra.spellcheck = false;
    extra.value = unclaimed(node.tokens).join('; ');
    card.appendChild(buildRow(strings.otherTokens, extra,
                              strings.otherTokensHint));

    // the names 'UserAgent' takes, from the table rather than from the hint
    const agents = document.createElement('p');
    agents.className = 'agents';
    // the names go on their own line, indented: the sentence in front of
    // them is long enough that a run-on reads as one paragraph
    agents.textContent = strings.userAgentNames + '\n ' +
                         state.data.useragents.join(', ');
    card.appendChild(agents);

    // the one token whose cost is not obvious from its name
    const warning = document.createElement('p');
    warning.className = 'warning';
    warning.textContent = strings.userAgentWarning;
    card.appendChild(warning);

    const submit = async () => {
        const tokens = [];
        for (const {sw, node: control, borrowed} of controls) {
            if (sw.off) {
                // a blocked row is read back the same way: its control still
                // shows what this name says, so what it says is kept.
                if (control.value === 'on') tokens.push(sw.on);
                else if (control.value === 'off') tokens.push(sw.off);
            } else if (control.checked && !borrowed) {
                // a borrowed tick is one this directory inherited, not one
                // it said: writing it down would put a word in its name that
                // changes nothing.
                tokens.push(sw.on);
            }
        }
        for (const token of extra.value.split(';')) {
            const trimmed = token.trim();
            if (trimmed !== '') tokens.push(trimmed);
        }
        try {
            const reply = await api('set',
                {node: node.id, name: name.value.trim(), tokens: tokens});
            if (reply.error) throw new Error(reply.error);
            await load();
            // the directories below are not on this page, so an edit which
            // reached them has to say so.
            toast(reply.followed
                  ? strings.followed.replace('%1', reply.followed)
                  : strings.saved);
        } catch (e) {
            // leave what was typed on screen so it can be corrected, and
            // say so rather than pretending the change took.
            card.classList.add('invalid');
            toast(strings.failed);
        }
    };

    name.addEventListener('change', submit);
    extra.addEventListener('change', submit);
    for (const {node: control} of controls)
        control.addEventListener('change', submit);

    return card;
}

function render() {
    const strings = state.data.strings;

    document.title = strings.title;
    document.getElementById('title').textContent = strings.title;
    el.inheritance.textContent = strings.inheritance;

    el.chain.textContent = '';
    const chain = state.data.chain || [];

    // only the directory this tab sits in is shown to begin with. The ones
    // above it are what its settings are inherited from, and they are
    // usually not what the page was opened to change, so they wait behind a
    // disclosure -- opened, they come back in the order they always had,
    // the root first and the nearest last. (D-122)
    if (chain.length > 1) {
        const above = document.createElement('details');
        const summary = document.createElement('summary');
        summary.textContent =
            strings.ancestors.replace('%1', chain.length - 1);
        above.appendChild(summary);
        chain.slice(0, -1).forEach((node, depth) => {
            above.appendChild(buildDirectory(node, depth));
        });
        el.chain.appendChild(above);
    }
    if (chain.length)
        el.chain.appendChild(buildDirectory(chain[chain.length - 1],
                                            chain.length - 1));

    const none = chain.every(node => node.root);
    el.empty.hidden = !none;
    el.empty.textContent = strings.noDirectories;
}

// -------------------------------------------------------------------- boot

async function load() {
    // no query: which chain to show is decided by where this tab sits.
    state.data = await api('tree');
    render();
}

load().catch(e => {
    // if '/api/tree' itself is what failed there is no translation to use,
    // so the English is kept as the fallback rather than showing nothing.
    const prefix = (state.data && state.data.strings && state.data.strings.loadFailed)
        || 'Could not load the directories: ';
    document.body.textContent = prefix + e.message;
});

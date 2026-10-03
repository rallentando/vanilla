'use strict';

if(window.top !== window.self){
    document.documentElement.textContent = '';
    throw new Error('framed');
}

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

function tokenState(tokens, sw) {
    const on = new RegExp('^' + sw.pattern + '$');
    const off = new RegExp('^!' + sw.pattern + '$');
    let state = -1;
    if (tokens.some(t => off.test(t))) state = 0;
    else if (tokens.some(t => on.test(t))) state = 1;
    return (sw.inverted && state !== -1) ? 1 - state : state;
}

function unclaimed(tokens) {
    return tokens.filter(t =>
        !state.data.switches.some(sw =>
            new RegExp('^!?' + sw.pattern + '$').test(t)));
}

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

    const controls = [];

    const name = document.createElement('input');
    name.type = 'text';
    name.spellcheck = false;
    name.value = node.name;
    head.appendChild(name);
    head.appendChild(profile);
    card.appendChild(head);

    for (const sw of state.data.switches) {
        const st = tokenState(node.tokens, sw);
        const inherited = (node.inherited && sw.key in node.inherited)
            ? node.inherited[sw.key] : -1;
        const settled = inherited !== -1 ? inherited
            : (typeof sw.absence === 'number' ? sw.absence : -1);
        let control;
        let borrowed = false;
        if (sw.off) {
            control = document.createElement('select');
            const dflt = strings.stateDefault.replace('%1',
                settled === 1 ? strings.stateOn : strings.stateOff);
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
            control = document.createElement('input');
            control.type = 'checkbox';
            borrowed = st === -1 && settled === 1;
            control.checked = st === 1 || borrowed;
            control.disabled = borrowed;
        }
        if (sw.blocked) control.disabled = true;
        controls.push({sw: sw, node: control, borrowed: borrowed});
        card.appendChild(buildRow(sw.label, control, sw.hint));
    }

    const id = controls.find(c => c.sw.key === 'id');
    if (id) {
        const follow = () => {
            if (!id.node.checked)
                id.node.disabled = id.borrowed || name.value.trim() === '';
        };
        follow();
        name.addEventListener('input', follow);
    }

    const extra = document.createElement('input');
    extra.type = 'text';
    extra.spellcheck = false;
    extra.value = unclaimed(node.tokens).join('; ');
    card.appendChild(buildRow(strings.otherTokens, extra,
                              strings.otherTokensHint));

    const agents = document.createElement('p');
    agents.className = 'agents';
    agents.textContent = strings.userAgentNames + '\n ' +
                         state.data.useragents.join(', ');
    card.appendChild(agents);

    const warning = document.createElement('p');
    warning.className = 'warning';
    warning.textContent = strings.userAgentWarning;
    card.appendChild(warning);

    const submit = async () => {
        const tokens = [];
        for (const {sw, node: control, borrowed} of controls) {
            if (sw.off) {
                if (control.value === 'on') tokens.push(sw.on);
                else if (control.value === 'off') tokens.push(sw.off);
            } else if (control.checked && !borrowed) {
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
            toast(reply.followed
                  ? strings.followed.replace('%1', reply.followed)
                  : strings.saved);
        } catch (e) {
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

function toggleAnimated(details, summary) {
    const opening = !details.open || details.dataset.closing === '1';
    const from = details.getBoundingClientRect().height;
    details.getAnimations().forEach(a => a.cancel());
    delete details.dataset.closing;
    if (window.matchMedia('(prefers-reduced-motion: reduce)').matches) {
        details.open = opening;
        return;
    }
    details.open = true;
    const style = getComputedStyle(details);
    const border = parseFloat(style.borderTopWidth)
        + parseFloat(style.borderBottomWidth);
    const to = opening ? details.getBoundingClientRect().height
        : summary.getBoundingClientRect().height + border;
    if (!opening) details.dataset.closing = '1';
    const animation = details.animate(
        { height: [from + 'px', to + 'px'] },
        { duration: Math.min(400, 150 + Math.abs(to - from) / 4),
          easing: 'cubic-bezier(0.2, 0, 0, 1)' });
    animation.onfinish = () => {
        if (details.dataset.closing === '1') {
            delete details.dataset.closing;
            details.open = false;
        }
    };
}

function render() {
    const strings = state.data.strings;

    document.title = strings.title;
    document.getElementById('title').textContent = strings.title;
    el.inheritance.textContent = strings.inheritance;

    const aboveWasOpen = el.chain.querySelector('details')?.open ?? false;
    el.chain.textContent = '';
    const chain = state.data.chain || [];

    if (chain.length > 1) {
        const above = document.createElement('details');
        above.open = aboveWasOpen;
        const summary = document.createElement('summary');
        for (const [className, text] of [
            ['when-closed', strings.ancestorsShow],
            ['when-open', strings.ancestorsHide],
        ]) {
            const label = document.createElement('span');
            label.className = className;
            label.textContent = text.replace('%1', chain.length - 1);
            summary.appendChild(label);
        }
        above.appendChild(summary);
        summary.addEventListener('click', e => {
            e.preventDefault();
            toggleAnimated(above, summary);
        });
        chain.slice(0, -1).forEach((node, depth) => {
            above.appendChild(buildDirectory(node, depth));
        });
        el.chain.appendChild(above);
    }
    if (chain.length) {
        const current = buildDirectory(chain[chain.length - 1], chain.length - 1);
        current.classList.add('current');
        const heading = document.createElement('h2');
        heading.className = 'current-label';
        heading.textContent = strings.currentDirectory;
        current.prepend(heading);
        el.chain.appendChild(current);
    }

    const none = chain.every(node => node.root);
    el.empty.hidden = !none;
    el.empty.textContent = strings.noDirectories;
}

async function load() {
    state.data = await api('tree');
    render();
}

load().catch(e => {
    const prefix = (state.data && state.data.strings && state.data.strings.loadFailed)
        || 'Could not load the directories: ';
    document.body.textContent = prefix + e.message;
});

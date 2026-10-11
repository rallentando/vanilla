'use strict';

if(window.top !== window.self){
    document.documentElement.textContent = '';
    throw new Error('framed');
}

const state = {
    categories: [],
    items: [],
    strings: null,
    current: null,
    query: '',
    inputs: null,
    inputEditing: false,
    inputBusy: false,
    passwords: null,
    downloads: null,
    directory: null,
    directoryId: null,
    directoryAboveOpen: false,
    directoryTreeOpen: false,
    directoryTreeHeight: '',
    jumpTo: null,
};

const el = {
    nav: document.getElementById('nav'),
    main: document.getElementById('main'),
    empty: document.getElementById('empty'),
    search: document.getElementById('search'),
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

function sameValue(a, b) {
    if (Array.isArray(a) && Array.isArray(b))
        return a.length === b.length && a.every((v, i) => v === b[i]);
    return a === b;
}

const controls = {
    bool: {
        make: () => Object.assign(document.createElement('input'), {type: 'checkbox'}),
        read: (node) => node.checked,
        write: (node, value) => { node.checked = value; },
        event: 'change',
    },
    int: {
        make: () => Object.assign(document.createElement('input'), {type: 'number', step: '1'}),
        read: (node) => node.value.trim() === '' ? null : Number(node.value),
        write: (node, value) => { node.value = value; },
        event: 'change',
    },
    text: {
        make: () => Object.assign(document.createElement('input'), {type: 'text'}),
        read: (node) => node.value,
        write: (node, value) => { node.value = value; },
        event: 'change',
    },
    directory: {
        make: () => Object.assign(document.createElement('input'),
                                 {type: 'text', spellcheck: false}),
        read: (node) => node.value,
        write: (node, value) => { node.value = value; },
        event: 'change',
    },
    choice: {
        make: (item) => {
            const node = document.createElement('select');
            item.choices.forEach((choice, i) => {
                const option = document.createElement('option');
                option.value = choice;
                option.textContent = (item.choiceLabels && item.choiceLabels[i]) || choice;
                node.appendChild(option);
            });
            return node;
        },
        read: (node) => node.value,
        write: (node, value) => { node.value = value; },
        event: 'change',
    },
    textlist: {
        make: () => Object.assign(document.createElement('textarea'), {spellcheck: false}),
        read: (node) => node.value.split('\n').map(s => s.trim()).filter(s => s !== ''),
        write: (node, value) => { node.value = (value || []).join('\n'); },
        event: 'change',
        wide: true,
    },
};

function noticeLine(text) {
    const line = document.createElement('p');
    line.className = 'notice';
    line.textContent = text;
    return line;
}

function buildRow(item) {
    const control = controls[item.type];
    const row = document.createElement('div');
    row.className = 'row' + (control.wide ? ' wide' : '');
    row.dataset.key = item.key;

    const text = document.createElement('div');
    text.className = 'text';

    const label = document.createElement('label');
    label.className = 'label';
    label.textContent = item.label;
    if (item.needsRestart) {
        const badge = document.createElement('span');
        badge.className = 'restart';
        badge.textContent = state.strings.restart;
        label.appendChild(badge);
    }
    text.appendChild(label);

    if (item.hint) {
        const hint = document.createElement('span');
        hint.className = 'hint';
        hint.textContent = item.hint;
        text.appendChild(hint);
    }

    if (item.appliesWhen) {
        const note = document.createElement('span');
        note.className = 'hint note';
        note.hidden = true;
        text.appendChild(note);
    }
    if (item.key === 'webview/@SavePasswords' &&
        state.categories.some(category => category.name === 'passwords')) {
        const link = document.createElement('a');
        link.className = 'hint link';
        link.href = '#passwords';
        link.textContent = state.strings.openPasswords;
        text.appendChild(link);
    }
    row.appendChild(text);

    const holder = document.createElement('div');
    holder.className = 'control';

    const node = control.make(item);
    node.id = 'c-' + item.key.replace(/[^A-Za-z0-9]/g, '-');
    label.setAttribute('for', node.id);
    control.write(node, item.value);
    holder.appendChild(node);

    let browse = null;
    if (item.picker) {
        browse = document.createElement('button');
        browse.className = 'browse';
        browse.type = 'button';
        browse.textContent = state.strings.browse;
        holder.appendChild(browse);
    }

    const reset = document.createElement('button');
    reset.className = 'reset';
    reset.type = 'button';
    reset.textContent = state.strings.reset;
    reset.title = state.strings.resetTitle;
    holder.appendChild(reset);

    row.appendChild(holder);

    const markChanged = () => {
        row.classList.toggle('changed', !sameValue(item.value, item.fallback));
    };
    markChanged();

    const saveValue = async (value) => {
        if (value === null) {
            control.write(node, item.value);
            return false;
        }
        try {
            const reply = await api('set', {key: item.key, value: value});
            if (reply.error) throw new Error(reply.error);
            item.value = reply.value;
            control.write(node, item.value);
            row.classList.remove('invalid');
            markChanged();
            refreshApplicability();
            toast(item.needsRestart ? state.strings.savedRestart : state.strings.saved);
            return true;
        } catch (e) {
            row.classList.add('invalid');
            toast(state.strings.failed);
            return false;
        }
    };

    node.addEventListener(control.event, async () => {
        await saveValue(control.read(node));
    });

    if (browse) browse.addEventListener('click', async () => {
        browse.dataset.busy = 'true';
        browse.disabled = true;
        try {
            const reply = await api('pick-directory', {key: item.key});
            if (reply.error) throw new Error(reply.error);
            if (!reply.path) return;

            let value = control.read(node);
            if (Array.isArray(value)) {
                if (value.includes(reply.path)) return;
                value.push(reply.path);
            } else {
                value = reply.path;
            }
            control.write(node, value);
            await saveValue(value);
        } catch (e) {
            row.classList.add('invalid');
            toast(state.strings.failed);
        } finally {
            delete browse.dataset.busy;
            browse.disabled = false;
            refreshApplicability();
        }
    });

    reset.addEventListener('click', async () => {
        const reply = await api('reset', {key: item.key});
        item.value = reply.value;
        control.write(node, item.value);
        row.classList.remove('invalid');
        markChanged();
        refreshApplicability();
        toast(state.strings.saved);
    });

    return row;
}

const ARROWS = {U: '↑', D: '↓', R: '→', L: '←',
                UR: '↗', UL: '↖', DR: '↘', DL: '↙'};
const MOUSE_BUTTONS = ['LeftButton', 'RightButton', 'MidButton', 'ExtraButton1', 'ExtraButton2'];
const WHEELS = ['WheelUp', 'WheelDown'];

function inputWord(token) {
    return (state.inputs.strings[token]) || token;
}

function inputText(table, entry) {
    if (table.kind === 'keys')
        return (entry.parts || []).map(stroke => stroke.join('+')).join(', ') + ' ' + entry.input;
    if (table.kind === 'mouse')
        return entry.input.split('+').map(inputWord).join('+') + ' ' + entry.input;
    return entry.input.split(',').map(s => ARROWS[s] || s).join(' ') + ' ' + entry.input;
}

function inputMatches(table, entry, query) {
    if (!query) return true;
    const haystack = (table.label + ' ' + entry.label + ' ' + entry.action + ' ' +
                      inputText(table, entry)).toLowerCase();
    return query.split(/\s+/).every(word => haystack.includes(word));
}

function kbd(text) {
    const node = document.createElement('kbd');
    node.textContent = text;
    return node;
}

function drawInput(table, entry) {
    const holder = document.createElement('span');
    holder.className = 'input';
    const joiner = (text) => {
        const node = document.createElement('span');
        node.className = 'joiner';
        node.textContent = text;
        return node;
    };

    if (table.kind === 'keys' && entry.parts) {
        entry.parts.forEach((stroke, i) => {
            if (i) holder.appendChild(joiner(', '));
            stroke.forEach((part, j) => {
                if (j) holder.appendChild(joiner('+'));
                holder.appendChild(kbd(part));
            });
        });
    } else if (table.kind === 'mouse') {
        entry.input.split('+').forEach((token, i) => {
            if (i) holder.appendChild(joiner('+'));
            holder.appendChild(kbd(inputWord(token)));
        });
    } else if (table.kind === 'gesture') {
        for (const stroke of entry.input.split(',')) {
            const node = document.createElement('span');
            node.className = 'stroke';
            node.textContent = ARROWS[stroke] || stroke;
            node.title = stroke;
            holder.appendChild(node);
        }
    } else {
        holder.appendChild(kbd(entry.input));
    }
    return holder;
}

function actionSelect(table, value) {
    const select = document.createElement('select');
    select.className = 'action';
    let found = false;
    for (const group of state.inputs.vocabularies[table.vocabulary]) {
        const optgroup = document.createElement('optgroup');
        optgroup.label = group.label;
        for (const action of group.actions) {
            const option = document.createElement('option');
            option.value = action.name;
            option.textContent = action.label;
            optgroup.appendChild(option);
            if (action.name === value) found = true;
        }
        select.appendChild(optgroup);
    }
    if (value && !found) {
        const option = document.createElement('option');
        option.value = value;
        option.textContent = value;
        select.insertBefore(option, select.firstChild);
    }
    select.value = value || 'NoAction';
    return select;
}

function heldBack(table, entry) {
    if (table.kind === 'keys' && table.singleKeysHeld && entry.single && state.inputs.singleKeysOff)
        return state.inputs.strings.singleKeyOff;
    if (table.kind === 'gesture' && entry.diagonal && state.inputs.fourWayGestures)
        return state.inputs.strings.diagonalOff;
    return '';
}

function replaceTable(described) {
    const tables = state.inputs.tables;
    const i = tables.findIndex(t => t.group === described.group);
    if (i >= 0) tables[i] = described;
}

function holdInputs() {
    state.inputBusy = true;
    for (const node of el.main.querySelectorAll('.input-table button, .input-table select, .input-table input'))
        node.disabled = true;
}

async function writeTable(table, entries) {
    if (state.inputBusy) return false;
    holdInputs();
    try {
        const reply = await api('input-set', {
            group: table.group,
            entries: entries.map(e => ({input: e.input, action: e.action})),
        });
        if (reply.error) throw new Error(reply.error);
        replaceTable(reply);
        toast(state.strings.saved);
        return true;
    } catch (e) {
        toast(state.strings.failed);
        return false;
    } finally {
        state.inputBusy = false;
        render();
    }
}

async function resetTable(table) {
    if (state.inputBusy) return;
    holdInputs();
    try {
        const reply = await api('input-reset', {group: table.group});
        if (reply.error) throw new Error(reply.error);
        replaceTable(reply);
        toast(state.strings.saved);
    } catch (e) {
        toast(state.strings.failed);
    } finally {
        state.inputBusy = false;
        render();
    }
}

function bindingRow(table, entry, index) {
    const words = state.inputs.strings;
    const row = document.createElement('div');
    row.className = 'binding';
    if (!entry.isDefault) row.classList.add('changed');
    if (!entry.known) row.title = words.unknown;

    const why = heldBack(table, entry);
    if (why) {
        row.classList.add('inactive');
        row.title = why;
    }

    row.appendChild(drawInput(table, entry));

    if (state.inputEditing) {
        const select = actionSelect(table, entry.action);
        select.addEventListener('change', () => {
            const entries = table.entries.slice();
            entries[index] = {input: entry.input, action: select.value};
            writeTable(table, entries);
        });
        row.appendChild(select);

        const remove = document.createElement('button');
        remove.type = 'button';
        remove.className = 'remove';
        remove.textContent = '×';
        remove.title = words.remove;
        remove.setAttribute('aria-label', words.remove);
        remove.addEventListener('click', () => {
            writeTable(table, table.entries.filter((_, i) => i !== index));
        });
        row.appendChild(remove);
    } else {
        const label = document.createElement('span');
        label.className = 'does';
        label.textContent = entry.label || entry.action || '—';
        row.appendChild(label);
    }
    return row;
}

const NAMED_KEYS = {
    ArrowUp: 'Up', ArrowDown: 'Down', ArrowLeft: 'Left', ArrowRight: 'Right',
    PageUp: 'PgUp', PageDown: 'PgDown', Home: 'Home', End: 'End',
    Insert: 'Ins', Delete: 'Del', Escape: 'Esc', Tab: 'Tab',
    Backspace: 'Backspace', Enter: 'Return', ' ': 'Space', Pause: 'Pause',
    ContextMenu: 'Menu', PrintScreen: 'Print',
};

function keyName(event) {
    if (event.key === 'Tab' && event.shiftKey) return 'Backtab';
    if (NAMED_KEYS[event.key]) return NAMED_KEYS[event.key];
    if (/^F\d{1,2}$/.test(event.key)) return event.key;
    if (event.key.length === 1) return event.key.toUpperCase();
    return '';
}

let stopRecording = null;

function keyRecorder(onChange) {
    const words = state.inputs.strings;
    const recorder = document.createElement('button');
    recorder.type = 'button';
    recorder.className = 'recorder';
    recorder.textContent = words.pressKey;

    let recording = false;
    const stop = () => {
        recording = false;
        stopRecording = null;
        recorder.classList.remove('recording');
        window.removeEventListener('keydown', onKey, true);
    };
    const onKey = (event) => {
        event.preventDefault();
        event.stopPropagation();
        if (['Control', 'Shift', 'Alt', 'Meta'].includes(event.key)) return;
        const name = keyName(event);
        if (!name) return;
        const parts = [];
        if (event.ctrlKey)  parts.push('Ctrl');
        if (event.altKey)   parts.push('Alt');
        if (event.shiftKey) parts.push('Shift');
        if (event.metaKey)  parts.push('Meta');
        parts.push(name);
        const value = parts.join('+');
        stop();
        recorder.textContent = value;
        onChange(value);
    };
    recorder.addEventListener('click', () => {
        if (recording) return;
        recording = true;
        stopRecording = stop;
        recorder.classList.add('recording');
        recorder.textContent = words.recording;
        onChange('');
        window.addEventListener('keydown', onKey, true);
    });
    recorder.addEventListener('blur', () => {
        if (!recording) return;
        stop();
        recorder.textContent = words.pressKey;
    });
    return recorder;
}

function mouseBuilder(onChange) {
    const words = state.inputs.strings;
    const holder = document.createElement('span');
    holder.className = 'builder';

    const boxes = ['Shift', 'Ctrl', 'Alt'].map(name => {
        const label = document.createElement('label');
        const box = Object.assign(document.createElement('input'), {type: 'checkbox'});
        box.dataset.name = name;
        label.append(box, name);
        holder.appendChild(label);
        return box;
    });

    const held = document.createElement('select');
    held.className = 'short';
    held.appendChild(Object.assign(document.createElement('option'),
                                   {value: '', textContent: words.nothingHeld}));
    for (const button of MOUSE_BUTTONS)
        held.appendChild(Object.assign(document.createElement('option'),
                                       {value: button, textContent: words.held + ': ' + inputWord(button)}));
    holder.appendChild(held);

    const last = document.createElement('select');
    last.className = 'short';
    last.appendChild(Object.assign(document.createElement('option'), {value: '', textContent: '\u2014'}));
    for (const token of MOUSE_BUTTONS.concat(WHEELS))
        last.appendChild(Object.assign(document.createElement('option'),
                                       {value: token, textContent: inputWord(token)}));
    holder.appendChild(last);

    const update = () => {
        if (!last.value) { onChange(''); return; }
        const parts = boxes.filter(b => b.checked).map(b => b.dataset.name);
        if (held.value) parts.push(held.value);
        if (!parts.length && (last.value === 'LeftButton' || last.value === 'RightButton')) {
            onChange('');
            return;
        }
        parts.push(last.value);
        onChange(parts.join('+'));
    };
    for (const node of [...boxes, held, last]) node.addEventListener('change', update);
    return holder;
}

function gestureBuilder(onChange) {
    const words = state.inputs.strings;
    const holder = document.createElement('span');
    holder.className = 'builder';
    const strokes = [];

    const preview = document.createElement('span');
    preview.className = 'input preview';

    const redraw = () => {
        preview.textContent = strokes.map(s => ARROWS[s]).join(' ');
        onChange(strokes.join(','));
    };

    for (const stroke of ['U', 'D', 'L', 'R', 'UL', 'UR', 'DL', 'DR']) {
        const arrow = document.createElement('button');
        arrow.type = 'button';
        arrow.className = 'arrow';
        if (stroke.length === 2 && state.inputs.fourWayGestures) {
            arrow.classList.add('inactive');
            arrow.title = words.diagonalOff;
        }
        arrow.textContent = ARROWS[stroke];
        arrow.addEventListener('click', () => {
            if (strokes[strokes.length - 1] === stroke) return;
            strokes.push(stroke);
            redraw();
        });
        holder.appendChild(arrow);
    }
    const clear = document.createElement('button');
    clear.type = 'button';
    clear.className = 'arrow';
    clear.textContent = words.clear;
    clear.addEventListener('click', () => { strokes.length = 0; redraw(); });
    holder.append(clear, preview);
    return holder;
}

function addRow(table) {
    const words = state.inputs.strings;
    const row = document.createElement('div');
    row.className = 'binding adding';

    let input = '';
    const add = document.createElement('button');
    add.type = 'button';
    add.className = 'browse';
    add.textContent = words.add;
    add.disabled = true;

    const onChange = (value) => {
        input = value;
        add.disabled = !value;
    };
    const builder = table.kind === 'keys'  ? keyRecorder(onChange)
                  : table.kind === 'mouse' ? mouseBuilder(onChange)
                  : gestureBuilder(onChange);
    const select = actionSelect(table, 'NoAction');

    add.addEventListener('click', () => {
        if (!input) return;
        writeTable(table, table.entries.concat([{input: input, action: select.value}]));
    });

    row.append(builder, select, add);
    return row;
}

function inputSection(table, query) {
    const words = state.inputs.strings;
    const indexed = table.entries.map((entry, index) => ({entry, index}));
    const shown = indexed.filter(({entry}) => inputMatches(table, entry, query));
    if (query && !shown.length) return null;

    const section = document.createElement('section');
    section.className = 'input-table';
    section.dataset.group = table.group;

    const head = document.createElement('div');
    head.className = 'input-head';
    const title = document.createElement('h3');
    title.textContent = table.label;
    if (table.changed) {
        const badge = document.createElement('span');
        badge.className = 'restart';
        badge.textContent = words.changed;
        title.appendChild(badge);
    }
    head.appendChild(title);
    if (state.inputEditing && table.changed) {
        const reset = document.createElement('button');
        reset.type = 'button';
        reset.className = 'reset visible';
        reset.textContent = words.resetTable;
        reset.addEventListener('click', () => resetTable(table));
        head.appendChild(reset);
    }
    section.appendChild(head);

    if (table.hint) {
        const hint = document.createElement('p');
        hint.className = 'hint';
        hint.textContent = table.hint;
        section.appendChild(hint);
    }

    const why = shown.map(({entry}) => heldBack(table, entry)).find(Boolean);
    if (why) {
        const legend = document.createElement('p');
        legend.className = 'hint';
        legend.textContent = words.dimmed.replace('%1', why) + ' ';
        const key = table.kind === 'keys' ? 'webview/@EnableSingleKeyShortcut' : null;
        if (key && state.items.some(i => i.key === key)) {
            const link = document.createElement('a');
            link.className = 'link';
            link.href = '#page';
            link.textContent = words.openSetting;
            link.addEventListener('click', (event) => {
                event.preventDefault();
                jumpToSetting(key);
            });
            legend.appendChild(link);
        }
        section.appendChild(legend);
    }

    if (state.inputEditing && !query) section.appendChild(addRow(table));

    const list = document.createElement('div');
    list.className = 'bindings';
    for (const {entry, index} of shown) list.appendChild(bindingRow(table, entry, index));
    if (!table.entries.length) {
        const empty = document.createElement('p');
        empty.className = 'hint';
        empty.textContent = words.empty;
        list.appendChild(empty);
    }
    section.appendChild(list);
    return {section, count: shown.length};
}

function inputToolbar() {
    const words = state.inputs.strings;
    const bar = document.createElement('section');
    bar.className = 'input-toolbar';
    const edit = document.createElement('button');
    edit.type = 'button';
    edit.className = 'browse';
    edit.textContent = state.inputEditing ? words.done : words.edit;
    edit.setAttribute('aria-pressed', String(state.inputEditing));
    edit.addEventListener('click', () => {
        state.inputEditing = !state.inputEditing;
        render();
    });
    bar.appendChild(edit);
    return bar;
}

const floatingToggle = {node: null, observer: null};

function hideFloatingToggle() {
    if (floatingToggle.observer) floatingToggle.observer.disconnect();
    floatingToggle.observer = null;
    if (floatingToggle.node) floatingToggle.node.hidden = true;
}

function tableInView() {
    const header = document.querySelector('header').getBoundingClientRect().bottom;
    for (const section of el.main.querySelectorAll('.input-table')) {
        const margin = parseFloat(getComputedStyle(section).scrollMarginTop) || 0;
        if (section.getBoundingClientRect().bottom > Math.max(header, margin) + 1) return section.dataset.group;
    }
    return null;
}

function watchFloatingToggle(toolbar) {
    if (!floatingToggle.node) {
        const toggle = document.createElement('button');
        toggle.type = 'button';
        toggle.id = 'input-toggle';
        toggle.hidden = true;
        toggle.addEventListener('click', () => {
            const group = tableInView();
            state.inputEditing = !state.inputEditing;
            render();
            const section = group && el.main.querySelector(`.input-table[data-group="${group}"]`);
            if (section) section.scrollIntoView({block: 'start'});
        });
        document.body.appendChild(toggle);
        floatingToggle.node = toggle;
    }
    const words = state.inputs.strings;
    floatingToggle.node.textContent = state.inputEditing ? words.done : words.edit;
    floatingToggle.node.setAttribute('aria-pressed', String(state.inputEditing));
    floatingToggle.observer = new IntersectionObserver((entries) => {
        floatingToggle.node.hidden = entries[entries.length - 1].isIntersecting;
    });
    floatingToggle.observer.observe(toolbar);
}

function renderInputs(query, searching, label) {
    let shown = 0;
    const sections = [];
    for (const table of state.inputs.tables) {
        const drawn = inputSection(table, query);
        if (!drawn) continue;
        sections.push(drawn.section);
        shown += drawn.count;
    }
    if (!sections.length) return 0;

    if (searching) {
        const heading = document.createElement('section');
        const h2 = document.createElement('h2');
        h2.textContent = label;
        heading.appendChild(h2);
        el.main.appendChild(heading);
    }
    const toolbar = inputToolbar();
    el.main.appendChild(toolbar);
    for (const section of sections) el.main.appendChild(section);
    watchFloatingToggle(toolbar);
    if (state.inputBusy) holdInputs();
    return Math.max(shown, 1);
}

async function refreshInputs() {
    try {
        state.inputs = await api('inputs');
        render();
    } catch (e) {
    }
}

const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));

function shownOrigin(origin) {
    return origin.replace(/^(https:\/\/.*):443$/, '$1').replace(/^(http:\/\/.*):80$/, '$1');
}

async function refreshPasswords() {
    try {
        state.passwords = await api('passwords');
        render();
    } catch (e) {
    }
}

async function settlePasswords() {
    for (let i = 0; i < 40; i++) {
        await sleep(i ? 500 : 150);
        try {
            state.passwords = await api('passwords');
        } catch (e) {
            break;
        }
        if (!state.passwords.pending) break;
    }
    render();
}

function sameEntry(a, b) {
    return a.space === b.space && a.origin === b.origin && a.username === b.username;
}

async function removePassword(entry, button) {
    const words = state.passwords.strings;
    button.disabled = true;
    button.textContent = words.removing;
    try {
        await api('passwords-remove', entry);
    } catch (e) {
        toast(words.removeFailed);
        render();
        return;
    }
    await settlePasswords();
    if (state.passwords.entries.some(e => sameEntry(e, entry))) toast(words.removeFailed);
}

async function removeAllPasswords() {
    const words = state.passwords.strings;
    if (!window.confirm(words.confirmAll)) return;
    try {
        await api('passwords-remove-all', {});
    } catch (e) {
        toast(words.removeFailed);
        return;
    }
    await settlePasswords();
}

function passwordMatches(entry, query, label) {
    if (!query) return true;
    const haystack = (entry.origin + ' ' + entry.username + ' ' + label).toLowerCase();
    return query.split(/\s+/).every(word => haystack.includes(word));
}

function renderPasswords(query, searching, label) {
    const data = state.passwords;
    if (!data) return 0;
    const words = data.strings;
    const entries = data.entries.filter(e => passwordMatches(e, query, label));
    if (searching && !entries.length) return 0;

    const section = document.createElement('section');
    if (searching) {
        const heading = document.createElement('h2');
        heading.textContent = label;
        section.appendChild(heading);
    }

    const notes = [];
    if (!data.available) notes.push(words.unavailable);
    if (!searching && data.available && !data.entries.length) notes.push(words.none);
    for (const note of notes) section.appendChild(noticeLine(note));

    const spaces = new Set(data.entries.map(e => e.space));
    for (const entry of entries) {
        const row = document.createElement('div');
        row.className = 'row';
        const text = document.createElement('div');
        text.className = 'text';
        const site = document.createElement('span');
        site.className = 'label';
        site.textContent = shownOrigin(entry.origin);
        text.appendChild(site);
        const who = document.createElement('span');
        who.className = 'hint';
        who.textContent = (entry.username || words.noUsername) +
            (spaces.size > 1 ? '  (' + words.space + ': ' + entry.space + ')' : '');
        text.appendChild(who);
        row.appendChild(text);

        const holder = document.createElement('div');
        holder.className = 'control';
        const button = document.createElement('button');
        button.type = 'button';
        button.className = 'browse';
        button.textContent = words.remove;
        button.addEventListener('click', () => removePassword(entry, button));
        holder.appendChild(button);
        row.appendChild(holder);
        section.appendChild(row);
    }

    if (!searching && (data.edge || data.entries.length)) {
        const row = document.createElement('div');
        row.className = 'row';
        const text = document.createElement('div');
        text.className = 'text';
        if (data.edge) {
            const hint = document.createElement('span');
            hint.className = 'hint';
            hint.textContent = words.edge;
            text.appendChild(hint);
        }
        row.appendChild(text);
        const holder = document.createElement('div');
        holder.className = 'control';
        const button = document.createElement('button');
        button.type = 'button';
        button.className = 'browse';
        button.textContent = words.removeAll;
        button.addEventListener('click', removeAllPasswords);
        holder.appendChild(button);
        row.appendChild(holder);
        section.appendChild(row);
    }

    el.main.appendChild(section);
    return Math.max(entries.length, 1);
}

let downloadsTimer = 0;

async function refreshDownloads() {
    clearTimeout(downloadsTimer);
    try {
        state.downloads = await api('downloads');
        render();
    } catch (e) {
    }
    if (state.current === 'downloads' && !state.query.trim() &&
        state.downloads && state.downloads.entries.some(e => e.state === 'running'))
        downloadsTimer = setTimeout(refreshDownloads, 1000);
}

async function downloadAction(endpoint, id) {
    try {
        await api(endpoint, {id: id});
    } catch (e) {
        toast(state.strings.failed);
    }
    refreshDownloads();
}

function size(bytes) {
    if (!(bytes > 0)) return '';
    const units = ['B', 'KB', 'MB', 'GB', 'TB'];
    let value = bytes, unit = 0;
    while (value >= 1024 && unit < units.length - 1) { value /= 1024; unit++; }
    return (unit ? value.toFixed(value < 10 ? 1 : 0) : value) + ' ' + units[unit];
}

function downloadMatches(entry, query, label) {
    if (!query) return true;
    const haystack = (entry.name + ' ' + entry.url + ' ' + label).toLowerCase();
    return query.split(/\s+/).every(word => haystack.includes(word));
}

function actionButton(text, handler) {
    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'browse';
    button.textContent = text;
    button.addEventListener('click', handler);
    return button;
}

function renderDownloads(query, searching, label) {
    const data = state.downloads;
    if (!data) return 0;
    const words = data.strings;
    const entries = data.entries.filter(e => downloadMatches(e, query, label));
    if (searching && !entries.length) return 0;

    const section = document.createElement('section');
    if (searching) {
        const heading = document.createElement('h2');
        heading.textContent = label;
        section.appendChild(heading);
    }

    if (!data.entries.length && !searching) section.appendChild(noticeLine(words.none));

    for (const entry of entries) {
        const row = document.createElement('div');
        row.className = 'row';
        const text = document.createElement('div');
        text.className = 'text';
        const name = document.createElement('span');
        name.className = 'label';
        name.textContent = entry.name;
        text.appendChild(name);

        const sizes = entry.state === 'running'
            ? size(entry.received) + (entry.total > 0 ? ' / ' + size(entry.total) : '')
            : size(entry.state === 'completed' && entry.total > 0 ? entry.total : entry.received);
        const parts = [words[entry.state] || entry.state];
        if (sizes) parts.push(sizes);
        if (entry.state !== 'running' && !entry.exists) parts.push(words.missing);
        const status = document.createElement('span');
        status.className = 'hint';
        status.textContent = parts.join('  ·  ');
        text.appendChild(status);

        if (entry.url) {
            const from = document.createElement('span');
            from.className = 'hint';
            from.textContent = entry.url;
            text.appendChild(from);
        }
        row.appendChild(text);

        const holder = document.createElement('div');
        holder.className = 'control';
        if (entry.state === 'running') {
            holder.appendChild(actionButton(words.cancel,
                () => downloadAction('downloads-cancel', entry.id)));
        } else {
            if (entry.canOpen)
                holder.appendChild(actionButton(words.open,
                    () => downloadAction('downloads-open', entry.id)));
            if (entry.exists)
                holder.appendChild(actionButton(words.show,
                    () => downloadAction('downloads-show', entry.id)));
            holder.appendChild(actionButton(words.remove,
                () => downloadAction('downloads-remove', entry.id)));
        }
        row.appendChild(holder);
        section.appendChild(row);
    }

    if (!searching && data.entries.some(e => e.state !== 'running')) {
        const row = document.createElement('div');
        row.className = 'row';
        const text = document.createElement('div');
        text.className = 'text';
        row.appendChild(text);
        const holder = document.createElement('div');
        holder.className = 'control';
        holder.appendChild(actionButton(words.clear,
            () => downloadAction('downloads-clear', 0)));
        row.appendChild(holder);
        section.appendChild(row);
    }

    el.main.appendChild(section);
    return Math.max(entries.length, 1);
}

let directoryLoading = false;

async function refreshDirectory(id) {
    if (directoryLoading) return;
    directoryLoading = true;
    try {
        const query = id ? 'directory?node=' + encodeURIComponent(id) : 'directory';
        const data = await api(query);
        if (data.fallback && state.directory) toast(data.strings.gone);
        state.directory = data;
        state.directoryId = data.current || null;
        render();
    } catch (e) {
    } finally {
        directoryLoading = false;
    }
}

function directoryTokenState(tokens, sw) {
    const on = new RegExp('^' + sw.pattern + '$');
    const off = new RegExp('^!' + sw.pattern + '$');
    let st = -1;
    if (tokens.some(t => off.test(t))) st = 0;
    else if (tokens.some(t => on.test(t))) st = 1;
    return (sw.inverted && st !== -1) ? 1 - st : st;
}

function directoryUnclaimed(tokens) {
    return tokens.filter(t =>
        !state.directory.switches.some(sw =>
            new RegExp('^!?' + sw.pattern + '$').test(t)));
}

function directoryRow(labelText, control, hintText) {
    const row = document.createElement('div');
    row.className = 'row' + (control.type === 'text' ? ' wide' : '');
    const text = document.createElement('div');
    text.className = 'text';
    const label = document.createElement('span');
    label.className = 'label';
    label.textContent = labelText;
    text.appendChild(label);
    if (hintText) {
        const hint = document.createElement('span');
        hint.className = 'hint';
        hint.textContent = hintText;
        text.appendChild(hint);
    }
    row.appendChild(text);
    const holder = document.createElement('div');
    holder.className = 'control';
    holder.appendChild(control);
    row.appendChild(holder);
    return row;
}

function directoryCard(node, depth, current) {
    const data = state.directory;
    const strings = data.strings;

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
    name.setAttribute('aria-label', strings.name);
    head.appendChild(name);
    head.appendChild(profile);
    if (!current) {
        const show = document.createElement('button');
        show.type = 'button';
        show.className = 'browse';
        show.textContent = strings.showThis;
        show.addEventListener('click', () => refreshDirectory(node.id));
        head.appendChild(show);
    }
    card.appendChild(head);

    for (const sw of data.switches) {
        const st = directoryTokenState(node.tokens, sw);
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
        card.appendChild(directoryRow(sw.label, control, sw.hint));
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
    extra.value = directoryUnclaimed(node.tokens).join('; ');
    card.appendChild(directoryRow(strings.otherTokens, extra, strings.otherTokensHint));

    const agents = document.createElement('p');
    agents.className = 'agents';
    agents.textContent = strings.userAgentNames + '\n ' + data.useragents.join(', ');
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
            const reply = await api('directory-set',
                {node: node.id, name: name.value.trim(), tokens: tokens});
            if (reply.error) throw new Error(reply.error);
            await refreshDirectory(state.directoryId);
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
    for (const {node: control} of controls) control.addEventListener('change', submit);

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
        { duration: Math.min(300, 150 + Math.abs(to - from) / 4),
          easing: 'ease-in-out' });
    animation.onfinish = () => {
        if (details.dataset.closing === '1') {
            delete details.dataset.closing;
            details.open = false;
        }
    };
}

function renderDirectory() {
    const data = state.directory;
    if (!data) return 0;
    const strings = data.strings;
    const section = document.createElement('section');
    const panel = document.createElement('div');
    panel.className = 'directory-panel';
    section.appendChild(panel);

    if (data.tree && data.tree.length) {
        const outline = document.createElement('details');
        outline.className = 'dir-tree';
        outline.open = state.directoryTreeOpen || data.tree.length === 1;
        outline.addEventListener('toggle', () => { state.directoryTreeOpen = outline.open; });
        const heading = document.createElement('summary');
        heading.textContent = strings.tree;
        outline.appendChild(heading);
        heading.addEventListener('click', e => {
            e.preventDefault();
            toggleAnimated(outline, heading);
        });
        const list = document.createElement('nav');
        list.className = 'dir-tree-list';
        list.setAttribute('aria-label', strings.tree);
        if (state.directoryTreeHeight) list.style.height = state.directoryTreeHeight;
        new ResizeObserver(() => {
            if (outline.open && list.style.height) state.directoryTreeHeight = list.style.height;
        }).observe(list);
        outline.appendChild(list);
        const above = new Set((data.chain || []).map(node => node.id));
        for (const entry of data.tree) {
            const button = document.createElement('button');
            button.type = 'button';
            button.style.setProperty('--depth', entry.depth);
            button.textContent = entry.name || strings.unnamed;
            if (entry.id === data.current) {
                button.className = 'current';
                button.setAttribute('aria-current', 'true');
            } else if (above.has(entry.id)) {
                button.className = 'above';
            }
            button.addEventListener('click', () => refreshDirectory(entry.id));
            list.appendChild(button);
        }
        panel.appendChild(outline);
    }

    const intro = document.createElement('p');
    intro.className = 'intro';
    intro.textContent = strings.inheritance;
    panel.appendChild(intro);

    const chain = data.chain || [];
    if (chain.length > 1) {
        const above = document.createElement('details');
        above.open = state.directoryAboveOpen || false;
        above.addEventListener('toggle', () => { state.directoryAboveOpen = above.open; });
        const summary = document.createElement('summary');
        for (const [className, text] of [['when-closed', strings.ancestorsShow],
                                          ['when-open', strings.ancestorsHide]]) {
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
            above.appendChild(directoryCard(node, depth, false));
        });
        panel.appendChild(above);
    }
    if (chain.length) {
        const current = directoryCard(chain[chain.length - 1], chain.length - 1, true);
        current.classList.add('current');
        const heading = document.createElement('h2');
        heading.className = 'current-label';
        heading.textContent = strings.currentDirectory;
        current.prepend(heading);
        panel.appendChild(current);
    }

    el.main.appendChild(section);
    return 1;
}

function refreshApplicability() {
    for (const row of el.main.querySelectorAll('.row')) {
        const item = state.items.find(i => i.key === row.dataset.key);
        if (!item || !item.appliesWhen) continue;

        const other = state.items.find(i => i.key === item.appliesWhen.key);
        const applies = !other || other.value === item.appliesWhen.value;

        row.classList.toggle('inapplicable', !applies);
        for (const node of row.querySelectorAll('input, select, textarea, button'))
            node.disabled = !applies || node.dataset.busy === 'true';

        const note = row.querySelector('.note');
        note.hidden = applies;
        if (!applies)
            note.textContent =
                (item.appliesWhen.value ? state.strings.notUsedWhileOff
                                        : state.strings.notUsedWhileOn)
                .replace('%1', other.label);
    }
}

function matches(item, query, categoryLabel) {
    if (!query) return true;
    const haystack = (item.label + ' ' + (item.hint || '') + ' ' +
                      (item.choiceLabels || []).join(' ') + ' ' +
                      categoryLabel + ' ' + item.key).toLowerCase();
    return query.split(/\s+/).every(word => haystack.includes(word));
}

function render() {
    if (stopRecording) stopRecording();
    hideFloatingToggle();
    const query = state.query.trim().toLowerCase();
    const searching = query !== '';

    el.main.querySelectorAll('section').forEach(s => s.remove());

    document.body.classList.toggle('wide',
        !searching && !!state.inputs && state.current === state.inputs.category);

    let shown = 0;
    for (const category of state.categories) {
        if (!searching && category.name !== state.current) continue;

        if (state.inputs && category.name === state.inputs.category) {
            shown += renderInputs(query, searching, category.label);
            continue;
        }
        if (category.name === 'passwords') {
            shown += renderPasswords(query, searching, category.label);
            continue;
        }
        if (category.name === 'downloads') {
            shown += renderDownloads(query, searching, category.label);
            continue;
        }
        if (category.name === 'directory') {
            if (!searching) shown += renderDirectory();
            continue;
        }

        const items = state.items.filter(
            i => i.category === category.name && matches(i, query, category.label));
        if (!items.length) continue;

        const section = document.createElement('section');
        if (searching) {
            const heading = document.createElement('h2');
            heading.textContent = category.label;
            section.appendChild(heading);
        }
        for (const item of items) section.appendChild(buildRow(item));
        el.main.appendChild(section);
        shown += items.length;
    }

    refreshApplicability();

    el.empty.hidden = shown !== 0;
    el.empty.textContent = state.strings.noMatch;

    for (const button of el.nav.children)
        button.setAttribute('aria-current', String(!searching && button.dataset.name === state.current));
}

function jumpToSetting(key) {
    const item = state.items.find(i => i.key === key);
    if (!item) return;
    state.jumpTo = key;
    if (window.location.hash.slice(1) === item.category) showCategory(item.category);
    else window.location.hash = item.category;
}

function showCategory(name) {
    state.current = name;
    state.query = '';
    el.search.value = '';
    window.scrollTo(0, 0);
    render();
    if (state.jumpTo) {
        const row = el.main.querySelector('.row[data-key="' + CSS.escape(state.jumpTo) + '"]');
        state.jumpTo = null;
        if (row) {
            row.scrollIntoView({block: 'center'});
            row.classList.add('spot');
            setTimeout(() => row.classList.remove('spot'), 1800);
        }
    }
    if (state.inputs && name === state.inputs.category) refreshInputs();
    if (name === 'passwords') refreshPasswords();
    if (name === 'downloads') refreshDownloads();
    if (name === 'directory') refreshDirectory(state.directoryId);
}

function buildNav() {
    for (const category of state.categories) {
        const button = document.createElement('button');
        button.type = 'button';
        button.dataset.name = category.name;
        button.textContent = category.label;
        button.addEventListener('click', () => {
            if (window.location.hash.slice(1) === category.name) showCategory(category.name);
            else window.location.hash = category.name;
        });
        el.nav.appendChild(button);
    }
}

function categoryOfFragment() {
    const name = window.location.hash.slice(1);
    if (state.categories.some(category => category.name === name)) return name;
    return state.categories.length ? state.categories[0].name : null;
}

async function main() {
    const schema = await api('schema');
    state.strings = schema.strings;
    state.categories = schema.categories;
    state.items = schema.items;
    state.inputs = schema.inputs || null;
    state.current = categoryOfFragment();

    document.title = state.strings.title;
    document.getElementById('title').textContent = state.strings.title;
    el.search.placeholder = state.strings.search;

    buildNav();
    render();
    if (state.categories.some(category => category.name === 'passwords')) refreshPasswords();
    if (state.categories.some(category => category.name === 'downloads')) refreshDownloads();
    if (state.current === 'directory') refreshDirectory(null);

    window.addEventListener('hashchange', () => {
        const name = categoryOfFragment();
        if (name !== null) showCategory(name);
    });

    let searchTimer = 0;
    el.search.addEventListener('input', () => {
        clearTimeout(searchTimer);
        searchTimer = setTimeout(() => {
            state.query = el.search.value;
            render();
        }, 90);
    });
}

main().catch(e => {
    const prefix = (state.strings && state.strings.loadFailed)
        || 'Could not load the settings: ';
    document.body.textContent = prefix + e.message;
});

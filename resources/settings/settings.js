'use strict';

/*

  The settings page.

  Everything it knows comes from '/api/schema': the sections, the settings in
  them, what type each one is, its default, the value in force, and the words
  this page says for itself. Nothing about any particular setting is written
  here, so adding one to 'settingsschema.cpp' is enough to make it appear.

  No English is written here either, for the same reason: this page runs
  inside Chromium and cannot reach the application's translations, so
  anything it displays has to arrive already translated.

  A change is sent as it is made and applied immediately. There is no save
  button because there is nothing to save: the application re-reads its
  settings on every write.

*/

const state = {
    categories: [],
    items: [],
    strings: null,
    current: null,
    query: '',
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

// ---------------------------------------------------------------- controls

function sameValue(a, b) {
    if (Array.isArray(a) && Array.isArray(b))
        return a.length === b.length && a.every((v, i) => v === b[i]);
    return a === b;
}

// each type answers the same three questions: what widget, how to read it,
// and how to write into it.
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
            for (const choice of item.choices) {
                const option = document.createElement('option');
                option.value = choice;
                option.textContent = choice;
                node.appendChild(option);
            }
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

// ------------------------------------------------------------------- rows

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

    // filled in by 'refreshApplicability', which is what decides whether this
    // row is in use at all.
    if (item.appliesWhen) {
        const note = document.createElement('span');
        note.className = 'hint note';
        note.hidden = true;
        text.appendChild(note);
    }
    row.appendChild(text);

    const holder = document.createElement('div');
    holder.className = 'control';

    const node = control.make(item);
    node.id = 'c-' + item.key.replace(/[^A-Za-z0-9]/g, '-');
    label.setAttribute('for', node.id);
    control.write(node, item.value);
    holder.appendChild(node);

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

    node.addEventListener(control.event, async () => {
        const value = control.read(node);
        if (value === null) {           // an emptied number box: put it back.
            control.write(node, item.value);
            return;
        }
        try {
            const reply = await api('set', {key: item.key, value: value});
            if (reply.error) throw new Error(reply.error);
            item.value = reply.value;
            control.write(node, item.value);
            row.classList.remove('invalid');
            markChanged();
            // this may be the setting another row hangs off.
            refreshApplicability();
            toast(item.needsRestart ? state.strings.savedRestart : state.strings.saved);
        } catch (e) {
            // leave what was typed on screen so it can be corrected, and say
            // so rather than pretending the change took.
            row.classList.add('invalid');
            toast(state.strings.failed);
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

// ------------------------------------------------------------------ render

// some settings stop meaning anything while another one is set a certain way:
// 'appliesWhen' names that other setting and the value it has to have. Such a
// row keeps its place -- it is still a setting, and the search still finds it
// -- but it says which setting took it out of use and stops accepting input,
// so that nothing here looks as though it were in force when it is not.
function refreshApplicability() {
    for (const row of el.main.querySelectorAll('.row')) {
        const item = state.items.find(i => i.key === row.dataset.key);
        if (!item || !item.appliesWhen) continue;

        const other = state.items.find(i => i.key === item.appliesWhen.key);
        const applies = !other || other.value === item.appliesWhen.value;

        row.classList.toggle('inapplicable', !applies);
        for (const node of row.querySelectorAll('input, select, textarea, button'))
            node.disabled = !applies;

        const note = row.querySelector('.note');
        note.hidden = applies;
        // the sentence names the other setting by its label, so it can be
        // found: the key is what the page has, not what the reader sees.
        if (!applies)
            note.textContent =
                (item.appliesWhen.value ? state.strings.notUsedWhileOff
                                        : state.strings.notUsedWhileOn)
                .replace('%1', other.label);
    }
}

// the section a setting sits in is part of what it is called: 'Access keys' is
// written at the head of that section and nowhere in the settings under it, so
// leaving it out of the haystack means those settings can only be found by
// their key -- which is English whatever language the page is in -- and not by
// the name the reader was actually shown.
function matches(item, query, categoryLabel) {
    if (!query) return true;
    const haystack = (item.label + ' ' + (item.hint || '') + ' ' +
                      categoryLabel + ' ' + item.key).toLowerCase();
    return query.split(/\s+/).every(word => haystack.includes(word));
}

function render() {
    const query = state.query.trim().toLowerCase();
    const searching = query !== '';

    el.main.querySelectorAll('section').forEach(s => s.remove());

    let shown = 0;
    for (const category of state.categories) {
        // while searching, every section is considered; otherwise only the
        // one that is selected.
        if (!searching && category.name !== state.current) continue;

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

function buildNav() {
    for (const category of state.categories) {
        const button = document.createElement('button');
        button.type = 'button';
        button.dataset.name = category.name;
        button.textContent = category.label;
        button.addEventListener('click', () => {
            state.current = category.name;
            state.query = '';
            el.search.value = '';
            el.main.scrollTop = 0;
            render();
        });
        el.nav.appendChild(button);
    }
}

// -------------------------------------------------------------------- boot

async function main() {
    const schema = await api('schema');
    // the words the page says for itself arrive translated with everything
    // else, because nothing in here can reach the application's translations
    // on its own. 'state.strings' is only ever English if the fetch failed.
    state.strings = schema.strings;
    state.categories = schema.categories;
    state.items = schema.items;
    state.current = state.categories.length ? state.categories[0].name : null;

    document.title = state.strings.title;
    document.getElementById('title').textContent = state.strings.title;
    el.search.placeholder = state.strings.search;

    buildNav();
    render();

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
    // if '/api/schema' itself is what failed there is no translation to use,
    // so the English is kept as the fallback rather than showing nothing.
    const prefix = (state.strings && state.strings.loadFailed)
        || 'Could not load the settings: ';
    document.body.textContent = prefix + e.message;
});

# Licence texts of what is shipped beside vanilla (vendored)

The package carries the Qt libraries and Qt WebEngine, so it has to carry
their licence texts as well (LGPLv3 asks for the licence itself to travel
with the binaries). The texts are taken unmodified from the Qt sources:

| file | from | SHA256 |
|---|---|---|
| `LGPL-3.0-only.txt` | `qtwebengine/LICENSES/` of Qt WebEngine 6.140.0 (Qt 6.12.0; unchanged from Qt 6.11.2) | `EA7D049C7705DC13AFC202DD18E1827F3484F8212FD3FA7B82FC4A0C363432C9` |
| `GPL-3.0-only.txt` | `qtwebengine/LICENSES/` of Qt WebEngine 6.140.0 (Qt 6.12.0; unchanged from Qt 6.11.2) | `0B383D5A63DA644F628D99C33976EA6487ED89AAA59F0B3257992DEAC1171E6B` |
| `LICENSE.Chromium` | `qtwebengine/` of Qt WebEngine 6.140.0 (Qt 6.12.0; unchanged from Qt 6.11.2) | `9A6D3B2FB747638198147C6AF23E6BDC5B5D929B75E18B4E6DB656596D39811F` |

`NOTICE.txt` is ours: it names what ships, under which licence, where the
source is, and that Qt is linked dynamically. **It carries version numbers
(Qt 6.12.0, Qt WebEngine 6.140.0, Chromium 140.0.7339.225 -- chrome/VERSION of the shipped sources;
Qt WebEngine is versioned apart from Qt since 6.12) -- when the Qt or the Qt WebEngine this is built against
changes, both this table and `NOTICE.txt` have to be updated.**

The install rules put all four files in `licenses/qt/` of the package,
next to `licenses/webview2/`. The application's own `LICENSE` goes beside
the executable.

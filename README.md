# vanilla

[日本語はこちら](#日本語)

A desktop web browser built with Qt, designed around tree-structured tab management.

It manages pages and directories, and a directory can hold both pages and other directories.
Combining a thumbnail overview, keyboard and mouse gesture control, and command input,
it aims to keep large numbers of pages open while preserving their structure.

## Features

- **Tree-structured tabs** — Organize pages in nested directories, then open, close, move, and collapse them with the structure intact
- **Tabs carry over** — The tab tree is restored at every start. There is no setting to start without the previous tabs
- **Tab list** — See the tree as a table of thumbnails and organize it directly (`Gadgets` / `GraphicsTableView`)
- **Per-directory settings** — Tokens in a directory's name apply profile separation, disable JavaScript, switch engines, and more for everything under it. Settings are inherited downward ("Directory" in the settings page)
- **Multiple rendering engines** — The default on Windows x64 is a view that **hosts Edge WebView2 directly** (`EdgeWebView`); on other operating systems it is Qt WebEngine. Either can be chosen per directory. There are also a QML WebEngine view and a Qt WebView-based view
- **Chrome extensions** — Loads Manifest V3 extensions from unpacked folders (see [docs/EXTENSIONS.md](docs/EXTENSIONS.md) for what is supported)
- **Command bar** — A command input in the style of Emacs's Anything/Helm. Narrow down and choose from commands, tabs, trash, settings, and web search suggestions
- **Address bar** — Suggests the current tab's surroundings (parent URLs, tabs at the same level, typed URL history) and actions on the page
- **Start page** — A search box, cards for recently viewed tabs, and a tab history that includes closed tabs (`vanilla://start`)
- **Keyboard and mouse gestures** — Most actions can be bound to keys and gestures. Bindings can be changed in the settings page (`vanilla://settings`)
- **Password saving** — Offers to save passwords from login forms and fills in the account you choose. Qt WebEngine views keep passwords in the operating system's credential store
- **Minimap** — A miniature scrollbar that shows the page's text lines, images, and controls as a diagram (VS Code style, `MiniMap`; off by default)
- **Light / Dark colour schemes** — Follows the desktop theme automatically, or can be set to either scheme
- **Japanese and English** — UI translations included

## Requirements

| Item | Details |
|---|---|
| Qt | **Tested with Qt 6.12.0 + Qt WebEngine 6.140.0 (Windows) and Qt 6.10.2 (Linux)** (the CMake build requires Qt 6.5 or later). Does not work with Qt 5 |
| Compiler | Tested with **MSVC 2022 (Windows)** and **GCC 15 (Linux)**. Not yet built on macOS |
| Required Qt modules | Core, Gui, Widgets, Network, Xml, OpenGL(+Widgets), WebChannel, Multimedia(+Widgets), Qml, Quick(+Widgets). LinguistTools at build time only |
| Optional Qt modules | **WebEngine** (the default view outside Windows), PrintSupport, WebView (enables `QuickNativeWebView`) |
| Optional SDK | With the WebView2 SDK in `third_party/webview2`, `EdgeWebView` is enabled on Windows x64 and becomes the default view (WebEngine if the WebView2 runtime is missing) |

The build system is CMake.
See [docs/BUILD.md](docs/BUILD.md) for build instructions and how to run the tests.

## Documentation

The documents below are written in Japanese.

- [docs/BUILD.md](docs/BUILD.md) — Build instructions, tests, and packaging
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — Main classes and their responsibilities, and where data is stored
- [docs/EXTENSIONS.md](docs/EXTENSIONS.md) — Chrome extension support (by namespace and by method)

## Distribution

- Releases are on [GitHub Releases](https://github.com/rallentando/vanilla/releases) as a portable
  zip archive. Settings and sessions are created in `data/` next to the executable
- Licence notices for the bundled Qt, Qt WebEngine, and Edge WebView2 loader are in `licenses/` in the zip
- The executable is **not code-signed**. Windows SmartScreen may warn on first launch
  (choose "More info" and then "Run anyway").
  The SHA-256 checksum of the zip is published in the release notes, so please verify it before running the executable
- In Qt WebEngine views, some extension features (such as Vimium's vomnibar) need a patched Qt WebEngine, which the releases do not include.
  Edge views do not need the patch. See "Qt WebEngine へのパッチ" in [docs/BUILD.md](docs/BUILD.md) to build one yourself

### If Qt WebEngine views display incorrectly

The graphics API of Qt WebEngine views is chosen with **"Graphics API (WebEngineView)"** under
"Rendering and engine" in the settings page (`vanilla://settings`), and the setting takes effect after a restart.
The default is `OpenGL` on Windows and `Auto` elsewhere. `Auto` leaves the choice to Qt
(Direct3D 11 on Windows, Metal on macOS).

- If the screen stays blank or is slow (`OpenGL` on Windows): change it to `Auto`
- If small black boxes appear across the page, or half the screen flashes black (Direct3D 11 and others): change it to `OpenGL`

The choices are `Auto` / `Software` / `OpenGL` / `Direct3D 11` / `Direct3D 12` / `Vulkan` / `Metal`.
A choice the operating system does not support is treated as `Auto`. The environment variable `QSG_RHI_BACKEND` takes priority when it is set.

## License

BSD 3-Clause License. Copyright (c) 2016, rallentando. See [LICENSE](LICENSE) for the full text.

---

## 日本語

木構造のタブ管理を中心に据えた、Qt 製のデスクトップ Web ブラウザ。

ページとディレクトリを管理でき、ディレクトリの中にはページもディレクトリも置ける。
サムネイル一覧、キーボードとマウスジェスチャによる操作、コマンド入力を組み合わせて、
大量のページを開いたまま構造を保って扱うことを狙っている。

### 主な特徴

- **木構造のタブ管理** — ページをディレクトリの入れ子で整理し、構造を保ったまま開閉・移動・折りたたみができる
- **タブは常に引き継がれる** — 起動のたびにタブのツリーを復元する。前回のタブを引き継がずに起動する設定はない
- **タブ一覧** — 木構造をサムネイルのテーブルとして俯瞰し、そのまま整理できる（`Gadgets` / `GraphicsTableView`）
- **ディレクトリ固有の設定** — ディレクトリ名にトークンを書くと、その配下だけプロファイル分離・JavaScript 無効・エンジン切り替えなどが効く。設定は下の階層へ継承される（設定画面の「ディレクトリ」）
- **複数のレンダリングエンジン** — Windows x64 の既定は **Edge WebView2 を直接ホストするビュー**（`EdgeWebView`）、それ以外の OS の既定は Qt WebEngine。どちらもディレクトリ単位で選べる。ほかに QML 版 WebEngine と Qt WebView 経由の実装がある
- **Chrome 拡張** — 展開済みのフォルダから Manifest V3 の拡張を読み込める（対応状況は [docs/EXTENSIONS.md](docs/EXTENSIONS.md)）
- **コマンドバー** — Emacs の Anything/Helm ライクなコマンド入力欄。コマンド・タブ・ゴミ箱・設定・Web 検索の候補を絞り込んで選べる
- **アドレスバー** — 候補は今のタブとその周辺（上の階層の URL、同じ階層のタブ、入力した URL の履歴）とページへの操作
- **スタートページ** — 検索欄、最後に見た順のタブのカード、閉じたタブも含むタブ履歴（`vanilla://start`）
- **キーボード操作とマウスジェスチャ** — 大半の操作にキーとジェスチャを割り当て可能。割り当ては設定画面（`vanilla://settings`）で変えられる
- **パスワードの保存** — ログインフォームで保存するかを聞き、選んだアカウントを入力する。Qt WebEngine のビューでは OS の資格情報の保管庫に保存する
- **ミニマップ** — ページの構造（テキスト行・画像・コントロール）を図式化した縮小スクロールバー（VSCode 方式、`MiniMap`。ウィンドウごとに「表示」メニューで出し入れする。既定はオフ）
- **Light / Dark 配色** — デスクトップのテーマに自動追従（固定も可）
- **日本語 / 英語** — UI 翻訳を同梱

### 動作環境

| 項目 | 内容 |
|---|---|
| Qt | **Qt 6.12.0 + Qt WebEngine 6.140.0（Windows）と Qt 6.10.2（Linux）で動作確認済み**（CMake の要求は Qt 6.5 以上）。Qt 5 では動かない |
| コンパイラ | **MSVC 2022（Windows）** と **GCC 15（Linux）** で確認済み。macOS はまだビルドしていない |
| 必須 Qt モジュール | Core, Gui, Widgets, Network, Xml, OpenGL(+Widgets), WebChannel, Multimedia(+Widgets), Qml, Quick(+Widgets)。ビルド時のみ LinguistTools |
| 任意 Qt モジュール | **WebEngine**（Windows 以外の既定のビュー）, PrintSupport, WebView（`QuickNativeWebView` が有効になる） |
| 任意 SDK | `third_party/webview2` に WebView2 SDK があると、Windows x64 で `EdgeWebView` が有効になり、既定のビューになる（WebView2 ランタイムが無ければ WebEngine） |

ビルドシステムは CMake。
手順とテストの実行方法は [docs/BUILD.md](docs/BUILD.md) を参照。

### ドキュメント

- [docs/BUILD.md](docs/BUILD.md) — ビルド手順・テスト・配布物の作り方
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — 主要クラスと責務、データの置き場
- [docs/EXTENSIONS.md](docs/EXTENSIONS.md) — Chrome 拡張の対応状況（名前空間ごとの分類と、メソッド単位の表）

### 配布物について

- 配布は [GitHub の Releases](https://github.com/rallentando/vanilla/releases) で行う。zip 形式の
  ポータブル構成で、設定とセッションは実行ファイルの隣の `data/` に作られる
- 同梱している Qt / Qt WebEngine / Edge WebView2 ローダのライセンス通知は zip の `licenses/` にある
- 実行ファイルは**コード署名をしていない**。初回起動時に Windows の SmartScreen が
  警告を出すことがある（「詳細情報」→「実行」で起動できる）。
  配布 zip の SHA-256 をリリースノートで公開するので、照合のうえ実行してほしい
- Qt WebEngine のビューでは、拡張の一部の機能（Vimium の vomnibar など）にパッチを当てた Qt WebEngine が必要で、配布物には入っていない。
  Edge のビューでは不要。自分でビルドする場合は [docs/BUILD.md](docs/BUILD.md)「Qt WebEngine へのパッチ」を参照

#### Qt WebEngine のビューが正常に表示されない場合

Qt WebEngine のビューの描画 API は、設定画面（`vanilla://settings`）の「描画とエンジン」にある
**「描画 API（WebEngineView）」** で選び、再起動すると反映される。既定は Windows で `OpenGL`、ほかの OS で `Auto` で、
`Auto` は Qt に任せる（Windows は Direct3D 11、macOS は Metal）。

- 画面が出ない、または遅いとき（Windows の `OpenGL`）：`Auto` に変える
- ページに黒い小さなボックスが並ぶ、画面の半分が一瞬黒くなるとき（Direct3D 11 など）：`OpenGL` に変える

選べるのは `Auto` / `Software` / `OpenGL` / `Direct3D 11` / `Direct3D 12` / `Vulkan` / `Metal` で、
その OS で動かないものを選んだときは `Auto` と同じになる。環境変数 `QSG_RHI_BACKEND` を設定して起動した場合はそちらが優先される。

### ライセンス

BSD 3-Clause License. Copyright (c) 2016, rallentando. 全文は [LICENSE](LICENSE) を参照。

---

（Deodorized. This tree is published with its source comments
and internal annotations stripped; the diff against the original
is [deodorant.zip](docs/deodorant.zip).）

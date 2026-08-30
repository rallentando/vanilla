# アーキテクチャ

最終更新: 2026-08-26

主要クラスと責務の**地図**。細部ではなく「どこを読めばよいか」を示すことを目的とする。
コードは大きなファイルに集中しているので（`src/ui/treebar.cpp` 161KB、`src/gadgets/graphicstableview.cpp` 128KB、
`src/ui/treebank.cpp` 124KB、`src/view/view.cpp` 108KB）、
まずここで当たりを付けてから読むこと。

---

## ディレクトリ構成

ソースは `src/` の下に**責務ごと**に置く（D-033）。

| どこ | 何が | 目印 |
|---|---|---|
| [src/core/](../src/core/) | `Application` にも GUI にも依存しない層 | **テストが直接呼べる。** `Hooks` で外を受け取る |
| [src/app/](../src/app/) | 起動、グローバル状態、プロセスの外との口 | `src/app/main.cpp` / `Application` / `Receiver` / `NetworkController` |
| [src/input/](../src/input/) | アクションに名前を付けている4つの表 | 綴りの残りは ROADMAP の C-2。**割り当ての読み書きは [src/core/inputmap.hpp](../src/core/inputmap.hpp)**（D-034） |
| [src/ui/](../src/ui/) | ウィジェットと描画 | `MainWindow` / `TreeBank` / `TreeBar` / `Theme` |
| [src/view/](../src/view/) | 表示エンジンごとの `View` 実装 | WebEngine / Native / Local |
| [src/gadgets/](../src/gadgets/) | サムネイル一覧（俯瞰 UI） | `QGraphicsScene` 側 |

- **`#include` は全部ファイル名だけの裸書き**で、各ディレクトリがインクルードパスに載っている。
  だからファイルを別の責務へ移しても `#include` は書き換えない
  （[CMakeLists.txt](../CMakeLists.txt) の `target_include_directories`、
  [vanilla.pro](../vanilla.pro) の `INCLUDEPATH`）
- **リポジトリのルートもインクルードパスにある。** `VERSION` を
  `#include <VERSION>` で読んでいるため
- `src/core/` が**増えていく方向**。Phase 7 は「テストを書きたいから切り出す」で
  進めているので、切り出した結果はだいたいここに来る
- テストは [tests/](../tests/)、リソースは [resources/](../resources/) と
  [qrc/](../qrc/)、翻訳は [translations/](../translations/)。**ビルド成果物は
  リポジトリの外**（[BUILD.md](BUILD.md) の「ビルドディレクトリ」）

---

## 実行時のデータの置き場

**根は2つある。** アプリ自身の状態は `Application::BaseDirectory()`、
WebEngine のプロファイルだけは**エンジンが名前から決める場所**（D-099）。

| 何 | どこ | 決めているもの |
|---|---|---|
| アプリの状態 | **`<Base>/data/<MD5(実行ディレクトリ)>/`** | `Application::StateDirectory()`（D-100） |
| 一時ファイル | `<Base>/temp/` | `Application::TemporaryDirectory()`。起動時に空にする |
| **WebEngine のプロファイル** | **`<AppDataLocation>/QtWebEngine/<MD5(実行パス + "/" + id)>`** | エンジン。名前がそのままディレクトリ名（D-099） |
| WebEngine のキャッシュ | `<Base>/data/webenginecache/<同じMD5>` | `setCachePath`。prefs を作り直さないので明示してよい |
| **WebView2 のプロファイル** | **`<Base>/data/edgewebview/EBWebView/WV2Profile_<同じMD5>`** | `EdgeEnvironment::UserDataFolder` ＋ 名前（D-258）。名前の規則は WebEngine と同じ |
| QML ビューのプロファイル | `<Base>/data/webengine/<MD5(実行パス + "/quick:" + id)>` | `setPersistentStoragePath`。QML 側は名前を先に渡せないので旧来のまま（D-071 / D-099） |

`Application::BaseDirectory()` は**実行ファイルの隣**。ただし
`C:/Windows/`・`C:/Program Files/`（UAC の保護下）に置かれているときだけ
`<AppLocalDataLocation>`（Windows なら `%LOCALAPPDATA%/vanilla/`）に逃げる。
**つまりインストール版の `data/` は `%LOCALAPPDATA%` にあり、
持ち運び用の実行ファイルでは実行ファイルの隣にある。**

**その逃げ先は実行ファイルごとに分かれない**（アプリ名だけで決まる）ので、
状態はもう一段下、**実行ディレクトリの MD5 の名前を持つディレクトリ**に置く
（D-100。`LocalServerName()` と同じハッシュ）。エンジンが持つ2つは
`data/` の直下のまま —— そちらの名前にはもう同じハッシュが入っている。

```
<Base>/data/
    <MD5(実行ディレクトリ)>/   <- StateDirectory()。下の表
    webengine/                 <- QML ビューのプロファイル
    webenginecache/            <- WebEngine のキャッシュ
```

`StateDirectory()` の中身:

| ファイル / ディレクトリ | 何 |
|---|---|
| `config.json` | 設定（`Settings`。153 キー） |
| `main_tree.json` / `trash_tree.json` | タブツリーとゴミ箱 |
| `cookie.json` | `QNetworkAccessManager` 側のクッキー（WebEngine のものではない） |
| `icondata.json` | ファビコン |
| `password.json` | **旧版の残置データ。現在は読み書きしない**（D-141。OS-backed vault 実装まで機能無効） |
| `<日時>-*.json` | 上記の起動ごとのバックアップ（`Application::BackUp`） |
| `image/` | ノードのサムネイル |
| `history/` | ノードごとの履歴 |

**この配置換えだけは、移行をアプリに入れていない**（D-100 限りの判断）。
手で動かすか [scripts/split-state-directory.ps1](../scripts/split-state-directory.ps1) を使う。
移す前に新しいビルドを起動すると、まっさらな状態で立ち上がる
（古いファイルは残っているので、移せば戻る）。
**ふだんは移行もアプリの仕事**で、0.2.2 以前の XML を読む口（`LegacyFileName`、
[settingsio.cpp](../src/core/settingsio.cpp)）も、置き場を動かした
D-044 / D-045 / D-099 も、すべてアプリの中に入っている。

**Windows で `AppDataLocation` は Roaming、`AppLocalDataLocation` は Local。**
D-099 でプロファイルがエンジンの既定に戻ったので、**インストール版の状態は
Local（`data/`）と Roaming（プロファイル）に分かれている。**
ローミングプロファイルを使う環境なら、そこは考えどころになる ——
Chromium のプロファイルは SQLite と LevelDB のロック付きファイルなので、
ログオンのたびに運ばれると壊れる（Chrome や Edge が user data を Local に
置くのはこのため）。しかも **QtWebEngine に「ローミング対応」は無い** ——
それは Chrome の機能で、この 6.11.1 のツリーには入っていない
（`RoamingProfileSupportEnabled` は `chrome/` にも `components/` にも1件も無く、
`DIR_ROAMING_USER_DATA` という参照者のいない定数が残るだけ）。
`--user-data-dir` も効かない。逃げ道は
`<AppDataLocation>/vanilla/QtWebEngine` を Local へのジャンクションにするか、
その空間だけ拡張を諦める（`setPersistentStoragePath`。D-098）。
**この開発機にローミングプロファイルは無いので、どちらも未実測。**

**古い配置の残骸**（どれも今のバイナリは読み書きしない。消してよい）:

- `<AppDataLocation>/QtWebEngine/<生の id>` —— D-044 以前。id がそのまま名前だった頃
- `<AppLocalDataLocation>/vanilla/QtWebEngine/` と同 `cache/QtWebEngine/` —— さらに古い版
- `<Base>/data/webengine/<MD5>` のうち `quick:` でないもの —— D-099 で移した跡

**このアプリが作っていないもの**も同じ親の下に出る。
`<AppLocalDataLocation>/vanilla/WebView2/EBWebView/` は
`QuickNativeWebView`（`Qt6::WebView` = Windows では Edge WebView2）の取り分で、
中身はランタイムが管理する。`cache/qmlcache`・`cache/qtpipelinecache-*` は Qt 自身。

---

## 全体像

```
main.cpp
  └─ Application                アプリ全体。設定・永続化・グローバル状態
       └─ MainWindow            ウィンドウ。メニューと周辺ウィジェット
            ├─ TreeBar          ツリー型タブのバー
            ├─ ToolBar          ツールバー
            ·  （NodePreview     タブに乗せたとき出るサムネイル。
            ·                    アプリで1つの独立ウィンドウ。D-028）
            └─ TreeBank         ★中枢。ツリー状態を保持し、View を作る
                 ├─ Node ツリー （ViewNode / LocalNode）
                 ├─ View        表示中のページ（エンジン別の実装）
                 ├─ Gadgets     サムネイル一覧 UI
                 ├─ Receiver    コマンド / URL / 検索の入力欄
                 └─ Notifier    ステータス表示
```

---

## コアとなるクラス

### `Application` — [application.hpp](../src/app/application.hpp) / [application.cpp](../src/app/application.cpp)

`QApplication` の派生。実質的にアプリのグローバル状態をすべて持つ god object。

- 設定の読み書き（[settingsio.hpp](../src/core/settingsio.hpp)。
  型は `QSettings::SettingsMap` = `QMap<QString, QVariant>` だが、**`QSettings` そのものは使っていない**）
- 保存先ディレクトリの決定（`BaseDirectory` / `DataDirectory` / `ThumbnailDirectory` / `HistoryDirectory`）
- 翻訳のロード、ブックマークの入出力、証明書判断、アイコンデータベース
- `BootApplication()` が起動時の初期化を一手に引き受ける

現在書く保存ファイルは `main_tree.json` / `trash_tree.json` / `cookie.json` /
`config.json` / `icondata.json` の5つ。**すべて JSON**（D-011）。旧版の
`password.json` は削除せず残すが、読み書きしない（D-141）。

同名の `.xml`（0.2.2 以前が書いたもの）があれば**読み込みだけする**。
判定は `Application::LegacyFileName()` による拡張子の差し替え。書き出しは JSON のみ。
**この読み込み経路は 0.3.0（2026-08-30 公開）まで残すと決めていたので、次版で削除する（ROADMAP E）。**

#### 設定ファイル（`config.json` / `icondata.json`）の形

キーを入れ子にせず、`"group/subgroup/key"` のフラットなオブジェクトにしてある
（キー名がユーザー由来なので、区切り文字で構造が壊れないようにするため）。

```json
{
    "application/@AutoSaveInterval": 300000,
    "application/@EnableAutoLoad": true,
    "application/@AllowedHosts": [],
    "application/@AllowedCertificates": ["example.com:<leaf SHA-256>"],
    "application/keymap/Ctrl+W": "Close",
    "mainwindow/geometry24387": {"@rect": [341, 192, 1365, 768]},
    "mainwindow/toolbar24387": {"@variant": "AAAADAAAAACA..."}
}
```

- JSON で表せる型はそのまま書く。それ以外は**型名1つだけを持つオブジェクト**にする
  （`@url` / `@size` / `@sizef` / `@point` / `@pointf` / `@rect` / `@rectf` / `@color`）
- どの分類にも入らないもの（`QIcon`、`QByteArray` など）は
  `QDataStream` + Base64 で `{"@variant": "..."}` になる。`icondata.json` はこれだけでできている
- **タグ名を間違えても落ちない。** 値が空の `QVariant` になって、
  その設定が黙って既定値に戻るだけ。だから
  [tests/tst_settingsio.cpp](../tests/tst_settingsio.cpp) がタグ名を1つずつ固定している
- 読み書きは [settingsio.hpp](../src/core/settingsio.hpp) / [settingsio.cpp](../src/core/settingsio.cpp)。
  **`SettingsIO` も `Application` に依存しない**（D-031）。
  旧ファイル名の作り方、バックアップの探し方、バックアップから復帰したときの通知は
  `SettingsIO::Hooks`（`std::function` 4つ）として渡される。
  `Application` 側の実体は `src/app/application.cpp` の `SettingsHooks()`
- **`Load` は「JSON → `.prev` → 同名の XML → バックアップ世代（新しい順）」の順に試す。**
  `Save` は `~name` に書いてから rename するので、書いている途中で落ちても
  既にあるファイルは壊れない。差し替え中は旧版を `.prev` に置き、新版の rename が
  失敗すれば元へ戻す（`FileExchange`、D-144）

#### セッションファイル（`main_tree.json` / `trash_tree.json`）の形

`children` の入れ子だけでできている。フォルダは `children` を持ち、
タブは `holdview: true` で自分自身に URL などを持つ。

```json
{
"children": [{
"primary": false, "holdview": false, "folded": true, "title": "folder",
"create": "...", "lastupdate": "...", "lastaccess": "...",
"children": [{
  "primary": true, "holdview": true, "folded": true, "title": "Google",
  "create": "...", "lastupdate": "...", "lastaccess": "...",
  "index": 1, "url": "https://www.google.com/", "scrollx": 0, "scrolly": 0, "zoom": 1,
  "history": "{uuid}.dat", "thumb": "{uuid}.jpg"
  }]
}]
}
```

行とインデントを節約する形は D-137（1ノード最大4行、波括弧は配列の括弧と
融合、1段2スペース）。**読む側は形に依存しない**ので、1キー1行だった
旧形のファイルもそのまま読める。

- `index` は保存時にそのノードを表示していたウィンドウの id。0 なら「どこにも表示していない」。
  読み込み時、0 以外なら該当ウィンドウを作ってそのノードを current にする
- `history` / `thumb` はゴミ箱のノードには書かない
- 読み書きは [treeserializer.hpp](../src/core/treeserializer.hpp) /
  [treeserializer.cpp](../src/core/treeserializer.cpp) にある。書き出しは `QTextStream` に直接流し、
  読み込みもファイルからノードを直接組み立てる（D-136）——
  どちらの向きも木を丸ごと `QJsonDocument` に持たないため
- **`TreeSerializer` は `ViewNode` 以外に依存しない。** ウィンドウ id の取得、
  ウィンドウの復元、ゴミ箱かどうかの判定、UI を固まらせないための `processEvents` は
  `TreeSerializer::Hooks`（`std::function` 4つ）として渡される。
  `TreeBank` 側の実体は `src/ui/treebank.cpp` の `TreeHooks()`。
  この形にしてあるので、[tests/tst_treeserializer.cpp](../tests/tst_treeserializer.cpp) が
  `Application` も `MainWindow` も無しにセッションファイルを読み書きできる
- 保存時の差し替えは設定と同じ `FileExchange` を通す。本体が読めないときは
  `.prev` を通常バックアップより先に読み、復元したことを通知する（D-144）
- **3-3 以前の XML（`<viewnode>` の下に `<histnode>` がぶら下がる形）は読まない。**
  読み込んでもクラッシュはしないが、URL とサムネイルと履歴が落ちて空のタブになる

#### ブックマークの入出力（セッションとは別物）

「お気に入りのインポート / エクスポート」は
[bookmarkio.hpp](../src/core/bookmarkio.hpp) / [bookmarkio.cpp](../src/core/bookmarkio.cpp)。
**セッションの読み書き（`TreeSerializer`）とは別の経路**で、
形式も D-008 により XML / HTML のまま。

| 形式 | 読む | 書く | 備考 |
|---|---|---|---|
| 内部形式（`<viewnode>` の XML） | ✅ | ✅ | 3-5 以前のセッション形式。`index` / `zoom` / スクロール位置を持つ |
| XBEL | ✅ | ✅ | `alias` / `separator` / `info` / `metadata` は読み飛ばす |
| NETSCAPE HTML | ✅ | ✅ | どのブラウザでも読める形 |
| Chrome の `Bookmarks` | ✅ | — | Opera / Vivaldi も同じ形。複数のルートを1つの `Favorites` フォルダにまとめる |
| Firefox の `bookmarks-*.json` | ✅ | — | ルートが1つなのでそのまま1フォルダになる |
| IE の `Favorites` ディレクトリ | ✅ | — | `.url` / `.website` は `InternetShortcut/URL` を持つ ini |

- **`BookmarkIO` も `ViewNode` 以外に依存しない。** `TreeSerializer` と同じく、
  外から要るのは `BookmarkIO::Hooks`（ウィンドウ id の取得1つだけ）と、
  書き出し時のタイムゾーン（`LocalUtcOffset()` を呼び出し側が渡す）。
  `Application::Import` / `Export` に残っているのは**ダイアログと形式の選択だけ**
- **URL はパーセントエンコードした形で書く**（D-030）。読む側は `toUtf8()` を通すので、
  デコード済みの URL を書くブラウザのファイルも読める
- 固定は [tests/tst_bookmarkio.cpp](../tests/tst_bookmarkio.cpp)

#### `cookie.json` の形

id ごとに、生のクッキー文字列の配列を持つだけ。

```json
{
    "root": ["name=value; expires=...; domain=..."],
    "trash": []
}
```

### `TreeBank` — [treebank.hpp](../src/ui/treebank.hpp) / [treebank.cpp](../src/ui/treebank.cpp)

**このアプリの中枢。** ツリーの状態を持ち、ノードと View の対応を管理する。

- ルートは静的メンバ: `m_ViewRoot` / `m_TrashRoot`
- 現在位置: `m_CurrentViewNode`
- `Back` / `Forward` / `Rewind` / `FastForward` は `View::CanGoBack` /
  `TriggerNativeGoBackAction` などを呼ぶだけで、**どう歩くかはビューが決める**。
  エンジン2実装は native の履歴だけを使い、**`QuickNativeWebView` は自前の URL
  リストを持つ**（D-160。Qt WebView に履歴 API が無いため）
- ノードから View を生成するファクトリを持つ。**既定は `WebEngineView`**、
  ノードの設定文字列（`quickwebengine` など）で他の実装を選べる

### `Node` 系 — [lightnode.hpp](../src/core/lightnode.hpp)

ツリーのデータモデル。[lightnode.hpp](../src/core/lightnode.hpp) に2種類ある。

| クラス | 役割 |
|---|---|
| `ViewNode` | ツリーのノード。`HoldsView()` が真ならタブ、偽ならフォルダ |
| `LocalNode` | ローカルファイル |

`ViewNode` はタブ1つ分のデータをすべて自分で持つ。

| メンバ | 内容 |
|---|---|
| `m_HoldView` | タブかフォルダか。フォルダのときだけ子を持つ |
| `m_Url` / `m_Title` | 表示中の URL とタイトル |
| `m_Image` / `m_ImageFileName` | サムネイル。`ThumbnailDirectory()` に `{uuid}.jpg` として保存 |
| `m_HistoryData` / `m_HistoryFileName` | 戻る/進む履歴。`HistoryDirectory()` に `{uuid}.dat` として保存。中身は**書いたビュー次第**で、エンジン2実装はシリアライズ済みの本物、`QuickNativeWebView` は URL の並び（D-160。**先頭 4 バイトで見分け、互いに読み違えない**） |
| `m_ScrollX` / `m_ScrollY` / `m_Zoom` | スクロール位置とズーム |

画像と履歴は `SaveImageIfNeed()` / `SaveHistoryIfNeed()` で保存時に遅延書き出しされ、
`GetImage()` / `GetHistoryData()` で必要になった時点でファイルから読み戻される。

ツリーの形は [lightnode.hpp](../src/core/lightnode.hpp) の冒頭コメントに図がある。

### `View` — [view/view.hpp](../src/view/view.hpp) / [view/view.cpp](../src/view/view.cpp)

表示エンジンを抽象化するインターフェース。実装は以下。

| 実装 | 有効条件 | 備考 |
|---|---|---|
| `WebEngineView` | `WEBENGINEVIEW` | **既定**。QWebEngineView ベース。WebEngine 系は `src/view/webengine/`（D-249） |
| `QuickWebEngineView` | `WEBENGINEVIEW` | QQuickWidget + QML（[view/quickwebengineview6.qml](../src/view/webengine/quickwebengineview6.qml)）。widgets 層の不具合からの逃げ場（D-061） |
| `LocalView` | `LOCALVIEW` | ローカルファイル閲覧。`GraphicsTableView` と `View` の多重継承。何のファイルかは [view/mediatype.hpp](../src/view/mediatype.hpp) が決める（D-024） |
| `QuickNativeWebView` | `NATIVEWEBVIEW` | QWidget + QQuickView + 窓コンテナ（D-070）。Windows では **Edge WebView2** —— Qt WebEngine 丸ごとの不具合からの唯一の逃げ場 |
| `EdgeWebView` | `EDGEWEBVIEW` | QWidget + **`ICoreWebView2` を自前でホスト**（D-166）。**窓は自分で作って container で置く**（D-168。ウィジェットのネイティブ窓は Qt が作り直すので使えない）。Windows と x64、`third_party/webview2` がある場合のみ。**Qt WebView を通さないのでホスト側の口が使える** —— キー・メニュー・戻り道は C-7 c。**a〜e まで実機で確認済み**（比較表の Edge 列がその結果）。**ページはバックエンドの窓ではなく DirectComposition のビジュアルに描かれ**、マウスはこちらが `SendMouseInput` で渡す（C-7 e、D-190）。合成が作れない機械だけ窓ありに落ちる。**ソースは `src/view/edge/` に責務ごとの 8 つの `.cpp`**、COM を含む私有部は `edgewebview_p.hpp`（D-249。どこに何があるかはそのヘッダ冒頭の一覧）。**backend は controller が bounds を得た時点で見せる**（D-253 で D-250 の cover を外した。1〜2 フレームのために読み込みが遅く見えていた。controller が来る前は HWND の下地塗りが見える） |

`Page` — [view/page.hpp](../src/view/page.hpp) はエンジンに依らないページ共通処理。
`WebEnginePage` がエンジン固有部分を担う。

**描画プロセスが死んだときの復帰は `View` が数える。** 落ちたら報せて読み直すのが
既定の答だが、読むたびに落ちるページではそれが際限のない繰り返しになる。
`view.hpp` の `RenderProcessLedger` が復帰を数え、**10 回目で別の文言を出して
読み直しをやめる**（以後は何も出さない）。数えは `OnLoadFinished(true)` で戻る ——
**失敗した読み込みでは戻らない**。描画プロセスの死はそれが担っていた読み込みを
失敗させるので、そこで戻すと数えないのと同じになるため。意図的な破棄
（`Discarded`）は台帳より前で弾いていて数に入らない。WebEngine / QuickWebEngine /
QuickNative の 3 実装が同じ判定を通る。

#### ウェブ4実装の機能比較（2026-08-26 時点、D-070〜D-248 まで反映。ただしクッキー・ブロック / DNT・証明書・休止・編集系・権限・ミュート・Notifier との共存の各行は D-275 まで）

**`QuickNative` と `Edge` はどちらも Windows では WebView2 に行き着くが、別物。**
前者は Qt WebView 越しで、×の多くは QML API に無いから塞がっている。
後者は `ICoreWebView2` を自前で持つ。この表は現状の印だけを持つ。

○=実機確認済み / △=配線済みだが実機未確認 / ×=不可・未配線 / ―=土台の API が無く原理的に不可能。
`JS` は「注入した JavaScript で実装している」の印。**`※` は条件つき**で、
表のすぐ下の「行ごとの注」に一行ずつある。**セルには印と D 番号だけを置く。**

| 機能 | WebEngine | QuickWebEngine | QuickNative | Edge |
|---|---|---|---|---|
| 描画 | ○ | ○ | ○ (D-070) | ○ (D-166) |
| サムネイル | ○ | ○ | ○ (D-072) | ○ (D-171) |
| 俯瞰(Gadgets)との共存 | ○ | ○ | ○※ (D-076) | ○※ (D-168) |
| Notifier / Receiver との共存 | ○ | ○ | ○※ (D-275) | ○※ (D-177, D-178) |
| プロファイルのスペース分離 | ○ | ○ (D-071) | ― | ○ (D-166) |
| Private (OTR) | ○※ (D-202) | ○※ (D-201) | ― | ○※ (D-166, D-199, D-202, D-220) |
| Cookie 永続 | ○ | ○ (D-071) | ○ | ○ (D-199) |
| セッションクッキーの永続（`@SaveSessionCookie`） | ○ (D-270) | ○ (D-270) | ― | ×※ (D-259) |
| QNAM jar との連携（庫 → jar） | ○※ (D-257) | ○※ (D-270) | ―※ (D-072) | ○※ (D-259, D-260) |
| セッション履歴の保存復元 | ○ | △※ (D-080) | △※ (D-160) | ○※ (D-171, D-188) |
| 戻る/進む/巻き戻し | ○ | ○ (D-074) | ○ (D-160) | ○ (D-171, D-188) |
| スクロール保存復元 | ○ | ○ | ○ | ○ (D-170) |
| ズーム保存復元 | ○ | ○ | ○ JS (D-083) | ○ (D-170) |
| 検索 (SeekText) | ○ | ○ | ○ JS (D-083) | ○ JS (D-191) |
| 編集系 (Copy/Paste/SelectAll…) | ○ | ○ | ○ JS (D-083) | ○ JS (D-191) |
| 編集系・書式（15本） | ○ (D-091) | ○※ (D-272) | △ JS (D-091) | ○ JS※ (D-191, D-199) |
| 証明書エラーの続行 | ○ (D-091, D-140) | ○ (D-272) | ― | ○ (D-194, D-199) |
| 通知の表示 | ○ (D-091) | ○ (D-151, D-152) | ― | ○ (D-194, D-199) |
| 画面共有の共有先選択 | △ (D-091) | △ (D-162) | ― | × (D-196) |
| WebAuthn / パスキー | △ (D-091) | △ (D-162) | ― | × (D-196) |
| 権限（clipboard の読み取り） | ○ (D-091) | ○※ (D-274) | ― | ○※ (D-274) |
| 権限（local fonts） | △ (D-091) | △ | ― | △ (D-194, D-199) |
| 権限の永続化 | ○ (D-091) | ○ (D-274) | ― | ○ (D-194, D-199) |
| 閲覧データの消去 | ○ (D-091) | △※ | × | ○※ (D-195, D-199) |
| リクエストのブロック / DNT | ○※ (D-091, D-199) | ○ (D-271) | ― | ○※ (D-193, D-199) |
| スペルチェック | △ (D-091) | △※ (D-275) | ― | × |
| タブの休止 | ○ (D-091, D-116) | △※ (D-162) | ― | ○※ (D-195, D-199) |
| POST での遷移 | △ (D-091) | × (D-162) | ― | △ (D-193) |
| キャッシュ無視リロード | ○ | ○ | × | ○※ (D-193, D-199) |
| 独自コンテキストメニュー | ○ | ○ | × | ○ (D-169) |
| キーマップ | ○ | ○ | × | ○※ (D-167) |
| マウスジェスチャ | ○ | ○ | × | ○ (D-180, D-190) |
| 入力欄へのドロップ | ○ (A-25, D-164) | ○ | × | ○ (D-185) |
| ページ内キーのイベントフィルタ | ○ | ○ | ― | ○※ (D-167) |
| パスワードマネージャ | 無効 (D-141) | 無効 | ― | 無効 (D-141) |
| 音の検知（タブの印） | ○ | ○ | ― | ○ (D-191, D-199) |
| ミュート | ○※ | ○※ | ― | ○※ (D-191, D-199, D-274) |
| アクセスキー | ○ | ○ | × | ○※ (D-187, D-189) |
| ダウンロード統合 | ○ | ○ | × | △※ (D-172, D-173) |
| ページ設定（ディレクトリ設定） | ○ | ○ | ○※ (D-082) | ○※ (D-192) |
| UA / AcceptLanguage | ○ | ○ | ○※ (D-076) | ○ (D-192, D-193, D-199) |
| `vanilla://` ページ | ○ | ○ (D-073) | ―※ (D-218) | ○ (D-196) |
| ソース表示 (view-source:) | ○ | ○ | ○※ | ○ (D-248) |
| Inspector | ○ (D-064) | ○ (D-086) | × | ○ (D-192, D-245) |
| Inspector をウィンドウ内に | ○ (D-087) | ○ | ― | ○※ (D-245) |
| 動画再生位置 (MEDIATIME) | ○ | ○ (D-081) | ○※ (D-083) | ○ (D-191, D-199) |
| 印刷 | ○ | ○ (D-161) | ×※ | ○ (D-192, D-199) |
| フォント設定（11件） | ○ | × (D-161) | × | ×※ |
| ページからの `window.print()` | ○ (D-091) | △ | ― | ○ (D-192, D-199) |
| 全画面 | ○ | ○ | × | ○ (D-192, D-199) |
| 新窓 | ○ | ○ (D-200) | × | ○ (D-172) |

**行ごとの注**（`※` の付いたセルだけ。印を読み違えないためのもの）:

- **俯瞰(Gadgets)との共存**: QuickNative と Edge は**疑似**（D-076 / D-168）。
  どちらもネイティブ窓で兄弟ウィジェットの上に来るため、重ね順そのものは直らない（Edge は D-175）
- **Notifier / Receiver との共存**: Edge と QuickNative は**独立ウィンドウ**として出す
  （D-177 / D-178、D-275）。どちらも `ForbidToOverlap()` が真で、ページが
  ネイティブ窓だから —— 兄弟ウィジェットの重ね順を無視するので、重ねると
  ページの下に入って見えなくなる。QuickNative の `×` は D-275 まで
  `ForbidToOverlap()` が偽だったからで、**真にすれば済んだ** ——
  窓の一覧で `;edge` と同じ形（Notifier 300x50、`command` で Receiver 576x50、
  どちらも主窓が所有して手前）になることを実測し、ユーザーも確認済み
- **Private (OTR)**: 後からの付け外しへの追随は、widgets と Edge が**ビューの作り直し**（D-202）、
  quick が**その場の差し替え**（D-201）。Edge が空で始まるのは消去の完了を待つから（D-220）
- **セッションクッキーの永続（`@SaveSessionCookie`）**: widgets と quick は
  `ForcePersistentCookies` を受け取り、**期限の無いクッキーが再起動をまたぐことを
  両方で実測した**（D-270。設定 off で両方消えることも見ている）。quick が配線されたのは
  D-270 で、それまで設定は widgets と `cookie.json` にしか届いていなかった。
  **Edge は API そのものが無い** —— `ICoreWebView2Profile`〜`Profile9` に寿命の方針は無く、
  クッキーの口は `ICoreWebView2CookieManager` の 8 本（作る・複製・取る・足す/更新・消す 4 種）
  だけ（SDK 1.0.4129.50 の `WebView2.h` で数えた）。
  庫へ書き戻さない規則（D-259）なので jar 経由の裏道も無い。**Edge のビューでは設定によらず
  session cookie がプロセスと共に消える**ので、そのログインは再起動をまたがない
- **QNAM jar との連携（庫 → jar）**: **繋がっている 3 実装はどれも一方向**で、
  jar は鏡でしかない（D-259 の L1 ＝ アプリは庫へ書き込まない）。
  widgets と quick は `cookieAdded` / `cookieRemoved` を流す（D-257）。
  **どちらも実機で確認済み** —— widgets は D-257（`wemirror;web;id`）、quick は D-270。
  どちらも「頁を読ませてからパイプの `download` を送り、QNAM の要求が
  セッションを運ぶか」を見ている。
  **private はどれも映さない**（D-172 / D-199 / D-220）。Edge が両方向だったのは
  D-172 / D-173 の頃で、押し込みは D-259 で撤去した。QuickNative は**庫を読み返す口が
  無く**（D-072）、残っていた jar → 庫の種まきも D-259 で撤去したので、いまは繋がっていない。
  鏡は**どの庫のものか**を持つ（D-260。1 ビットだった頃は、未ログインの Edge の空の答えが
  WebEngine の生きたセッションを消した）
- **セッション履歴の保存復元**: quick は**保存のみ**（D-080）、
  QuickNative と Edge は **URL だけ**（D-160 / D-171・D-188）
- **編集系・書式（15本）**: Edge は JS 実装で、実機で見たのは **3本**（D-191 / D-199）。
  quick で見たのは **7本**（太字・斜体・下線・打ち消し・箇条書き・中央揃え・字下げ。
  D-272。`contenteditable` の `innerHTML` を頁自身にタイトルへ書かせて読んだ）
- **ソース表示 (view-source:)**: Chromium を積む3つは**バックエンドが描く**
  （行番号・色分け・折り返しは engine のもの）。QuickNative だけが `Page::SetSource` の
  **自前の色付け**で、この版だけ `contenteditable` なので `ApplySource` に意味がある。
  Edge が最後にこちら側へ来た（D-248）
- **権限（clipboard の読み取り）**: **quick と Edge の両方で実機を通した**
  （D-274。頁のボタンを押して 23 文字を読んだ）。**どちらも許可を訊かない** ——
  Edge では `PermissionRequested` が来ず、Permissions API の状態は `prompt` のまま
  読めた。**この `○` は「読み取りが動く」であって「許可の道が通っている」ではない**
  （行を分けたのはそのため。Codex, D-275 レビュー 低2）。**読み込み時には
  訊かせられない** —— ユーザー操作の無い `readText()` はダイアログを出さずに
  `NotAllowedError` で終わるので、頁にボタンを置くこと
- **権限（local fonts）**: **どのバックエンドでも実機を通していない。** 配線は
  権限ハンドラごと共通で（widgets は D-091、Edge は D-194 / D-199 の
  `PermissionName` の表に `LocalFontsAccess` がある）、訊く頁を作っていないだけ。
  **ダイアログを伴う権限として実機で見たのは通知だけ**（Edge は D-194 / D-199、
  quick は D-274）
- **閲覧データの消去**: quick はクッキーと HTTP キャッシュを消せるが、
  **訪問済みリンクだけ消せない** —— `QQuickWebEngineProfile` に 6.11 まで口が無い
  （配線漏れではない）。Edge は **cookie で実測**（D-195 / D-199）
- **リクエストのブロック / DNT**: widgets は D-199 で **DNT の送出も実機**。
  Edge は **iframe の文書を見逃す**（D-193）。**quick は D-271 まで配線が無かった** ——
  インターセプタは widget のプロファイルにしか渡っておらず、`△` は誤りだった。
  同じ起動に並べて実測し（ローカルサーバのヘッダと、規則で拒む画像が飛ぶかどうか）、
  配線してから両方が `DNT: 1` を送り、両方が拒まれた画像を取りに行かないことを見た
- **スペルチェック**: quick は **D-275 まで配線が無かった**（`network/@SpellCheckLanguages`
  は widget のプロファイルにしか渡っていなかった）。配線したが、辞書の `.bdic` が要るので
  実機は通していない —— どちらの `△` も「設定は届く、動きは未確認」の意味
- **タブの休止**: Edge は **`Frozen` 相当のみ**で `Discarded` は無い（D-195 / D-199）。
  **quick の `△` はパイプでは埋められない**（D-272）—— 休止を起こすのは
  `View::hide()` で、俯瞰（`viewtree`）はネイティブ窓を持つ実装（Edge / nwv）の窓しか
  隠さない。widgets と quick の上には俯瞰が**重なるだけ**なので、頁は動き続ける
  （実測。widgets を対照に置いて両方が動き続けることを見た）。埋めるには
  TreeBar のタブを実際に切り替える必要がある
- **キャッシュ無視リロード**: Edge は**ヘッダのみ・次の1回**（D-193 / D-199）
- **キーマップ**: Edge は `CanCompleteAction` の**許可リスト制**（D-167）
- **ページ内キーのイベントフィルタ**: Edge は**トップ文書だけ**（D-167）
- **アクセスキー**: Edge は窓の上に overlay を重ねるのではなく、
  **ページを Grab して俯瞰の背景に敷いて描く**（D-187 / D-189）。窓の性質に阻まれないので、
  D-196 時点の分類「窓の性質」は解消済み
- **ミュート**: widgets と quick の `○` は**コンテキストメニュー経由**を指す ——
  `WebEngineView::ToggleMediaMute` は直前のメニュー要求が無いと何もしない。
  **Edge は実測済み** —— パイプの `key M` でオーディオセッションのピークが
  `0.3052` → `0.0000` になり、もう一度で戻る（D-277）。タブバーからも効く
  （ユーザー確認、D-274）。D-273 の「キー経由では効かない」は**計測の誤り**で、
  窓へ送ったキーは `TreeBank` の `application/keymap` を引いていた
- **ダウンロード統合**: Edge は**一覧が無い**（D-172 / D-173）
- **ページ設定（ディレクトリ設定）**: QuickNative は**一部**（D-082）、
  Edge は **JS とエラーページだけ**（D-192）
- **UA / AcceptLanguage**: QuickNative は **UA だけ**（D-076）
- **`vanilla://` ページ**: QuickNative では開けず、**代わりに WebEngine が開く**（D-218）
- **Inspector をウィンドウ内に**: Edge は**別プロセスの窓を取り込む**
  （D-245。可否は3実装共通の設定 `webview/@InspectorInMainWindow`＝既定オン。D-247）
- **動画再生位置 (MEDIATIME)**: QuickNative は**ポーリング**（D-083）
- **印刷**: QuickNative は印刷ダイアログを持たず、**PDF 書き出しだけ**（`printToPdf`）
- **フォント設定（11件）**: Edge の × は API ではなく、**設定そのものが無い**

補足（読み違えやすいところだけ）:

- **Edge 列の × は「API が無い」**で、どれも「これから実装する」ではない
  （D-196。SDK 1.0.4129.50 の `WebView2.h` で数えた）。**例外はフォント設定**で、
  こちらは API ではなく設定そのものが無い（上の注）
- Edge 列に残る △ は **POST での遷移・ダウンロード一覧・権限（local fonts）** の3つ
  （ミュートと clipboard は D-274 で埋めた）。
  何を確かめるのに何が要るかは HANDOVER.md の表
- QuickWebEngine の△は「実機を通していない」の意だが、**それが「配線はある」を
  意味するとは限らない** —— D-271（ブロック / DNT）と D-275（スペルチェックほか3つ）は
  `△` と書いてあって配線が無かった。**`△` を見たら、まず誰が読むかを確かめること。**
  埋める作業（C-3b / C-3c）は D-200 / D-201 で打ち止めにした —— 残る×は
  **POST 遷移 (D-162)・フォント設定 (D-161)・セッション履歴の復元 (D-080)・
  widgets とのプロファイル共有 (D-071)** で、いずれも Qt に API が無い。
  別枠の既知として、Qt を経由しないエンジン自身のドラッグが始まることがある
- QuickNative の×の多くは Qt WebView の QML API が極端に小さいため。
  `runJavaScript` はあるので **JS で書けるものは動く**。
  **ページの上の入力は、API が生えても届かない**（ネイティブ窓が先に取る。
  入力ハンドラ群は死んでいる —— D-164 / D-165、
  [quicknativewebview.cpp](../src/view/quicknativewebview.cpp) の `keyPressEvent` 手前の注記）。
  ネイティブ窓ゆえに**兄弟ウィジェット（俯瞰・Receiver 等）より常に上**に重なり、
  **クッキーの保存領域はアプリで1つ**（スペース分離なし）
- **拡張は D-099**（widgets のプロファイルのみ。QML ビューとプライベートには
  効かない。ROADMAP A-8）

### `Gadgets` / `GraphicsTableView` — [src/gadgets/](../src/gadgets/)

サムネイルのテーブルとしてツリーを俯瞰・編集する UI。`QGraphicsScene` 上に構築される。

- `GraphicsTableView` が一覧の本体（`DisplayType` で ViewTree / TrashTree / LocalFolderTree などを切り替える）
- `AbstractNodeItem` / `Thumbnail` / `NodeTitle` が個々の項目
- `GadgetsStyle` が描画。`GlassStyle`（暗い半透明。既定）と `FlatStyle`（明るい）の2実装があり、
  どちらも色は [theme.hpp](../src/ui/theme.hpp) から引く

### 入力とアクション

| ファイル | 役割 |
|---|---|
| [actionmapper.hpp](../src/input/actionmapper.hpp) | アクション名の一覧をマクロで定義する |
| [keymap.hpp](../src/input/keymap.hpp) | キー → アクション名 の既定割り当て |
| [mousemap.hpp](../src/input/mousemap.hpp) | マウスジェスチャ → アクション名 の既定割り当て。**`Ctrl+WheelUp` のような名前を組み立てる側は `Application::WheelWentUp`**（向きは `angleDelta`。`pixelDelta` は普通のマウスでは空。D-123） |
| [commandmap.hpp](../src/core/commandmap.hpp) | コマンドの綴り → アクション名。`back` / `backward` のような綴りの揺れはここにしかない（D-054） |
| [receiver.hpp](../src/app/receiver.hpp) | 入力欄。Command / Query / UrlEdit / Search の4モードを持つ。`ReceiveCommand` は `CommandMap` に綴りを引かせ、返ってきたアクション名の signal を出すだけ |
| [jsobject.hpp](../src/input/jsobject.hpp) | ページ内 JavaScript から叩ける API |

> アクションの定義がこの5箇所に分散しているのが、現在の設計上の主な負債。
> （綴りの表を切り出したので `Receiver` は減ったが、**名前が3系統ある**のは変わっていない）
> 残っている作業は ROADMAP.md の C-2（JS API の綴りが手動同期）。

### その他

| クラス | 役割 |
|---|---|
| `NetworkController` — [networkcontroller.hpp](../src/app/networkcontroller.hpp) | `NetworkAccessManager` の生成、Cookie、プロキシ、認証 |
| `Saver` — [saver.hpp](../src/app/saver.hpp) | 自動保存。`m_IsSaving` で多重実行を防ぐだけの薄いクラス |
| `Transmitter` — [transmitter.hpp](../src/app/transmitter.hpp) | 多重起動時に、既存プロセスへ引数を渡して自分は終了する（`QLocalSocket`） |
| `UserAgent` — [useragent.hpp](../src/core/useragent.hpp) | 名乗るブラウザの表。綴り・正名・既定テンプレートが 15 行で、設定の読み書きもここ（D-058）。`%SYSTEM%` を埋めるのは `NetworkAccessManager::SetUserAgent` |
| `DownloadName` — [downloadname.hpp](../src/core/downloadname.hpp) | ダウンロード先のファイル名を作る規則。1成分に落とし、書ける綴りにし、宣言された MIME に合う拡張子を足す（D-264）。自前のダウンローダは `Suggest` を通し、エンジンが作った名前は Chromium 側が同じことを済ませているので `Sanitize` だけ。`Unique` は名前ではなく **path** を受け、埋まっていれば `_1`, `_2` を拡張子の前へ入れる |
| `WindowLedger` — [windowledger.hpp](../src/core/windowledger.hpp) | どのウィンドウがあって、今どれかを持つ表。ウィンドウ型でテンプレート化してあり、`Application` の `NewWindow` / `SwitchWindow` / `RemoveWindow` / `GetMainWindows` はこれを呼ぶだけ（D-056） |
| `Notifier` — [notifier.hpp](../src/ui/notifier.hpp) | ステータス・進捗の表示 |
| `Dialog` — [dialog.hpp](../src/ui/dialog.hpp) | 自前描画のモーダル / モードレスダイアログ |
| `MiniMap` — [minimap.hpp](../src/ui/minimap.hpp) | ページの脇の帯。絵は写真ではなく**注入 JS が集めた矩形の図式**（D-209）。widgets / quick / Edge の3実装に対応し（D-210）、クリックでジャンプ・ドラッグでつまみ・ホイールはビューのホイール経路へ渡す（D-238）。再収集はロード・`Grown`・スクロール静止後（D-237）。設定 `application/@EnableMiniMap` は**既定オフ**で要再起動、色は Theme の `MiniMap*` ロール5つ |

`Receiver` のローカルサーバは同一ユーザーだけ接続できる。受信は `readyRead` 駆動で、
`CommandFrame` が完全な `QDataStream` QString を待ってから消費する。上限は1 MiBで、
不正・超過フレームは接続を閉じる（D-145）。

---

## 設定

設定は `group/subgroup/key` → `QVariant` の平坦な地図
（[application.hpp](../src/app/application.hpp) の `Settings`）で、`config.json` に書かれる。
読むのは `s.value("application/@EnableAutoSave", true)` の形で、**153 キーある**。

### 設定画面

`vanilla://settings` という**ページ**（D-020）。タブとして開く。

| 何 | どこ |
|---|---|
| スキームハンドラ | [settingspage.hpp](../src/app/settingspage.hpp)。ページと JSON API を同じスキームで配信 |
| 何を描くかの表 | [settingsschema.hpp](../src/core/settingsschema.hpp)。型・ラベル・分類・既定値・選択肢 |
| ページ本体 | [resources/settings/](../resources/settings/) |

**設定を1つ画面に出すには、[settingsschema.cpp](../src/core/settingsschema.cpp) の表に1行足すだけ。**
ページは表から自分を組み立てるので、HTML も JS も触らなくてよい。

- **既定値は、そのキーを読むコードが `s.value` に渡すものと同じにすること。**
  [tests/tst_settingsschema.cpp](../tests/tst_settingsschema.cpp) がソースを読んで照合する。
- **書けるのは表に載っているキーだけ**で、型と選択肢も検査される。
  スキームは `CorsEnabled` 無しで登録してあるので、よそのページからは API に届かない。
- `QWebChannel` は使っていない（`USE_WEBCHANNEL` は無効のまま）。

開き方は3通り。`settings` コマンド、メニューの「設定」、
そして**アドレスバーに `vanilla://settings` と打つ**。
最後のものについては下の「アドレスバーに打った文字」を参照。

ディレクトリが QuickNative を名指ししていても、`vanilla://` の2ページだけは
`WebEngineView` が描く（`TreeBank::NeedsEngineForVanillaPage`。D-218。
Qt WebView にはリクエスト横取りの口が無く、ハンドラに届かないため）。

#### 画面の文字列

**この画面に英語を直書きしないこと。** ページは Chromium の中にいるので
`QTranslator` に触れず、lupdate は `.js` を読まない。表示するものは
**すべて訳された状態で届く**必要がある。

| 何 | どこ |
|---|---|
| 設定のラベルと説明 | 表の中。`QT_TRANSLATE_NOOP("SettingsSchema", ...)` |
| ページ自身の言葉（題・検索欄・ボタン・トースト） | `SettingsSchema::PageStrings()`。`/api/schema` の `strings` として渡る |

**`QT_TRANSLATE_NOOP` も `translate` も、マクロで短くしないこと。**
**lupdate はマクロを展開しない。** 以前この表は `T(...)` で短縮していて、
lupdate が拾う文字列が 101 個ではなく **7 個**になっていた。
拾われなかった文字列はエラーにならないので、黙って英語のままになる。

### ディレクトリ固有の設定（`vanilla://directory`）

ディレクトリノードの名前は `名前;トークン;トークン` で、`;` の後ろが
**そのディレクトリ以下に効く設定**になる（`ID` / `Private` / `!Javascript` /
`Encoding EUC-JP` など。読む側は [treebank.cpp](../src/ui/treebank.cpp) の
`GetNetworkSpaceId` / `GetNodeSettings`、
[view/view.cpp](../src/view/view.cpp) の `ApplySpecificSettings`、
[networkcontroller.cpp](../src/app/networkcontroller.cpp)）。
**`;` の後ろは画面には出ない** —— タブもサムネイルもツリーも
[lightnode.cpp](../src/core/lightnode.cpp) の `Node::ReadableTitle` が切った名前だけを見せ、
リネームの入力欄も D-120 から同じになった。

**`noload` だけはトークンが無いときも効いている**（D-078）。自動読み込みは
ディレクトリ単位のオプトインで、**トークンを否定した `!noload` を書いた
ディレクトリだけが起動時に裏で読み込まれる**（表示中のタブは従来どおり読み込む。
動機は外部サーバへの負荷）。**`webview/@SuspendHiddenViews` が `Active` でない間は
その否定も効かない**（D-116。休ませるつもりのものを裏で取りに行かない。
その値は D-121 まで `Off` という綴りだった）。

**`ID` の行だけは、上の答えを引き継がない**（D-122）。`ID` は下へ流れる設定ではなく
**「ここがネットワークプロファイルの境目だ」という印**で、`GetNetworkSpaceId` は
一番近いものを見る —— つまり**中にもう1つ境目を作れる**。`Token::ownOnly` がその印で、
`InheritedState` はこの行について -1 を返す（既定の `root;id` のせいで、
それまでは配下のどこでもチェックが灰色で入ったままだった）。
いま効いているプロファイルは名前の横に出る。

**この1行だけは、コントロールの向きがトークンと逆**（D-121）。行の名前は
「オートロード」で、トークンは `NoAutoLoad` —— **語があることが「無効」**になる。
向きを持っているのは `Token::inverted`、語が1つも無いときの意味は
`Token::absence`（この行だけ `0` ＝無効）。ひっくり返るのは
**画面に出す/画面から書く側だけ**で、`StateIn` を呼ぶ読み側は今までどおり
トークンについて答える。**`Active` でない間はこの行を触らせない**
（`DirectoryPage::TokenBlocked`。ページのセレクトもメニューのチェックも
`disabled` になる —— 何も起きない語を書かせるより良い）。

**継承は設定ごと**（D-117）。上のディレクトリ全部に発言権があり、
**ある設定については、それについて何か言っている一番近いディレクトリが決める** ——
言っていない設定はさらに上から1つずつ降りてくる。どのトークンが同じことを
言っているかは `DirectoryPage::TokenSubject`、組み立ては `InheritTokens`
（どちらも純関数で `tst_directorypage` が固定している）。
**規則は2つ、効く範囲が違う**:

| どこ | どちらが勝つか |
|---|---|
| ディレクトリの間 | **近いほう**（`InheritTokens` が遠いほうの語を落とす） |
| 同じ名前の中の `X;!X` | **否定**（従来どおり） |

**ただし、上で変えた設定は下まで行き渡る**（D-118）。編集のあと
`LetDescendantsFollow` が下を辿り、**その変更を止めてしまう語だけ**を題名から外す
（`ChangedWords` / `TitleFollowing`）。何も外すものが無いディレクトリは
**リネームもしない**。だから「近いほうが勝つ」が効くのは、**上を変えたあとに
下で改めて別の答えを書いたとき**になる。

読む側はどちらも `DirectoryPage::StateIn` で訊く（`1` / `0` / `-1`）。
`View::ApplySpecificSettings` の属性表・スーパードラッグ・`AutoLoadWithLink` の
`noload` が同じ1本を通るので、**「否定が勝つ」の写しが散らばらない**。

この編集は [directorypage.hpp](../src/app/directorypage.hpp) に一元化してある（D-047）。
**設定を書く出口は2つ、実装は1つ。**

| 出口 | 何 |
|---|---|
| `vanilla://directory` | **開いたノードの先祖ディレクトリの連なり**を見せるページ（設定は親子で継承されるので、そのノードに効くものだけが並ぶ）。対象は**そのページのタブが居る場所**で、URL は何も名指ししない（D-048）。`GET /api/tree`、`POST /api/set {node, name, tokens}` |
| ノードのコンテキストメニュー | TreeBar とツリー俯瞰の「DirectorySettings」サブメニュー（ID などのチェックボックス）。「ページを開く」はそのディレクトリについて開く |

**リネームはもう出口ではない**（D-120）。`TreeBank::RenameNode` はディレクトリに対して
`TitleName` だけを見せ、返ってきた名前に**元のトークンを付け直す**。だから
**名前は名前でしかなく**、`;` を打つと「無効なディレクトリ名」で断る。

- ページは `vanilla://settings` と同じハンドラ・同じロック（`CorsEnabled` 無し + initiator 検査）。
  ページ本体は [resources/directory/](../resources/directory/)
- **初期表示はこのタブが居るディレクトリ1枚だけ**（D-122）。その上は
  `<details>`「上位ディレクトリ (N)」に畳んであり、開くと元の順（ルートが先頭）に戻る
- **編集は必ず「リネーム」**: `SetTitle` → `TreeBank::ReconfigureDirectory(before, after)`。
  `RenameNode` と同じ経路なので、NAM の Copy / Move / Kill / Merge がそのまま働く
- タイトルの分解・合成・トークンの読み書きは純関数で、
  [tests/tst_directorypage.cpp](../tests/tst_directorypage.cpp) が固定している
- **ルートも編集できる。** リネームは `root` ネットワークスペースの移動になり、
  `ReconfigureDirectory` が `m_RootName`（= `application/@RootName` 設定）も
  書き換えるので次回起動にも残る。編集できないのはゴミ箱だけ
  （タイトルが起動のたびに `Initialize` で固定されるため。名前の `noload` は
  D-078 で既定になったので今は冗長だが、残してある）

---

## アドレスバーに打った文字

打った文字が **URL なのか検索語なのか**を決めるのは
[view/page.cpp](../src/view/page.cpp) の `ExtractUrlsFromText`。
URL に見えるものが1つも取れなかったときだけ `DirtyStringToUrls` が検索 URL を作る。

**スキームを1つ足し忘れると、そのアドレスは黙って検索される。**
エラーにならないので気付きにくい。[tests/tst_inputmapping.cpp](../tests/tst_inputmapping.cpp)
の `tellsAUrlFromSomethingToSearchFor` が、どちらに転ぶべきかを列挙して固定してある。

**もう1つ、[toolbar.cpp](../src/ui/toolbar.cpp) の `IsUrlToLoadInPlace` がある。**
こちらは短く、`http` / `https` / `javascript:` / `about:` / `chrome:` /
`vanilla:` だけ。**役割が違う**。

| | 何を決めるか |
|---|---|
| `ExtractUrlsFromText`（長い方） | URL か検索語か |
| `IsUrlToLoadInPlace`（短い方） | **今のタブで開くか、新しいタブで開くか** |

アドレスバーで Enter を押したとき、短い方に載っていれば
`View::Load()` で**そのタブが移動する**。載っていなければテキストは `Receiver` に渡り、
コマンドとして解釈された末に `Page::OpenInNew` に届く — つまり**新しいタブが開く**し、
`Receiver` を切っていると**どこにも行かない**。

`chrome://` は長い方にだけ載っていたので、開きはするが毎回新しいタブになっていた。
**スキームを足すときは2箇所とも見ること。**
スキーム名は [const.hpp](../src/core/const.hpp) の `VANILLA_SCHEME` にある。

---

## ユーザーエージェント

既定では **Qt WebEngine 自身の UA** を送る。別のブラウザのふりをするのは
`application/UserAgent_*` を選んだときだけで、選び方は
**フォルダのタイトルに `;useragent chrome` のように書く**
（[treebank.cpp](../src/ui/treebank.cpp) の `GetNodeSettings`。ノードから親へ遡って
**ユーザーエージェントについて何か言っている最初のディレクトリ**が効く。D-117）。

文字列は [const.hpp](../src/core/const.hpp) にあり、
[networkcontroller.cpp](../src/app/networkcontroller.cpp) の
`NetworkAccessManager::SetUserAgent` が3つのプレースホルダを埋める。

| | 何になるか |
|---|---|
| `%SYSTEM%` | プラットフォーム（`Windows NT 10.0; Win64; x64` など） |
| `%LOCATION%` | ロケール |
| `%CHROMIUM%` | **このビルドが実際に積んでいる Chromium のメジャー版** + `.0.0.0` |

**クライアントヒントの方には丸めない版を入れる**（`Sec-CH-UA-Full-Version-List`
は実ビルドを読みに来る場所なので。D-102）。どちらも数字は
`qWebEngineChromiumVersion()` から取り、**直に書かない** ——
`chrome://qt` が言う版と食い違わせないため。

**Chromium 系の文字列にバージョン番号を直書きしないこと。**
以前は書いてあり、エンジンが 140 になっても `Chrome/78` と名乗り続けていた。
実際に無い版を名乗る UA は、見破られるうえに
「このページはどの機能を使ってよいか」について嘘をつくことになる。

**名乗ると通らなくなる相手がいる。** Google は「Chrome だと名乗っているのに
Chrome ではないブラウザ」へのログインを拒み、**エンジン自身の UA なら通す**
（D-101。`sec-ch-ua` のブランドが1つしかない・`x-client-data` が無い、で分かる）。
偽装は「名前を要求してくるサイトにだけ」使うもので、常用するものではない ——
ディレクトリ設定のページ（`vanilla://directory/`）にそう書いてある。

---

## Chromium のスイッチ

`chrome://flags` はこのビルドに**無い**。Chrome のブラウザ層の機能で、
Qt WebEngine が組み込んでいるのは content 層だけだから、フラグで有効化できる
類のものではない（実測は HANDOVER.md）。

代わりに `application/@ChromiumFlags`（設定画面の「ページ」）がある。
空白区切りで、それぞれ `--` で始まる語だけが通る。
`Application::ApplyChromiumFlags()` がそれを環境変数
`QTWEBENGINE_CHROMIUM_FLAGS` に入れ、Qt がエンジン起動時に読む。

- **`src/app/main.cpp` で `config.json` を読む必要は無かった。** Qt がこの変数を読むのは
  **最初のプロファイルを作るとき**なので、`BootApplication` が設定を読んだ後、
  `NetworkController` を作る前に置けば間に合う。
  `QApplication` より前に読む必要は無い
- **反映は次の起動から**（`needsRestart`）
- 元から環境変数が設定されていれば**その後ろに足す**。外から渡した人の意図を消さない
- **エンジンが受け付けないスイッチを書くと起動しなくなりうる。**
  戻す手段は `data/config.json` を手で直すこと
- **リモートデバッグは通常起動では開かない。** CDP が必要な検証時だけ、起動前に
  `QTWEBENGINE_REMOTE_DEBUGGING=<port>` を外から設定する（D-142）。アプリ内 Inspector は
  専用 API を使うので、この環境変数を必要としない

---

## 外部コマンド（「別のブラウザで開く」）

**個別ブラウザのハードコードは無い**（D-004）。あるのは2つだけ。

| 何 | どこ |
|---|---|
| 既定のブラウザで開く | `Application::OpenUrlWithDefaultBrowser()`。中身は `QDesktopServices::openUrl()` |
| ユーザー定義コマンド | `Application::ExternalCommands()` / `RunExternalCommand()` |

ユーザー定義コマンドは設定の **1 キーに全部入っている**。
`application/@ExternalCommands` は `QStringList` で、1行が `name = コマンドライン`。
URL は `%u` の位置に入り、`%u` が無ければ末尾に足される。
コマンドラインは `QProcess::splitCommand()` で分割するので、
空白を含むパスは引用符で囲めばよい。

**アクションは既定のブラウザの分だけ**（`OpenWithDefault` / `OpenLinkWithDefault` /
`OpenImageWithDefault` / `OpenMediaWithDefault` / `OpenNodeWithDefault`）。
**ユーザー定義コマンドはアクションではない**（D-021）。個数が可変で enum に入らないうえ、
アクション表は列挙子1つにつき `QAction` を1つしか持たないので、
同じメニューに複数並べられない。代わりにメニューを組むところで `QAction` を直接作り、
コマンド名を `EXTERNAL_COMMAND_PROPERTY`（[const.hpp](../src/core/const.hpp)）に載せて
1つのスロットへ流している。`data()` は要素（`SharedWebElement`）で埋まっているので使えない。

メニューを組んでいるのは4箇所。**コマンドを増やす経路を足すならこの形を真似ること。**

| どこ | 何のメニュー |
|---|---|
| [view/view.cpp](../src/view/view.cpp) `AddExternalCommandActions` | ページ / リンク / 画像 / メディア |
| [gadgets/gadgets.cpp](../src/gadgets/gadgets.cpp) `CreateNodeMenu` | ツリー俯瞰のノード |
| [treebar.cpp](../src/ui/treebar.cpp) `NodeItem` のメニュー | タブバーのノード |

コマンド行からは `openwithdefault` / `openwith <name>` /
`opennodewithdefault` / `opennodewith <name>` /
`openlinkwithdefault` / `openimagewithdefault` / `openmediawithdefault`。
**リンク / 画像 / メディアの名前付きは無い** — この3つは
`Receiver::TriggerElementAction(Page::CustomAction)` 1本で `Page` に届くので、
名前を一緒に運べない。メニューからは使える。

---

## 色

描画に使う色は **[theme.cpp](../src/ui/theme.cpp) の1つの表にしかない**（D-017）。
呼び出し側は役割名で引く。

```cpp
painter->setBrush(Theme::Brush(Theme::BarBackground));
painter->setPen(Theme::Pen(Theme::GlassText));
// グラデーションの端や、アクティブ / 非アクティブの対にはアルファを渡す。
painter->setBrush(Theme::Brush(Theme::TitleBarBackground, isCurrent ? 200 : 128));
```

- **他のどこにも色リテラルを書かないこと。** 例外は
  [application.cpp](../src/app/application.cpp) の設定値 → `QColor` 変換だけ。
- **役割名は色ではなく用途**（`BarButtonPressed`。`Gray150` ではない）。
  接頭辞は `Bar*` / `TitleBar*` / `Glass*` / `Flat*` / `Gadgets*` /
  `Notifier*` / `Receiver*` / `Dialog*`。
- **関数ローカルの `static const QBrush` に入れないこと。** 最初の1回で色が焼き付き、
  実行中にパレットを差し替えられなくなる。
- 役割を足したら **Light と Dark の両方の表**と
  [tests/tst_theme.cpp](../tests/tst_theme.cpp) に足す。
  1つでも忘れると `static_assert` か `RoleCount` の比較で落ちる。

### 配色（Light / Dark）

パレットは2本ある。`application/@ColorScheme` が `Auto`（既定）/ `Light` / `Dark`。
`Auto` はデスクトップに追従し、実行中に変わっても拾う。決定は D-018 / D-019 / D-113。
**読む側は大文字小文字を見ない**（D-113 より前のファイルは全部小文字）。

- **スタイル（`GlassStyle` / `FlatStyle`）とは独立**。スタイルは形、配色は色。
- **色を「保持する」ものは push で更新する。** 毎フレーム引き直さないのは6つ —
  TreeBar のシーン背景、`LayerItem` の区切り線、TitleBar のボタン、`QAction` のアイコン、
  **`LineEdit` のスタイルシート**（コマンドラインとアドレスバーの入力欄）、
  **`QWebEnginePage` の背景色**。
  `Application::UpdateAllWidgets()` がここに配る。**新しく保持するものを作ったらここに足すこと。**
- **Qt 自身が描くもの**（メニュー、コンプリータのポップアップ、設定ダイアログの中身）は
  パレットではなく `QStyleHints::setColorScheme()` に従う。`Auto` のときはヒントを触らない。
- **スタイルシートに色を書くときは `#RRGGBB` にすること。**
  Qt のスタイルシートは解釈できない色の宣言を**黙って捨てる**ので、
  結果（何も塗られない）が「テーマが当たっていない」と見分けが付かない。
  `#AARRGGBB` も `rgba(r,g,b,a)` も、この経路では効かないのを実測した。
- **ページの背景**（`Theme::PageBackground`）は、**文書が何も塗らないところにだけ**出る。
  背景を持つ普通のページは今まで通りで、`about:blank` だけがこの色になる
  （背景指定の無い `data:text/html,...` を読ませて、白のままであることを確かめた）。

### アイコン

`resources/` のビットマップは**全部「黒インク」**で、明るい面のために描かれている。
暗い面に置くと消えるので、このアプリが自分で描く面に載るものは
`Theme::Pixmap(path, role)` を通す（Light では素通し、Dark では塗り直す）。

```cpp
painter->drawPixmap(rect, Theme::Pixmap(QStringLiteral(":/resources/treebar/plus.png"),
                                        Theme::BarIcon), source);
```

**明るいチップの上に載るものは通さないこと。** Gadgets のボタンと Notifier のキャンセルは
両配色ともチップが明るいので、塗り直すと逆に消える。
`blank.png` / `blankw.png` のように明暗2セットあるものは
「その面が暗いか」で選ぶ（`Theme::IsDark()`）。

**各ビットマップには `<名前>@2x.png`（ピクセル2倍）が隣にある。**
表示スケールが 100% を超える環境（D-039 で論理 DPI に現れる）では、
`Theme::Pixmap` が @2x を読んで `ScaleByDevice` 相当のサイズへ**縮小**して返す。
拡大は輪郭が滲むのでしない — @2x が無いビットマップは等倍のまま返る。
新しいアイコンを足すときは @2x も描いて qrc に登録すること。

**線画のアイコンは [scripts/draw_icons.py](../scripts/draw_icons.py) が生成する**
（D-131。等倍と @2x を同じ座標から描くので、直すときは PNG ではなく
スクリプトを直して再生成すること）。スクリプトが触らないのは、
木のグリフ4種とその白変種（意図して太いまま）、`table.png`
（vanilla のロゴそのもので、唯一塗りが残る）、`empty.png`。

---

## 重なり順（z-order）

2つの系統があり、**どちらも [const.hpp](../src/core/const.hpp) に定数がある**（D-092）。

1. **`QGraphicsScene` 上の z 値** — [const.hpp:136-146](../src/core/const.hpp#L136-L146)。
   `HIDDEN_CONTENTS_LAYER` (-10) → `VIEW_CONTENTS_LAYER` (0) → `COVERING_VIEW_CONTENTS_LAYER` (5)
   → `MAIN_CONTENTS_LAYER` (10) → `BUTTON_LAYER` (15) → `SPOT_LIGHT_LAYER` (20)
   → `DRAGGING_CONTENTS_LAYER` (30) → `BUTTON_ON_DRAGGING_LAYER` (35) → `SELECT_RECT_LAYER` (40)
   → `IN_PLACE_NOTIFIER_LAYER` (50) → `MULTIMEDIA_LAYER` (60)。
   俯瞰（Gadgets）と FileView（LocalView）の中身はこちら。

2. **`QWidget` の重なり** — [const.hpp:148-171](../src/core/const.hpp#L148-L171)。
   ウィジェットに z 値は無く、順序は**親の子リストの並びだけ**で、
   `raise()` は「その末尾へ動かす」でしかない。だから3枚以上の重なりは
   「正しい順に raise を並べる」以外に書きようがなく、**別の場所に書かれた
   raise が黙って割り込む**。重なるのは `TreeBank` の中だけなので、
   序列は**[`TreeBank::RestackChildWidgets`](../src/ui/treebank.cpp) の1箇所**が持つ。

   | 定数 | 誰 |
   |---|---|
   | `SCENE_WIDGET_LAYER` (0) | グラフィクスビュー（見せる物が無いとき。ページの下） |
   | `VIEW_WIDGET_LAYER` (10) | ウェブビュー（カレントが最後に上がる） |
   | `COVERING_SCENE_WIDGET_LAYER` (20) | 同じグラフィクスビュー（俯瞰または FileView を載せているとき） |
   | `NOTIFIER_WIDGET_LAYER` (30) | Notifier |
   | `RECEIVER_WIDGET_LAYER` (40) | Receiver（コマンドライン） |

   グラフィクスビューが2つの層を持つのは、**中身が空でも透明なまま全面を覆う**から。
   上に置いたままだとページからマウスを奪う。

**この2つに載らない `raise()` / `lower()` は残っているが、どれも別の話**:

- **窓の順序**（枠ウィジェット・タイトルバー・ダイアログ枠・切り離した Inspector・
  `PurgeView` 時のビュー）。ウィンドウマネージャの領分で、序列の定数とは無関係
- **ウィジェットが自分の子を並べる**（Receiver の中の `LineEdit`、TreeBar の `ResizeGrip`）。
  他所に見せる必要が無い
- **`View::lower()`** = 「このビューはもうカレントではない」。
  `VIEW_WIDGET_LAYER` の中の話なので序列は動かない

**新しくウィジェットを重ねるときは、まず 1 の系統に乗せられないか検討すること。**
どうしてもウィジェットで重ねるなら、`const.hpp` に層を足して
`RestackChildWidgets` に1行足す。**そこ以外に `raise()` を書かないこと。**

---

## ビルド時スイッチ

[switch.hpp](../src/core/switch.hpp) で機能を切り替える。現在有効なもの:

| マクロ | 状態 | 意味 |
|---|---|---|
| `USE_WEBCHANNEL` | 無効 | QWebChannel によるページ連携 |

> `PASSWORD_MANAGER`（D-141 で無効化した旧 XOR パスワード保存）、`USE_ANGLE`、
> `DEBUG_MODE` 系5つは 2026-08-30 にガード内のコードごと削除した。
> パスワード保存を再び持つなら OS の資格情報保管庫から新規に書く。
> `FAST_SAVER` は削除した。有効側しかコンパイルされておらず、
> 無効側の書き出しコードは誰も通っていなかったため（Phase 5）。

ビュー実装のマクロ（`WEBENGINEVIEW` / `LOCALVIEW` / `NATIVEWEBVIEW`）は
CMake が `find_package` の結果に応じて `vanilla_core` に **PUBLIC で**付ける
（[vanilla.pro](../vanilla.pro) は `qtHaveModule()` で同じことをする）。

---

## ターゲットの構成

`src/app/main.cpp` 以外のすべてのソースは **`vanilla_core` という静的ライブラリ**にまとまっている。
実行ファイル `vanilla` は `src/app/main.cpp` と `.qrc` だけを持ち、そのライブラリをリンクする。

- **理由はテスト。** [tests/](../tests/) の各テストが同じライブラリをリンクすることで、
  実行ファイルと**同じオブジェクト**を検証できる
- インクルードパスと機能マクロは `vanilla_core` の **PUBLIC** な使用要件にしてある。
  ヘッダの内容がマクロで変わる（`WEBENGINEVIEW` の有無でクラスの定義が変わる）ので、
  違う組み合わせでコンパイルされたターゲットをリンクすると壊れるため
- **`.qrc` は実行ファイル側に置いてある。** 静的ライブラリに入れると、リソースを登録する
  静的初期化子がリンカに落とされることがある。そのためテストはリソースを持たない
  （アイコン等を必要とするテストを書くときは、そのターゲットに `.qrc` を足すこと）

---

（Deodorized. This tree is published with its source comments
and internal annotations stripped; the diff against the original
is [deodorant.zip](deodorant.zip).）

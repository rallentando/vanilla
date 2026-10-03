# アーキテクチャ

最終更新: 2026-09-10

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
| [src/input/](../src/input/) | アクションに名前を付けている表と JS 公開口 | `_Vanilla` / `_View` の公開名と呼び先も表から生成する（D-295）。**割り当ての読み書きは [src/core/inputmap.hpp](../src/core/inputmap.hpp)**（D-034） |
| [src/ui/](../src/ui/) | ウィジェットと描画 | `MainWindow` / `TreeBank` / `TreeBar` / `Theme` |
| [src/view/](../src/view/) | 表示エンジンごとの `View` 実装 | WebEngine / Native / Local |
| [src/gadgets/](../src/gadgets/) | サムネイル一覧（俯瞰 UI） | `QGraphicsScene` 側 |

- **`#include` は全部ファイル名だけの裸書き**で、各ディレクトリがインクルードパスに載っている。
  だからファイルを別の責務へ移しても `#include` は書き換えない
  （[CMakeLists.txt](../CMakeLists.txt) の `target_include_directories`）
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
| QML ビューのプロファイル | `<Base>/data/webengine/<MD5(実行パス + "/quick:" + id)>` | 公開QML `WebEngineProfilePrototype` へ名前とpathを生成時に渡す。保存先は維持（D-313） |
| Edge拡張の所有記録 | `<Base>/data/extension-registry/<SHA256(environmentの保存先 + profile名)>.json` | Add前のintentをQSaveFileで保存。通常のconfig保存とは独立（D-313） |

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
置き場を動かしたD-044 / D-045 / D-099の移行はアプリ内にあるが、0.2.2以前のXMLを読む
1リリース限りの移行口はD-298で終了した。

**Windows で `AppDataLocation` は Roaming、`AppLocalDataLocation` は Local。**
D-099 でプロファイルがエンジンの既定に戻ったので、**インストール版の状態は
Local（`data/`）と Roaming（プロファイル）に分かれている。**
ローミングプロファイルを使う環境なら、そこは考えどころになる ——
Chromium のプロファイルは SQLite と LevelDB のロック付きファイルなので、
ログオンのたびに運ばれると壊れる（Chrome や Edge が user data を Local に
置くのはこのため）。しかも **QtWebEngine に「ローミング対応」は無い** ——
それは Chrome の機能で、6.11.2 のツリーにも 6.12.0（QtWebEngine 6.140.0）のツリーにも入っていない
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

0.2.2以前の `.xml` は読まない。0.3.0で移行期間を終え、次版から読込・書出ともJSONだけにした
（D-298）。ブックマークのXML/XBEL入出力は実行時dataとは別で、引き続き利用できる。

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
  バックアップの探し方、バックアップから復帰したときの通知は
  `SettingsIO::Hooks`（`std::function` 3つ）として渡される。
  `Application` 側の実体は `src/app/application.cpp` の `SettingsHooks()`
- **`Load` は「JSON → `.prev` → JSONバックアップ世代（新しい順）」の順に試す。**
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
- **旧セッションXMLは読まない。** 0.3.0で移行期間を終え、設定・cookieも含めて
  実行時dataの読込形式はJSONだけにした（D-298）

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
| Chrome の `Bookmarks` | ✅ | — | Opera / Vivaldi / Edge も同じ形（取り込みの一覧では Edge を先頭に出す）。複数のルートを1つの `Favorites` フォルダにまとめる |
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
- ノードから View を生成するファクトリを持つ。**既定は `WebEngineView`、`EDGEWEBVIEW` のあるビルド（Windows x64）では WebView2 ランタイムがあれば `EdgeWebView`**（D-454）、
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
| `WebEngineView` | `WEBENGINEVIEW` | **既定**（Windows では Edge が無いときだけ。`web` で名指しする。D-454）。QWebEngineView ベース。WebEngine 系は `src/view/webengine/`（D-249） |
| `QuickWebEngineView` | `WEBENGINEVIEW` | QQuickWidget + QML（[view/quickwebengineview6.qml](../src/view/webengine/quickwebengineview6.qml)）。widgets 層の不具合からの逃げ場（D-061） |
| `LocalView` | `LOCALVIEW` | ローカルファイル閲覧。`GraphicsTableView` と `View` の多重継承。何のファイルかは [view/mediatype.hpp](../src/view/mediatype.hpp) が決める（D-024） |
| `QuickNativeWebView` | `NATIVEWEBVIEW` | QWidget + QQuickView + 窓コンテナ（D-070）。Windows では **Edge WebView2** —— Qt WebEngine 丸ごとの不具合からの唯一の逃げ場 |
| `EdgeWebView` | `EDGEWEBVIEW` | QWidget + **`ICoreWebView2` を自前でホスト**（D-166）。**窓は自分で作って container で置く**（D-168。ウィジェットのネイティブ窓は Qt が作り直すので使えない）。Windows と x64、`third_party/webview2` がある場合のみ。**そのビルドでは種類を書かないディレクトリの既定**（WebView2 ランタイムが無い機械では `WebEngineView`。D-454）。**Qt WebView を通さないのでホスト側の口が使える** —— キー・メニュー・戻り道は C-7 c。**a〜e まで実機で確認済み**（比較表の Edge 列がその結果）。**ページはバックエンドの窓ではなく DirectComposition のビジュアルに描かれ**、マウスはこちらが `SendMouseInput` で渡す（C-7 e、D-190）。合成が作れない機械だけ窓ありに落ちる。**ソースは `src/view/edge/` に責務ごとの 8 つの `.cpp`**、COM を含む私有部は `edgewebview_p.hpp`（D-249。どこに何があるかはそのヘッダ冒頭の一覧）。**backend は controller が bounds を得た時点で見せる**（D-253 で D-250 の cover を外した。1〜2 フレームのために読み込みが遅く見えていた。controller が来る前は HWND の下地塗りが見える） |

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
| Private (OTR) | ○※ (D-202, D-428) | ○※ (D-201, D-428) | ― | ○※ (D-166, D-199, D-202, D-220, D-428) |
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
| 権限（local fonts） | ○ (D-332) | ○ (D-332) | ― | ○※ (D-194, D-199, D-332) |
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
| ダウンロード統合 | ○ (D-279) | ○ | × | ○※ (D-172, D-173, D-279) |
| ページ設定（ディレクトリ設定） | ○ | ○ | ○※ (D-082) | ○※ (D-192) |
| UA / AcceptLanguage | ○ | ○ | ○※ (D-076) | ○ (D-192, D-193, D-199) |
| `vanilla://` ページ | ○ | ○ (D-073) | ―※ (D-218) | ○ (D-196) |
| ソース表示 (view-source:) | ○ | ○ | ○※ | ○ (D-248) |
| 文字列の文書（`setHtml` / ApplySource） | △ | △ | △ | ○※ (D-332) |
| Inspector | ○ (D-064) | ○ (D-086) | × | ○ (D-192, D-245) |
| Inspector をウィンドウ内に | ○ (D-087) | ○ | ― | ○※ (D-245) |
| 動画再生位置 (MEDIATIME) | ○ | ○ (D-081) | ○※ (D-083) | ○ (D-191, D-199) |
| 印刷 | ○ | ○ (D-161) | ×※ | ○ (D-192, D-199) |
| フォント設定（11件） | ○ | × (D-161) | × | ×※ |
| ページからの `window.print()` | ○ (D-091) | △ | ― | ○ (D-192, D-199) |
| 全画面 | ○ | ○ | × | ○ (D-192, D-199) |
| 新窓 | ○ | ○ (D-200) | × | ○ (D-172) |
| ページから見える `chrome.webview` | ― | ― | ― | 隠す (D-339) |

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
  quick が**その場の差し替え**（D-201）。どちらを使うかはノードの継承済みの言葉だけで決まる（D-428）。Edge が空で始まるのは消去の完了を待つから（D-220）
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
- **権限（local fonts）**: 2026-09-12 に **widgets / quick / Edge の3つで実機を通した**（D-332）。
  頁のボタンから `queryLocalFonts()` を呼ぶと3つとも許可ダイアログ（機能: インストール済み
  フォントの一覧）が出て、「はい」で 294 書体が読めた。http の origin では再読込後も `granted`。
  **Edge の ※ は `file://` の頁では再読込で `prompt` に戻ること**（Chromium が file の origin に
  権限を残さない。widgets / quick は file:// で測っていない）。ダイアログを伴う権限として
  実機で見たのは通知（Edge は D-194 / D-199、quick は D-274）とこれ
- **文字列の文書（`setHtml` / ApplySource）**: Edge は `NavigateToString`（D-332）。**base url が
  無い**ので相対リンクは解決されず、文書自身は `about:blank` を名乗る。アドレスバー・ノード・
  履歴は `setHtml` に渡した address のまま（`EdgeIsOwnStringDocument`）で、その文書からの
  キー・スクロールの報告も通る。実機で通したのは Edge の ApplySource だけで、他の3つは
  配線があるだけ（QuickNative は `Page::SetSource` の色付けがこれを使う）
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
- **QuickNativeのlocalStorage設定**: Qt 6.11.2のWebView2 pluginは`setLocalStorageEnabled()`を
  未対応として報告する（D-302）。D-082の設定配線はon/offがbackendへ反映される保証ではない。
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
- **ダウンロード統合**: 進捗の行は**どのバックエンドも Notifier の同じ行**（`DownloadItem` 経由）。
  Edge は `EdgeDownloadAdapter` が quick の download item と同じ property / signal / slot を出し、
  それを `DownloadItem(QObject*)` が読む（D-279）。**「一覧 UI」はまだ無い**（ROADMAP C-7。
  行は進捗と完了だけで、履歴として残る一覧ではない）。
  Edge の**ダウンロード専用タブ（履歴の無い leaf）は木から外して隠し持つ** ——
  `TreeBank::ExtractDownloadCarrier` → `EdgeDownloadCarriers`（`Application` の子）。
  終端で捨て、`Application::TakeDown` が `ReleaseAllView` の直前に全部落とす（D-279）
- **ページ設定（ディレクトリ設定）**: QuickNative は**一部**（D-082）、
  Edge は **JS とエラーページだけ**（D-192）
- **UA / AcceptLanguage**: QuickNative は **UA だけ**（D-076）
- **`vanilla://` ページ**: QuickNative では開けず、**代わりに WebEngine が開く**（D-218）
- **Inspector をウィンドウ内に**: Edge は**別プロセスの窓を取り込む**
  （D-245。可否は3実装共通の設定 `webview/@InspectorInMainWindow`＝既定オン。D-247）
- **動画再生位置 (MEDIATIME)**: QuickNative は**ポーリング**（D-083）。終了時は全実装の
  最終取得を最大2秒待ち、イベントループへ戻ってからビュー解放とセッション保存を行う（D-293）
- **印刷**: QuickNative は印刷ダイアログを持たず、**PDF 書き出しだけ**（`printToPdf`）
- **フォント設定（11件）**: Edge の × は API ではなく、**設定そのものが無い**

補足（読み違えやすいところだけ）:

- **Edge 列の × は「API が無い」**で、どれも「これから実装する」ではない
  （D-196。SDK 1.0.4129.50 の `WebView2.h` で数えた）。**例外はフォント設定**で、
  こちらは API ではなく設定そのものが無い（上の注）
- Edge列に常設の実機確認待ちは無い（local fonts は D-332）。POST遷移は比較表では△だが呼び出し元がなく、
  常設の確認待ちにはしない（D-271 / D-302）。ダウンロード統合はD-279で○※、一覧UIは未実装。
  残る確認範囲はHANDOVER.mdの一覧を参照
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
  効かない。Quickの通常profileはD-313で対応。残件の決着はD-314）

### `Gadgets` / `GraphicsTableView` — [src/gadgets/](../src/gadgets/)

「タブ一覧」としてタブ・ディレクトリを表示・編集する UI。`QGraphicsScene` 上に構築される。
表示タイプは「通常」「再帰表示」「再帰表示（折りたたみ可）」の3種類。
内部値は Flat=0、Recursive=2、Foldable=3。廃止タイプを含む候補外の保存値は通常へ戻す（D-305）。
設定画面の表示タイプ変更・リセットは既存の全窓にも適用し、可視の一覧を再収集する。
非表示なら次回表示で反映。他設定の変更では窓別のメニュー選択を維持する（D-308）。

- `GraphicsTableView` が一覧の本体（`DisplayType` で ViewTree / TrashTree / LocalFolderTree などを切り替える）
- `AbstractNodeItem` / `Thumbnail` / `NodeTitle` が個々の項目
- `GadgetsStyle` が描画。`GlassStyle`（暗い半透明。既定）と `FlatStyle`（明るい）の2実装があり、
  どちらも色は [theme.hpp](../src/ui/theme.hpp) から引く
- サムネイルの当たり判定は `GadgetsStyle::ThumbnailHitRect` → `Thumbnail::shape()`。`boundingRect` はセル全体
  （描画範囲）、`shape` は描いているカードで、FlatStyle はカード間の余白を背景（table）に返す（D-338）

### 入力とアクション

| ファイル | 役割 |
|---|---|
| [actionmapper.hpp](../src/input/actionmapper.hpp) | アクション名の一覧と、公開 JS API の `(呼び先, camelCase名)` 対応表を定義する（D-295） |
| [keymap.hpp](../src/input/keymap.hpp) | キー → アクション名 の既定割り当て。ビューと `TreeBank` の表の**単独キー**（Ctrl / Alt / Meta なしで文字を打つキー。`InputMap::IsSingleKey`）は、設定 `webview/@EnableSingleKeyShortcut`（**既定オフ**、要再起動）が有効なときだけ効く。押したキーは `View::KeyAction` / `TreeBank::KeyAction` で引く。パイプの `key` とサムネイル一覧・アクセスキーの表は止めない（D-430） |
| [mousemap.hpp](../src/input/mousemap.hpp) | マウスジェスチャ → アクション名 の既定割り当て。**`Ctrl+WheelUp` のような名前を組み立てる側は `Application::WheelWentUp`**（向きは `angleDelta`。`pixelDelta` は普通のマウスでは空。D-123） |
| [commandmap.hpp](../src/core/commandmap.hpp) | コマンドの綴り → アクション名。`back` / `backward` のような綴りの揺れはここにしかない（D-054） |
| [receiver.hpp](../src/app/receiver.hpp) | 入力欄。Command / Query / UrlEdit / Search の4モードを持つ。`ReceiveCommand` は `CommandMap` に綴りを引かせ、返ってきたアクション名の signal を出すだけ |
| [jsobject.hpp](../src/input/jsobject.hpp) | ページ内 JavaScript から叩ける API。単純な無引数スロットは `actionmapper.hpp` の対応表から生成する |

> アクション名、入力の綴り、公開 JS 名という3系統は互換性のため残る。
> 公開 JS 名と呼び先は同じ対応表の行に置き、`_Vanilla` / `_View` のスロットをそこから生成する。
> API 名の全集合と `_View` の全 dispatch は `tst_commandmap` が固定する（D-295）。

### その他

| クラス | 役割 |
|---|---|
| `NetworkController` — [networkcontroller.hpp](../src/app/networkcontroller.hpp) | `NetworkAccessManager` の生成、Cookie、プロキシ、認証 |
| `Saver` — [saver.hpp](../src/app/saver.hpp) | 自動保存。GUIで設定・窓対応・Cookie等の値snapshotを採り、workerは直列にファイルを書く。保存中の要求は最新1件へ集約し、鎖全体の終端まで保存中を維持する（D-309） |
| `Transmitter` — [transmitter.hpp](../src/app/transmitter.hpp) | 多重起動時に、既存プロセスへ引数を渡して自分は終了する（`QLocalSocket`） |
| `UserAgent` — [useragent.hpp](../src/core/useragent.hpp) | 名乗るブラウザの表。綴り・正名・既定テンプレートが 15 行で、設定の読み書きもここ（D-058）。`%SYSTEM%` を埋めるのは `NetworkAccessManager::SetUserAgent` |
| `DownloadName` — [downloadname.hpp](../src/core/downloadname.hpp) | ダウンロード先のファイル名を作る規則。1成分に落とし、書ける綴りにし、宣言された MIME に合う拡張子を足す（D-264）。自前のダウンローダは `Suggest` を通し、エンジンが作った名前は Chromium 側が同じことを済ませているので `Sanitize` だけ。`Unique` は名前ではなく **path** を受け、埋まっていれば `_1`, `_2` を拡張子の前へ入れる |
| `WindowLedger` — [windowledger.hpp](../src/core/windowledger.hpp) | どのウィンドウがあって、今どれかを持つ表。ウィンドウ型でテンプレート化してあり、`Application` の `NewWindow` / `SwitchWindow` / `RemoveWindow` / `GetMainWindows` はこれを呼ぶだけ（D-056） |
| `Notifier` — [notifier.hpp](../src/ui/notifier.hpp) | ステータス・進捗の表示 |
| `Dialog` — [dialog.hpp](../src/ui/dialog.hpp) | 自前描画のモーダル / モードレスダイアログ。モードレスは窓の右 1/4 に積む別窓（`ModelessDialogFrame`）で、本文は幅いっぱいに折り返し、ボタンはその下。Windows で Qt Quick が OpenGL のときは全画面の窓に 1px の枠を付ける（枠が無いと全画面の窓が画面を独占し、重なり順で上にある別窓もモニターに出ない） |
| `MiniMap` — [minimap.hpp](../src/ui/minimap.hpp) | ページの脇の帯。絵は写真ではなく**注入 JS が集めた矩形の図式**（D-209）。widgets / quick / Edge の3実装に対応し（D-210）、クリックでジャンプ・ドラッグでつまみ・ホイールはビューのホイール経路へ渡す（D-238）。fixed / sticky は緑の背景、その配下は通常の種類別配色で重ね、トップ文書のものはインジケータへ追従する（D-287）。要素は幅優先で収集し、長い部分木より浅い見出しや周辺部を先に処理する。採用・訪問上限は各100,000、textのRangeは文書ごとに再利用する（D-291）。矩形応答は6値ずつ並ぶフラット配列で、C++は件数分を事前確保して変換する（D-292）。再収集はロード・`Grown`・スクロール静止後（D-237）。設定 `application/@EnableMiniMap` は**既定オフ**で要再起動、色は Theme の `MiniMap*` ロール8つ |

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
**行は表の順に出るので、同じ話題の行の隣に足す**（群の境は表のコメント。並びの決めかたは D-455）。

- **既定値は、そのキーを読むコードが `s.value` に渡すものと同じにすること。**
  [tests/tst_settingsschema.cpp](../tests/tst_settingsschema.cpp) がソースを読んで照合する。
- **書けるのは表に載っているキーだけ**で、型と選択肢も検査される。
  スキームは `CorsEnabled` 無しで登録してあるので、よそのページからは API に届かない。
- `QWebChannel` は使っていない（`USE_WEBCHANNEL` は無効のまま）。
- **キー・マウス・ジェスチャの 10 表**は行の表ではなく [inputmapschema.hpp](../src/core/inputmapschema.hpp) が描く（D-431）。
  カテゴリ「キーとマウス」で一覧（チートシート）と編集を兼ね、`/api/input-set` / `/api/input-reset` が表を丸ごと書く。
  設定画面ではビューがキーを先取りしない（記録のため。取らないキーは注入の報告で表へ戻る）。

開き方は4通り。`settings` コマンド、メニューの「設定」、拡張一覧の「拡張機能を管理」、
そして**アドレスバーに `vanilla://settings` と打つ**。
前の3つは `TreeBank::OpenSettings(category)` で**同じ1つのタブ**を使う（開いていれば作り直して前へ出す）。
アドレスバーに打ったものは下の「アドレスバーに打った文字」のとおり `IsUrlToLoadInPlace` に載っているので
**今のタブ**で開く。そのタブは末尾の `/` の無い `vanilla://settings` を持ち得るが、`VanillaPage::SameDocument`
は空のパスを `/` と読むので、次の `OpenSettings` はそのタブを見つけて使う（R-109c）。
表示中のカテゴリはページのフラグメント（`vanilla://settings/#network`）で、カテゴリのクリックは
フラグメントを書き、`hashchange` で切り替える（戻る・進む・アドレスバーの手入力も同じ経路）。
フラグメントが無い、または無いカテゴリを指すときは先頭のカテゴリで、起動時も `hashchange` も同じ
`categoryOfFragment` で解決する。戻るでフラグメントの無い住所へ帰ると、その住所が最初に見せた先頭へ戻る。
タブの検索はフラグメントを除いて比べる（`VanillaPage::SameDocument`）。

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
`RightGesture` / `Encoding EUC-JP` など。読む側は [treebank.cpp](../src/ui/treebank.cpp) の
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

**`Private` はビューごと**（D-428）。空間の `NetworkAccessManager` は通常のプロファイルと
private のプロファイルを 1 つずつ持ち（private は初めて求められたときに作る）、どちらも差し替えない。
どちらを使うかは各ビューが自分のノードの継承済みの言葉で決める（`DirectoryPage::SaysPrivate`。
widgets・quick・Edge とも同じ）。言葉が無ければ通常。private のセッションは空間ごとに 1 つで、終了まで続く。

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
`View::ApplySpecificSettings` の属性表・マウスジェスチャ・スーパードラッグ・
`AutoLoadWithLink` の `noload` が同じ1本を通るので、**「否定が勝つ」の写しが散らばらない**。
マウスジェスチャは画面上の名前で、正規トークンは `RightGesture` / `!RightGesture`。
何も書かなければグローバルの `webview/@EnableMouseGesture` に戻り、各ビュー個体が
解決済みの値を持つ（D-310）。手入力の `MouseGesture` も同じ設定として読む。
ディレクトリ変更時の既存ビューへの再適用はnetwork ID境界を越えて全子孫を辿り、
各ノードの `GetNodeSettings` を取り直す。managerの切替範囲とview設定の継承範囲は混ぜない。

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
- 設定とディレクトリ設定は`appearance: base-select`対応時にページ内pickerを使う（D-311）。配置は下方向から試し、収まらない場合だけ反転する。pickerに`min-height: min-content`を置くのは、ブラウザ既定の`max-height: stretch`が一覧を残りの空間まで縮めて「下に収まった」ことにしてしまうため（D-341）。ディレクトリの既定選択肢は継承元→グローバル/固定値から有効・無効を解決し、日本語では「既定（有効）／既定（無効）」と表示する。
- 両設定ページの本文は文書全体でスクロールし、通常ページと同じミニマップでクリック・ドラッグ・ホイール操作できる。ヘッダーと設定カテゴリはstickyで追従し、カテゴリ切り替えは文書の先頭へ戻る。設定ページのヘッダー高とカテゴリの上端は66pxで揃える。ミニマップの有効・無効は既存の全体設定に従う。
- 上位の開閉見出しは「表示／非表示」を切り替える。設定保存による再描画でも直前の開閉状態を維持する。展開中は上位全体をテーマの沈んだ背景とアクセント枠で囲む。対象カードは「このディレクトリ」と左線で示し、開閉で左位置を変えない。上位の字下げは4段で止める。
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

代わりに `application/@ChromiumFlags`（設定画面の「描画とエンジン」）がある。
TextList（スペルチェックの辞書と同じテキストエリア）で、空白/改行区切り、それぞれ `--` で始まる語だけが通る。
1 行に何個書いてもよく、行ごとに読む。値に空白があるスイッチは
`--switch="a b"` のように二重引用符で囲む（D-343）。
`Application::ApplyChromiumFlags()` がそれを環境変数
`QTWEBENGINE_CHROMIUM_FLAGS` に入れ、Qt がエンジン起動時に読む。
Edge ビューには [edgeenvironment.cpp](../src/view/edge/edgeenvironment.cpp) の
`get_AdditionalBrowserArguments` が同じ語を渡す（自動再生のスイッチの後ろ。自動再生は設定の真偽どちらでも明示のスイッチを渡す: D-453）。
語の切り出し `Application::ChromiumSwitches` と書き戻し `JoinChromiumSwitches` は両方で共通。

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

## Web ビューの描画 API

`application/@GraphicsApi`（設定画面の「描画とエンジン」、`Auto|Software|OpenGL|Direct3D11|Direct3D12|Vulkan|Metal`、
既定 `Auto`）を `Application::ApplyGraphicsApi()` が `QQuickWindow::setGraphicsApi` に渡す（D-340）。
`QWebEngineView` も `QuickWebEngineView` も中身は QQuickWindow なので、Qt Quick の RHI バックエンドが
Web ビューの描画 API になる。

- **`BootApplication` が設定を読んだ直後**に呼ぶ。`setGraphicsApi` は最初の QQuickWindow より前で
  ないと効かない
- 語は `QSGRendererInterface::GraphicsApi` の名前（他の選択肢と同じパスカルケース。D-342）で、
  `Application::GraphicsApiFor()` が enum に写す。`Auto` と知らない語は `Unknown`（Qt に任せる）。
  D-340 の時点では `QSG_RHI_BACKEND` の語（`opengl`、`d3d11` …）だったので、読む側は大文字小文字を
  問わず旧綴りも受け、設定画面は `SettingsSchema` の `Canonical` / `RENAMED` で今の綴りに直して見せる
- **環境変数 `QSG_RHI_BACKEND` があれば設定は無視する**（`ApplyChromiumFlags` と同じ流儀）。
  `setGraphicsApi` は環境変数を上書きしてしまうので、先に見る
- **その OS で動かない API は `Application::GraphicsApiRunsHere()` で弾いて `Auto` 扱い**
  （Direct3D は Windows、Metal は Apple、Vulkan は Apple 以外）。ドライバの有無まではここでは見ず、
  Qt が最初の窓で言う
- 反映は次の起動から（`needsRestart`）。動機は [BUILD.md](BUILD.md)
  「入れ替え後のちらつき: D3D11 の受け渡し競合」
- 主窓（widgets）も同じ API で最初から RHI にするのは `application/@EnableMainWindowRhi`（既定オフ）が
  オンのときだけ（`Application::ApplyWidgetsRhi`、D-458 / D-463）。オフでは主窓はラスタで、最初の WebEngine の
  ビューが入るときに一度作り直される。オンだと GeForce Experience などのオーバーレイがゲームとみなす

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

表示中のタブは専用の淡い背景と3論理pxのアクセント線で示す（D-300）。横TreeBarは上辺、
縦TreeBarは左辺。祖先フォルダのフォーカスとは別に、その窓の現在ノードとの一致で判定する。
テーブルのprimaryも同じ青のRGBを使う。Recursive／FoldableのViewTreeは現在ViewNodeに印を付け、
そのノードが隠れたら最も近い可視祖先へ、再表示されたら元のタブへ印を戻す。祖先も無ければ印なし（D-301）。
Foldableの開閉は200msのOutCubicで補間する（D-303）。木とprimaryは即時更新し、
画面内のlive項目は一時描画矩形と透明度、消える項目は画像で移動・伸縮する。
出入りするサムネイルの起点／終点は操作ディレクトリの中心。タイトル一覧は中心線上で幅を自分の
まま高さだけ補間する（D-303 追記 2026-09-15）。Flatへの再表示では階層opacityを1へ戻す。
再開閉や再配置・scroll・表示切替で補間を確定し、一時矩形と画像を残さない。
縦TreeBarのFold/Unfoldも人工ロックを掛けず、次の通知で前の描画を確定する（D-304）。
ドラッグ完了callbackは強制実行せずモデルから再構築し、通常scrollやcontrolの入力保護は維持する。

パレットは2本ある。`application/@ColorScheme` が `Auto`（既定）/ `Light` / `Dark`。
`Auto` はデスクトップに追従し、実行中に変わっても拾う。決定は D-018 / D-019 / D-113。
**読む側は大文字小文字を見ない**（D-113 より前のファイルは全部小文字）。

- **スタイル（`GlassStyle` / `FlatStyle`）とは独立**。スタイルは形、配色は色。
- **色を「保持する」ものは push で更新する。** 毎フレーム引き直さないのは7つ —
  TreeBar のシーン背景、`LayerItem` の区切り線、TitleBar のボタン、`QAction` のアイコン、
  **`LineEdit` のスタイルシート**（コマンドラインとアドレスバーの入力欄）、
  **`QWebEnginePage` の背景色**、**`ExtensionBar` のアイコンと窓の `QPalette`**（`ApplyTheme`）。
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
  自前タイトルバーは高さ32（DeviceScale前）の独立窓。最大化時は右端の5ボタンだけに縮め、
  普段は隠し画面最上端へのカーソル到達で表示する。上余白は追加しない（D-307、D-306の全幅化を撤回）。
  親所有timerはMainWindowが最大化/表示中のときだけ観測し、全画面・最小化では停止/非表示。
  EdgeのGotFocusは有効な表示中current viewだけが現在窓と枠のZ順を更新する。
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
CMake が `find_package` の結果に応じて `vanilla_core` に **PUBLIC で**付ける。

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

### 既定のページメニュー（2026-09-07）

`View::LoadSettings` の RegularMenu は新規タブから始め、ブックマークレット追加・
ブックマークレットのサブメニューを既定値から除く。保存済みのメニュー設定はそのまま使う。
ブックマークレットのコマンドとカスタムメニュー用項目は引き続き利用できる。

### 拡張機能（D-313）

`ExtensionController`（[src/app/extensioncontroller.cpp](../src/app/extensioncontroller.cpp)）は
実profileごとに非同期操作を直列化し、`network/@Extensions`、`network/@DisabledExtensions`、
`network/@PinnedExtensions` の共通設定へ照合する。Qtでは実profile配下、Edgeではenvironment配下で
実測したprofile名ごとに共有する。Quickの補助 `WebEnginePage` のprofileは操作しない。
登録のキーは登録したパス（`ExtensionManifest::path`）で、読むのは `folder`。Chrome の `Extensions/<id>/<版>_<n>` を登録していて、
Chrome の更新でそのフォルダが消えた場合は、隣の最新版を読む（`CurrentFolder`、D-383）。

`webengineextensions.cpp` はQtの管理器・Quick prototype・同profileの拡張ページ、
`edgeextensions.cpp` はWebView2 Profile7とnative popup controllerを担当する。
`View::Extensions / CreateExtensionView` を `ExtensionBar`（[src/ui/extensionbar.cpp](../src/ui/extensionbar.cpp)）が使い、
ToolBarのアドレス入力欄の右に一覧ボタンとpinを配置する。profile変更・view切替・拡張変更でpopupを閉じる。
一覧の「拡張機能を管理」はメニューの「設定」と同じ `TreeBank::OpenSettings("network")` で設定タブを開く。
一覧の「追加」と設定ページの参照ボタンは同じ `ExtensionController::DefaultPickDirectory()`（Chromeの
`Default/Extensions`、無ければ Chrome の User Data、それも無ければホーム。選択は `PickDirectoryFrom(root, home)`
で tst_extensions が固定）から folder dialog を開く。作業ディレクトリでは開かない。
バーのボタンはToolBarと同じ `TOOL_BAR_ICON_SIZE` / `TOOL_BAR_ICON_SPACING`、アイコンは
`resources/toolbar/extension.png` と `pin.png`（`draw_icons.py` の線画、`Theme::Pixmap` でink）。
一覧と拡張ページの窓は `NodePreview` と同じ `Preview*` の色（不透明な面と1pxの縁を `paintEvent` で描き、
中のQt標準ウィジェットには同じ色の `QPalette` を渡す）。寸法は const.hpp の `EXTENSION_*`（D-315）。
一覧のピンは checkable な `QToolButton` で、押下状態はスタイルの強調色に乗るので `QIcon::On` だけ `Theme::Fill` で
`PreviewBackground` に塗る（`Theme::Ink` / `Pixmap` はライトではビットマップをそのまま返すので使えない）。
Edge の `Enable` / `Remove` が拒まれたら `EdgeExtensionOperation`（edgewebviewstate.hpp）に従って一覧を取り直し、
新しい object で一度だけやり直す。engine にもう無い拡張は `ExtensionItem::gone` で controller に伝わる（D-337）。
Edge/Quickの拡張ページはTool|Frameless hostを使う。EdgeはQt::Popupのmouse grabを避け、
Quickはnative focus windowとrender windowを一致させる。Edgeのnative Escはcontrollerイベントで扱い、
callbackから戻った後にreceiver付き予約で閉じる。tokenはcontroller Close前に解除する。

Edgeのaction popupは、WebView2が1ビュー=1ウィンドウなので `chrome.tabs.query` の「現在ウィンドウの
active tab」がpopup自身になる。`View::EdgeExtensionTabQueryJsCode` をpopupのtop documentへ
`AddScriptToExecuteOnDocumentCreated` で入れ、`active` / `currentWindow` / `lastFocusedWindow` /
`WINDOW_ID_CURRENT` を含む呼び出しだけ、無条件の実queryの結果（id・url）をweb messageでホストへ送り、
ホストが元ビューの `get_Source` と一致する1件をidで名指しする（同URLが複数なら名指ししない。titleは使わない。
`EdgeExtensionTabRequest`、順序は `EdgeExtensionPageState`）。呼び出し側の条件はその後にengineへ渡し、名指しされたtabだけを返す。
要求はdocumentごとのmarkとseqを持ち、受信時と応答時で元ビューのcommitted addressが違えば名指ししない。
ホストからURLは渡さず、同origin以外の要求には答えず、manifestに `tabs` 権限も `host_permissions` も無い拡張には入れない
（popupを開くたびに読み直し、wrapper側も `chrome.runtime.getManifest()` で確かめる）。host 権限だけの拡張（uBOL）には、
元ビューの住所に host 権限が当たるときだけ名指し、名指さなければエンジン本来の答え（popup 自身）を返す（D-389）。options page（`View::ExtensionOptionsPage`）は
単独のタブなので入れない（D-335）。
Qt ビューのポップアップと設定ページが `window.open` やリンクの target で求める新しい窓は、`tabs.create` と同じ門（`TabUrlOf`）を通ったものだけ、対象のビューから新しいタブとして開き、パネルを閉じる（D-412）。ページが `window.close()` を呼んだとき（uBOL の「要素を削除」）もパネルを閉じ、開いたビューにフォーカスを戻す（D-423）。

通常のWebEngineView / QuickWebEngineView / EdgeWebViewが対象。Manifest V3の展開済みフォルダーを
参照し、Chromeの登録元を変更・コピー・削除しない。privateと他backendは利用不能理由を表示する。
Chromeの完全な代替APIではなく、`action.onClicked` の発火、activeTabの付与、Chrome固有の
タブ・ウィンドウ・badge等の統合は提供しない（拡張の context menu は Edge だけ D-377 で載る）。popup URLがある拡張ページは表示できるが、
拡張自身が要求するAPIによって機能しない場合がある。
有効化・content script成功は広告ブロックや拡張UI互換性の保証ではない。

#### Chrome拡張の機能比較（2026-09-14 時点、D-313〜D-315・D-335 まで反映）

印は上の「ウェブ4実装の機能比較」と同じ: ○=実機確認済み / △=配線済みだが実機未確認 / ×=不可・未配線 /
―=土台の API が無く原理的に不可能。加えて **？=未計測**。**`※` は条件つき**で、表のすぐ下の
「行ごとの注」に一行ずつある。**Chrome 列は仕様どおりの印で、こちらの実機確認ではない。**
**セルには印と D 番号だけを置く。**Qt への報告は保留、Qt 側の互換層は見送り（D-314）。
**Qt 2 列の `―` / `？` は素のエンジンの印のまま**で、その後 D-350 以降の互換レイヤーが `action`・badge・context menu・`tabs.query`・`notifications`（D-418b）・`commands`（D-416）ほかを埋めた。
互換レイヤーでの到達点は下の「拡張の互換レイヤー」の項と「Chrome拡張 API の名前空間の分類」を見る（この表の印は実機の確認を伴うので、行ごとに測るまで書き換えない）。

| 機能 | Chrome | WebEngine | QuickWebEngine | Edge |
|---|---|---|---|---|
| 展開済みフォルダーの登録・有効/無効・ピン | ○ | ○ (D-313) | ○ (D-313) | ○ (D-313) |
| Web Store / CRX からの導入 | ○ | ― (D-313) | ― (D-313) | ― (D-313) |
| 一覧・切替・ピン・popup の UI | ○ | ○※ (D-313, D-315) | ○※ (D-313, D-315) | ○※ (D-313, D-315) |
| private プロファイルでの拡張 | ○※ | ×※ (D-313) | ×※ (D-313) | ×※ (D-313) |
| content script の注入 | ○ | ○ (D-313) | ○ (D-313) | ○ (D-313) |
| 無効→有効で開いたままのページへの再注入 | ×※ | ？ | ？ | ×※ |
| `chrome.storage.local` の共有 | ○ | ○ (D-313) | ○ (D-313) | ○ (D-313) |
| action popup の表示 | ○ | ○※ (D-313) | ○※ (D-313) | ○ (D-313) |
| `chrome.i18n.getMessage` | ○ | ○※ (D-373) | ○※ (D-373) | ○ (D-313) |
| options page | ○ | ○※ (D-313) | ○※ (D-313) | ○※ (D-313, D-335, D-378) |
| 静的 DNR の遮断 | ○ | ○※ (D-347) | ○※ (D-347) | ○ (D-313) |
| 動的 / session の DNR ルール | ○ | ？ | ？ | ？ |
| chrome 層 API の存在 | ○ | ―※ (D-314) | ―※ (D-314) | ○※ (D-314) |
| popup の `tabs.query` が現在タブを返す | ○ | ― (D-314) | ― (D-314) | ○※ (D-335, D-378) |
| popup から現在タブへ `tabs.sendMessage` | ○ | ― (D-314) | ― (D-314) | ○ (D-335) |
| Service Worker（MV3 background） | ○ | ？※ (D-314) | ？※ (D-314) | ○※ (D-313) |
| `action.onClicked`（popup を持たないボタン） | ○ | ― (D-314) | ― (D-314) | ― (D-335) |
| activeTab の付与 | ○ | ― (D-314) | ― (D-314) | ― (D-335) |
| badge / title / アイコンの差し替え | ○ | ― (D-314) | ― (D-314) | ―※ (D-335) |
| 拡張が登録する context menu | ○ | ― (D-314) | ― (D-314) | ○※ (D-377) |
| `chrome.notifications` の表示 | ○ | ― (D-314) | ― (D-314) | ？※ |
| `commands`（キーボードショートカット） | ○ | ？ | ？ | ？※ |

行ごとの注:

- **一覧・切替・ピン・popup の UI**: Chrome 独自の UI ではなく、アドレスバー右の puzzle ボタン（D-315）
- **private プロファイル**: Chrome は許可制。Vanilla は 3 実装とも管理せず、注入もしない
- **無効→有効の後の content script**: Chromium は無効化で外し、有効化では開いたままのページへ再注入しない。
  再読み込みで戻る。Vimium 2.4.2 は `onInstalled` の更新時だけ自前で既存タブへ注入する。Edge は 2026-09-14 に
  自作 fixture で実測（無効→有効直後は `sendMessage` が Receiving end does not exist、再読み込み後は応答）
- **action popup の表示**: Qt 2 種は chrome 層 API を使う拡張（Stands 2.1.71）で本文が欠ける
- **`chrome.i18n.getMessage`**: 素の Qt は空文字（`RendererHost` 未 bind、D-313 追記）。互換レイヤーのコピーは拡張の `_locales` から
  ユーザーのロケールの辞書を作ってシムの先頭に書き、シムが Chrome の規則で答える（D-373）。manifest / CSS の `__MSG_…__` 置換は無い
- **options page**: 別タブではなく popup の窓で表示。Edge では、鍵入りのコピーの拡張はページのシムがホストに尋ね（D-378）、それ以外には現在タブの解決を入れない（D-335）
- **静的 DNR**: `rule_resources` の遮断。Qt 自身は評価しない（WebRequest proxy 未接続、D-313 追記）ので、Vanilla が
  `RequestInterceptor` で評価する（`Dnr::Rules` / `ExtensionNetRules`、D-347）。block / allow / redirect（`url`・`extensionPath`）/
  upgradeScheme に対応し、`modifyHeaders`・計算型 redirect・`tabIds` つき・読んでいない条件キーを持つものは評価せず数える。
  `allowAllRequests` は、その 1 要求の allow であるうえ、**文書の下の要求に効く**（D-411）: interceptor は MainFrame 以外の要求で `firstPartyUrl`（タブの見えている住所 = 今の文書の住所。pushState 後も）を「main_frame・initiator 無し」として評価し、拡張ごとの allowAll の最大 rank（`Decision::frameAllows`）を `Dnr::Framed`（住所と Rules の世代の LRU 256）に置いて `Request::frameAllows` に渡す。その拡張の rank 以下の block / redirect / upgrade は候補から外れる（同順位は allow）。sub_frame だけに当たる allowAll は 1 要求の allow のまま。web のスキームの要求だけを評価する。private には適用しない。Edge は WebView2 が評価する。
  Qt ではエンジンの `get` / `updateEnabledRulesets` と `get` / `update(Dynamic|Session)Rules` を使わず、ホストが答える（D-393）。選んだルールセットと動的ルールは
  `ExtensionController` がプロファイルごとのファイル `extension-rules-<digest>.json` に残し（session はメモリだけ）、評価には Qt の全プロファイルの分を
  session、dynamic、static の順に入れる。2 つの update だけは引数を要求の本文で運ぶ（`ExtensionHostWire::TakesBody`、4 MiB、D-353 の読みかた）
- **拡張の互換レイヤー**（Qt 2 種。目的は Edge ビューの無い Windows 以外の環境。他 OS の枝は未ビルド・未確認）: CDP でシムを入れる最初の形（D-348 / D-349）は D-472 で外した。
  シムの本文は `src/core/cdpshims.*`（worker 用と content script 用。D-350）。WebEngine ビューは、そのシムを同梱した拡張のコピー
  （`src/core/extensioncopy.*`、D-351）を読み込む（`QtExtensions::Add`、設定 `network/@ExtensionShims`、D-352）。コピーのパスは `webengineextensions.cpp` / `edgeextensions.cpp` の外へ出ない（Edge はどの登録がコピーか・どのパスを渡したかを台帳 `extension-registry/<hash>-copies.json` に持ち、パスが変わった登録は `Migrate` が一度外して足し直す。D-378 追記 3）。Edge に渡すのは junction `extension-copies/<id>/edge`（`ExtensionCopy::Pin`）で、エンジンの起動前に今のコピーへ張り替えるので、コピーが変わっても外さず storage が残る。コピーが変わった起動は、台帳の `loaded` と比べて、`Ready` の前にその拡張を無効化→有効化し worker を入れ直す（D-396）。台帳の `keyless`（中継ページを立てるかを決める `HasKeyedShims` が読む）は、動いているコピー（`loaded`）の鍵の有無として起動ごとと入れ直しのたびに写し直す（`ExtensionCopy::KeylessOfLoaded`。`loaded` が今のコピーと違う間はどちらかが鍵なしなら鍵なし: D-438）。
  **読み込みは、そのプロファイルで最初のビューが読み込みを頼んだとき**（`ExtensionNavigation::Request` が `ExtensionController::Start()` を呼び、`Ready` を待ってからページを読む。D-382）。
  プロファイルは `cookie.json` の id の数だけ起動時に作られるが、拡張を読むのはページを見せたものだけで、未開始の controller はエンジンに何も頼まない。
  コピーの worker の番地は印に依らず `vanilla_worker.js`、worker のシムは `install` で `skipWaiting()`、`activate` で `clients.claim()` を呼ぶ（D-382）。コピーが変わってもエンジンは前の登録を使い続けるので、Qt の backend は起動で初めて有効にした拡張を直後に無効化→有効化して、その起動のコピーから登録し直させる（D-397。Edge は D-396）
  コピーのシムは、カスタムスキーム `vanilla-extension` で C++ に尋ねる（`src/app/extensionhost.*` がハンドラ、呼び元の確認・絞り込みは純粋な `src/core/extensionhostwire.*`、D-354）。
  答えるのは `tabs.query` / `tabs.get` / `windows.getCurrent` / `windows.getAll` / `windows.update`（読むだけ、その場で答える。`windows.update` は `focused: true` を受けて何もしない）と `tabs.create` / `update` / `remove` / `reload`（木を変える、下の列で実行してから答える。D-358）。要求はヘッダで運び、本文は読まない（D-353）。
  どのタブがその拡張のものかは、ビューが持つ controller で決め、controller をまだ言えないビュー（プロファイルが後から届く Edge）とビューの無いノードはディレクトリの言葉（`DirIsOfProfile`）で決める。その言葉は必要になったときに `ExtensionHostWire::TabIsTold` の中で尋ねる（D-473）。
  タブの `url` / `title` を渡すのは、拡張が `tabs` 権限を持つか、`host_permissions` がそのタブの住所に当たるとき（Chrome の規則。タブごと・答える時点の住所で。`ExtensionHostWire::Sight`、D-389）。
  host 権限の path は見ず、port つきなど読めない pattern は 1 つずつ捨て、`file:` は `tabs` 権限でしか見せない。拡張自身のページは見せる。イベントの `onUpdated` も同じ規則で、見えなくなった住所については何も言わない。`activeTab` の一時的な許可は無い。
  `search.query` は木を変える Act として同じ列に載り、実行時に Vanilla 自身の主検索エンジン（`Page::CreateQueryUrl`）で url を作って `Create` / `Update` に渡す。雛形に `%1` が無い、http / https でない、host が無い url は `ExtensionHostWire::WhyNotSearchable` が拒む（D-369）。
  `sessions.restore`（ごみ箱の先頭を `TreeBank::Restore` で現在の隣に戻す）、`tabs.duplicate`（`CloneViewNode`）、`tabs.move`（`index` は `tabs.query` の並びでの最終位置。同じ親なら `MoveChild`、別の親なら `MoveNode`）も同じ列の Act（D-371）。`tabs.getZoom` / `setZoom` は Act ではなく届いたところで、ノードの論理ズームを読み書きする（読む前に見えているビューだけ `SaveZoom`、書いたらビューの `RestoreZoom`。0 は 1.0、Chrome の範囲 0.25〜5。ビューがあれば、そのエンジンが見せられる範囲（`View::MinimumZoom` / `MaximumZoom`。D-429）に収めて書く。両エンジン。`LocalView` のノードではサムネイルの倍率。D-387）。
  `bookmarks.getTree` / `history.search` も読むだけでその場に答える（D-367）: このアプリにはブックマークも履歴も木のほかに無いので、**木そのもの**を答える。ブックマークはフォルダつきの木（葉は `tabs.query` と同じ規則・同じ集合、フォルダは自身の言葉で `NodeIsOfProfile` のときだけ。
  見せないフォルダの見せる子孫はいちばん近い見せる先祖の下へ、`ExtensionHostWire::Shown`）、履歴は同じ葉を最終アクセス（`View::OnFocusIn`）の新しい順に。manifest の `permissions` に `bookmarks` / `history` が無ければ「not available」。
  `bookmarks.get` / `getChildren` / `getSubTree` / `search` も同じ木で答える（`ExtensionHostWire::Bookmarks`。id は `"0"` = 根と見せるノードの番号だけ、Chrome の文言で拒む。`getSubTree` 以外は `children` を返さない。D-403）。
  `history.onVisited` / `onVisitRemoved` は `history` 権限の拡張にだけ、下の `vanilla.events` の差分（`Diff`）の末尾で届く（葉が現れた・url が変わった・見た = `onVisited`、どの葉も持たなくなった url = `onVisitRemoved`。日時は `history` の購読があるときだけ `Look` が写す。`bookmarks.on*` は鳴らない。D-403）。
  `fontSettings.getFontList` は `fontSettings` 権限の拡張に `QFontDatabase::families()` を `{fontId, displayName}` で答える（読むだけ。D-404）。
  `topSites.get` は `topSites` 権限の拡張に、履歴と同じ葉から http / https の住所を住所ごとに 1 件（題は最後に見た葉のもの）、最後に見た新しい順に 10 件 `{url, title}` で答える（訪問は数えない。Edge でもホストが答え、WebView2 の物は呼ばない。D-450a）。
  content script のメッセージがどのタブから来たかは、**ページ単位の `QWebEngineUrlRequestInterceptor`** が `vanilla-extension://` の要求へ
  プロセス乱数の MAC つきでビューを刻印し、content shim が `/bind` で置いた nonce を worker が `vanilla.tabOf` で本物の tabId に引く（D-356）。
  木を変える要求（`tabs.create` / `update` / `remove` / `reload`）は `requestStarted` では検査だけ行い、プロセスに 1 本の列（`ExtensionHostWire::OneAtATime`）へ積む。`TreeBank::MayChangeFromOutside()` が真のときに
  イベントループから 1 件ずつ実行し、実行のあとで答える。列が持つのは値だけで、ノード・ビュー・`TreeBank` は実行のとき番号から引き直す（D-358）。`TreeBank::ChangeScope` は木を変えてポンプし得る関数がスタックにある間を数え、
  拡張の列・`View::CloseLater`・`Page::Recreate`・`AutoLoad` がそれを見る（D-357）。
  ページの world へは何も入れず、DOM にも URL にも nonce は出ない。引けなければ文書ごとの乱数へ落ちる。
  worker から content script へは、ホストを通らず**エンジン自身のポート**で運ぶ（D-359）: content shim は `runtime.onMessage` のリスナーが足されたら、挨拶（`sendMessage`。眠っている worker を起こす）のあとで
  `runtime.connect` し、worker shim はその繋がりの表から `tabs.sendMessage` と `webNavigation.getAllFrames` に答える。見えている文書だけが 20 秒ごとに ping するので、その拡張のタブが見えている間は worker が眠らない。
  タブに起きたこと（`tabs.onCreated` / `onRemoved` / `onMoved` / `onUpdated`（`status: loading` / `complete` を含む）/ `onZoomChange` / `onActivated` / `onHighlighted`、`windows.onFocusChanged`。D-415）は、worker の呼び出し `vanilla.events` をホストが**答えずに保留**して届ける（D-360）。木は引数なしの
  `TreeBank::SomethingChanged()` で「何か変わった」とだけ言い、ホストは少しあと（イベントループから、誰も木を変えている最中でないときだけ）に `tabs.query` と同じ一覧を取り直して、購読ごとに最後に渡した写しとの差分
  （`ExtensionHostWire::Diff` / `Subscribers`）からイベントを作る。保留中の呼び出しがある間は、合図が無くても 2 秒ごとに同じことをする。保留に期限は無く、worker が止まれば job ごと消える。
  文書が源のイベント（`webNavigation.onHistoryStateUpdated` / `onReferenceFragmentUpdated`）は、content shim が isolated world の Navigation API（ページ本体の `pushState` も届く）で拾い、見えていて繋がっているときだけ
  同じポートで worker へ流す（100 ms 束ねて最後の url だけ。D-361）。`getAllFrames` の `url` もこれで新しくなる。
  `webNavigation.onCommitted` は、content の名乗りが `linked` を受けるまで付ける `fresh: 1` を worker が文書の最初の繋がりとして受け、`port.sender.url` で出す（繋がりの専用の枠で番号を待つ。出した nonce は 64 件覚えて重ねない。D-426）。
  `webNavigation.onDOMContentLoaded` / `onCompleted` は、見えている文書だけが `document.readyState` の段（1 = DOM、2 = 読み込み済み）を、名乗りの `ready` / `readyFrom` か繋がった後の `{ready, from}`（寿命で高々 2 通）で worker へ伝え、worker は伝えた段より上だけを `{nav}` と同じ溜めに受けた順で入れて出す（nonce ごとに出した段を 64 件覚える）。隠れたタブの分は見せたときに出る。`onTabReplaced` はリスナーを持つだけで鳴らない（Vanilla のタブの番号は差し替わらない）。D-452。
  拡張自身のページ（オプションなど `chrome-extension://<id>/…` の文書）には、コピーの `*.html` / `*.htm` の先頭（`ExtensionCopy::PlaceOfPageShim`）へ足した 1 行の `<script src="/vanilla_page_shim.js">` でページ用のシム（`Cdp::PageShim()`）が入る（D-363）。
  代役は `chrome` と、Chromium 148 以降のエンジンが持つもう 1 つのルート `browser` の両方に立てる（D-378 追記 3。Qt 6.11.2 の Chromium には `browser` が無い）。
  同じコピー・同じシムが両方のエンジンで動くので、シムは先頭でエンジンを判別する（`navigator.userAgentData.brands` に `Microsoft Edge WebView2` が完全一致で在るときだけ Edge。UA は Vanilla 自身が偽装するので見ない。D-379 追記 2）:
  Edge では WebView2 自身が持って動く `contextMenus` / `downloads` / `offscreen` / `i18n` / `storage`（`sync` と `onChanged`）を代役に立てず、`runtime.getContexts` も包まない。id を持つ名前空間（`tabs` / `windows` / `webNavigation` / `bookmarks` / `history` / `sessions` / `search`）と `action` は Edge でも代役。
  Edge の worker はホストへ `fetch` できない（測った）ので、コピーの中継ページ `vanilla_relay.html`（鍵入り。ページシムも封筒も無し）が代わりに尋ねる（D-379）:
  `EdgeExtensions` が鍵入りコピーの拡張ごとに 1 つ、見えない WebView2（`EdgeRelayView`。画面外の tool 窓、`IsVisible` 真、navigate は 1 回だけ）でそれを開き、
  中継ページが worker へ port `__vanilla_relay__` を張る。worker の入口 `vanilla_worker.js`（住所は固定: D-382）の 1 行目にはコピーの番号を書く（D-395。ただし WebView2 は残った登録の worker を中身が変わっても入れ替えないので、Edge は上の無効化→有効化で入れ直す: D-396）。worker のシムは問いを `{ticket, call}` で port に流し、中継ページが `/call` を fetch して `{ticket, answer}` で返す（同時 64 件、再送しない）。
  port は通信が無いと 30 秒で worker ごと切れる（測った）ので、中継ページは自分からは繋ぎ直さず、worker が起きたときに送る `{__vanillaRelay: 'wanted'}` で繋ぐ（保険は 5 分に 1 回）。
  port が切れたら中継ページは保留中の `vanilla.events` を abort して `vanilla.abort` をホストに送り、ホストは `Subscribers::Abort` で購読を残したまま保留を解く（worker 側は port の切断で保留を畳む。`{aborted: true}` の答えは防御）。中継ビューは Edge のタブを全部閉じても起動中ずっと残る。
  content shim は brand を読まず（安全でない文脈では `userAgentData` が無い）、i18n は `getMessage` の答え、`storage.sync` は最初の呼び出しの答えでエンジンか代役かを決める。
  ページのシムは 2 本（D-366）: 鍵入りの `vanilla_page_shim.js`（`tabs.*` / `windows.*` をホストに尋ねる）と、鍵なしの `vanilla_page_shim_open.js`（`web_accessible_resources` に当たる = Web が埋め込めるページが名指す）。WAR のパターンは字面だけで読む（D-366 追記 3）: 「文字どおりのパス」と「文字どおりのフォルダ + `/*`」の 2 形を読み、文字どおりのフォルダで始まるほかの形（`images/*.svg`）はそのフォルダの `/*` として読む（D-436）。鍵の入るファイル（worker のシム・ページのシム）に届く拡張と、先頭から読めない形（`*.js`、`~` や末尾のドットのある最初のフォルダなど。要求の綴りを見通せない）を持つ拡張は全部のシムが鍵なし。鍵なしにしたことは設定画面の拡張の行に出る（`ExtensionController::CopyNote`、D-398）。
  ページの `tabs.sendMessage` は `runtime.sendMessage({__vanillaRelay})` で worker のシムへ行き、worker が `sender.origin` が拡張の origin のときだけ自分の繋がりの表から送って答える（ページには繋がりの表が無い）。
  Web ページに埋め込まれた拡張のページ（vomnibar・HUD の iframe = `web_accessible_resources`）からの `runtime.sendMessage` にも、ページのシムが content shim と同じ封筒（`/bind` の nonce + `__vanillaEnvelope`）を付け、worker のシムがホストの刻印から `sender.tab` を付ける（D-370。Vimium の worker は `sender.tab` の無いメッセージを全部捨てる）。
  引けなかった拡張 origin の文書はタブ無し（乱数を作らない）で覚え、トップレベルのページ（ポップアップ）は 1 通目を待たせない。
  `chrome.i18n.getMessage` は、コピーがシムを運ぶ 4 ファイルの先頭に書く辞書（`ExtensionCopy::Messages`: `_locales` の default_locale ← 言語 ← ロケールの順で上書き、名前は小文字）から
  シムが Chrome の規則（大文字小文字を区別しない名前、placeholder、`escapeLt`、`$1`〜`$9`、10 個以上の substitutions は `undefined`、`$$`、`@@extension_id` ほか）で答える。manifest の `name` / `action.default_title` の `__MSG_…__` も同じ表（`ExtensionMessages::Messages`）で出現ごとに置き換える（`ExtensionManifest::Read`。D-404）。ロケールは `Stamp` に入る（D-373）。
  `chrome.extension`（`inIncognitoContext` / `isAllowedFileSchemeAccess` / `isAllowedIncognitoAccess` = no）は 3 文脈で代役（D-373）。
  ポップアップのビュー（`WidgetPopup` / `QuickPopup`）は表示スケールを `zoomFactor` で受け、右クリックは自前の小さいメニュー（編集・コピー・リンクのコピー・再読み込み。翻訳文脈 `ExtensionPopup`）（D-373）。
  ボタンのポップアップの窓は中身の大きさ（D-390）: ホストが開いてから 250 / 700 / 1500 / 3000 ms に `View::ExtensionPopupSizeJsCode` を引き（Qt は application world の `runJavaScript`、Edge は `ExecuteScript`）、文書の min-content の幅とその幅での根の高さ（25〜800 × 25〜600 CSS px、物理 px に切り上げ）にページのウィジェットを `setFixedSize` する。パネルの layout は `SetFixedSize` で追い、`resizeEvent` でボタンへ付け直す。Quick 根も同じ（QML の `runJavaScript(script, ApplicationWorld, callback)` を QML の関数から呼び、`measured(string)` で返す）。3 秒より後は、利用者の操作（ページのウィジェットの `MouseButtonRelease`（右ボタン以外）/ `KeyRelease`。Widget は `childEvent` で子（描画の子のほか、ページやメニューにも付く）に filter、Quick は override）の 100 / 500 ms 後に測り直す（D-413）。操作を伴わない変化は追わない。設定ページは固定の 450×520 のまま。
  **`permissions.request`**（D-417。Qt）: シムが manifest から答える（D-392 の `getAll` / `contains` と同じ部品）。すでに与えられたものは `true`、optional にだけあるものは `false`（許す仕組みが無い）、どちらにも無いものは Chrome の文言で拒む。
  **Edge のボタンの右クリック**（D-469）: `contextMenus` はエンジンのもの（D-379 追記 2 の決定 8）なので、worker のシム（`MENU_MIRROR`）がエンジンの受け入れた呼び出しをまとめて `vanilla.menuMirror` でホストの登録簿へ写し（ずれは許す）、Edge の Controller はそれをメニューのファイルに保存する。ボタンの右クリックはその写しを読み、ページのメニューは E-1 のまま（`View::AddExtensionMenu` は写しを読まない）。選んだら、ホストが拡張の relay ページで `runtime.sendMessage` を走らせ、寝ている worker も起きて `onClicked` に届く。
  **ボタン自身のメニューと `action.openPopup`**（D-419）: ピン留めのボタンの右クリック（と拡張の行の「…」）に、その拡張の `contexts` が `action` / `all` の項目（最上位 6 つまで、`ExtensionUi::Menus::ActionShown`）を、Vanilla の項目（設定・再試行・外す・ピン留め / 解除、区切りの下に拡張機能を管理）の前に並べる。先頭は Chrome と同じく拡張の名前で、`ExtensionManifest::homepage`（`homepage_url` が http(s) ならそれ、無ければ鍵のある拡張の `update_url` が Chrome / Edge のストアならそのストアの頁）を新しいタブで開く。行き先が無ければ灰色（D-466）。**Edge のビューでは拡張自身の項目は出ない**（`contextMenus` はエンジンのもので、ホストへ届かない。D-379 追記 2 の決定 8）。選ぶとページのメニューと同じ `ClickMenu`（文脈は `action`、`pageUrl` はボタンのビューの住所）。`action.openPopup` はホストが今のタブのノードの番号で `ActionCommand` を出し、ボタンの押下と同じ門で開く（ポップアップが無ければ Chrome の文言で拒む）。
  **`commands`**（D-416。Qt だけ）: manifest の `suggested_key` を Chrome の規則（`ExtensionUi::CommandsOf`）で読み、controller が鍵（Chrome の綴り）→ 拡張とコマンドの表を持つ（有効な拡張の設定の順で先の物）。`View::TriggerKeyEvent` がビューのキー割り当てに無い鍵を、application/keymap にも無ければ（`ExtensionUi::RouteKey`）拡張へ回し、当たった押下は食う。押下は Windows の仮想キーで綴る。`_execute_action` は同じビューのボタンの行がボタンの押下と同じ門で開く / 押す（`ExtensionBar::Execute`）、ほかは `commands.onCommand` を起きている worker へ。`commands.getAll` はホストが答える。
  **`runtime.onStartup`**（D-414。Qt のエンジンは鳴らさない）: controller が `Start` のとき有効な拡張をメモリの集合に持ち（実行中に外れた・無効になった・版が変わったものは落とし、戻さない）、worker のシムが `vanilla.startup` を 1 回尋ねる（`onInstalled` と同じ形・同じ門）。読み込みの始まりと終わりは `View::OnLoadStarted` / `OnLoadFinished` がプロセス全体の通し番号と `SomethingChanged` で知らせる（D-415）。
  **`runtime.onInstalled`**（D-399。Qt のエンジンは鳴らさない）: Qt の controller がプロファイルごとの台帳 `extension-installed-<digest>.json`（id → manifest の版 + 未配達の理由）を持ち、`Start` で決める（台帳に無い = `install`、版が違う = `update`、台帳のファイルが無い・読めない = 全部 `update`）。決めた拡張のメニューの登録簿は空にする。実行中は新しく登録された拡張だけを決め直す（版の変更は次の `Start`）。worker のシムは最初のリスナーの次のターンに `vanilla.installed` を尋ね、ホストは配達済みを書いてから答える。その起動で無効化→有効化を済ませていない拡張（D-397。worker は前の起動の物）には何も渡さない（`WorkerIsOfThisRun`）。Edge と鍵なしのコピーはエンジンの物のまま。
  **タブの id**（D-400）はノードの通し番号で、起動ごとに `StateDirectory()/serial-base` の基点から始まる（拡張が storage に残した id が次の起動の別のタブに当たらない）。
  **ボタンとメニュー**（S2b-11 / S2b-12、D-374）: `chrome.action.*`（`setIcon` の `path` / `imageData`、バッジの文字と色、`setTitle`、`enable` / `disable`、get 系、`getUserSettings`）はホストがその場で答え、状態は `ExtensionController` がメモリに持つ（純粋部 `src/core/extensionui.*`）。登録簿（`ExtensionUi::Menus`）の radio は Chromium の `MenuManager` と同じ規則で持つ: `update` で on にすると run の兄弟が off、`checked: false` は受けて何もしない、追加・削除・親の付け替えのあとは各 run を「最後に on だったもの、無ければ先頭」に正す、親の付け替えは兄弟の末尾へ、非 checkable にしても checked を見えないまま保つ（D-468）。
  タブごとの上書きは設定時の url と同じ間だけ有効（別の url で読まれたら消える）。バー（`ExtensionBar::Decorate`）はアイコン・バッジ・ツールチップ・enabled を描き、`ActionChanged` と現在ビューの `urlChanged` で描き直す。ポップアップの無いシム入りの拡張のボタンは押せて、`action.onClicked` が worker へ届く。
  `chrome.contextMenus`（`create` / `update` / `remove` / `removeAll`。manifest の `permissions` に要る）は controller の登録簿（拡張ごと 100 項目・深さ 8）へ入り、プロファイルごとの `extension-menus-<key>.json` に残る（Chrome の state store と同じ。登録を外した拡張の分は消す）。
  ページの右クリック（`Page::DisplayContextMenu` → `View::AddExtensionMenu`）は文脈（page / selection / link / image / video / editable、`documentUrlPatterns` / `targetUrlPatterns`）に合う項目を、ページ自身の項目の後・Inspect の前に出す（最上位 1 つならそのまま、2 つ以上は拡張名のサブメニュー）。
  選ぶと controller が checkbox / radio を Chrome の規則で変え、`WorkerEvent` → ホストの `Fire` が購読の列（`Subscribers::Fire`、購読ごと 32）に積み、次の `Publish` で差分の後ろに `contextMenus.onClicked(info, tab)` / `action.onClicked(tab)` として届く。購読の無い（寝ている）worker には届かない。
  **`scripting.executeScript`**（S2b-14、D-376。manifest に `scripting` 権限のある拡張だけ）: ホストは関わらず、worker が自分のファイルを `fetch` で読み（content からは WAR で読めない）、既存のポート（D-359）で `{run, codes}` として content shim へ渡し、content shim が isolated world で間接 eval して `{ran, value | error}` を返す。
  top だけ / `allFrames` / `frameIds`、`func` と `args`、複数フレームは成功分だけ、file の cache と上限は D-376。**isolated world の eval は自前ビルドの QtWebEngine のパッチ（`tools/patches/…isolated-world-unsafe-eval.patch`）が要る**: 公式のエンジンでは CSP で拒まれ call は失敗する。MAIN world は無い。
  **`scripting.registerContentScripts` / `unregisterContentScripts` / `getRegisteredContentScripts` / `insertCSS` / `removeCSS`**（A-32、D-410。`scripting` 権限のある拡張だけ。ホストは関わらない。MAIN world の登録だけは例外 = 下の D-420）: 登録は worker のシムがメモリに持ち（ファイルの本文は登録の時点で読む。worker が止まれば消える。uBOL は起き直しで登録し直す）、content shim が link したとき（`port.sender.url` に `matches` が当たり `excludeMatches` が当たらない ISOLATED の登録）と、登録が通ったときの繋がっている文書へ、`{run, codes, at, script}` で送る。文書側は走らせた `script` の id を寿命の間持ち、同じ登録を 2 度走らせない。`runAt` が `document_start` 以外なら `DOMContentLoaded` を待つ。CSS は `{style, add|remove}` で `adoptedStyleSheets` に入れる（author の sheet。`origin` は無視）。
  拡張の**ページ**の `scripting.executeScript` / `insertCSS` / `removeCSS`（Qt のページには無い）は、ページのシム（`PAGE_RELAY`）が `tabs.sendMessage` と同じ中継 `{__vanillaRelay: 1}` で worker へ渡し、worker の `relay` が自分の呼び出しで答える（D-422。注入のオブジェクト 1 つだけで、`func` は運ばない。uBOL のポップアップの「要素を削除」）。
  **MAIN world の登録**（`world: 'MAIN'`、uBOL の scriptlet。A-32、D-420）だけはホストへ行く: ページの script より先に走らせる口がエンジンのユーザースクリプトしか無いため。worker のシム（`ENVELOPE_WORKER_MAIN`）が MAIN の登録の一覧を**名前だけ**（`filePath` で正規化した相対パス、matches、excludeMatches、allFrames、runAt）で `vanilla.mainScripts`（本文で運ぶ）に送る。変わった次のターンに、登録の途中でなければ、1 つずつ、最新を最後に送り、一覧を変えた `register` はホストの答えを待って resolve する（uBOL の「全部取り消す → 登録し直す」は同じ一覧なので何も送らない）。
  ホスト（`ExtensionHost::MainScriptsCall`、`scripting` 権限が要る）は拡張が今動いているコピーのフォルダ（`ExtensionController::RunFolderOf`）から、正準パスがフォルダの中に留まる通常のファイルだけを読み、`ExtensionMainScripts`（`src/core`、純粋）がファイルごとに `QWebEngineScript`（`MainWorld`、`document_start` = `DocumentCreation`、`allFrames` = `runsOnSubFrames`）を作る。どこで走るかはメタデータの `@include` 1 行の正規表現 `^(?=matches)(?=host_permissions)(?!excludeMatches)` だけ（各パターンは URL 全体に固定、http/https だけ。`@match` は file / qrc も読むので書かない）。`ExtensionMainScripts::Table` が拡張ごとの持ち物を持ち、同じ一覧なら何もせず、違えば全部入れ替え、`Changed` で有効でない・権限の無い・フォルダの変わった拡張の分を外す。Widget は `QWebEngineProfile::scripts()`、Quick は `userScripts` の invokable を使う。Edge は `RunFolderOf` が空で何もしない（エンジンの `scripting`）。起動をまたいでは、ホストが `persistAcrossSessions` の分を名前とパターンだけで `extension-main-scripts-<digest>.json` に持ち（`ExtensionMainScripts::Store`、`QSaveFile`）、次の起動で `Ready` と `Changed` のたびに、表に無く `MainScriptsWanted` な拡張の分を今のコピーから読んで入れる（D-421）。版が違う・読めない保存は捨て、削除はコントローラの `Unlisted` で捨てる。シムの `getRegisteredContentScripts` は永続分を返さない。
  uBOL のように manifest に content_scripts が無い拡張のために、コピーは**運び手** `vanilla_carrier.js`（`self.__vanillaCarrier = 1` + content shim）を `content_scripts` の先頭に足す（`permissions` に `scripting` があり、`host_permissions` に content_scripts の `matches` として書ける pattern = `ExtensionCopy::CarriablePattern` があるとき。`all_frames`、`document_start`。`file:` は写さない）。運び手の文書は聞き手が無くても link し、最後のリスナーが外れても切らない。運び手は挨拶と名乗りに `carrier: 1` を添え、登録を受けない worker（WebView2、`scripting` 権限無し）が `carrying: 0` と答えたら止まって普通の文書になる。登録は全部送るか何も送らず、部屋が無くて送れなかった link は、待ちが 1 つ終わるたびに次のターンで送り直す（`starved`、64 link まで。空の link にも収まらない登録はその link には送らない）。`host_permissions` の path は `/*` と読む。`world: 'MAIN'` の登録は持って返すが送らない。`updateContentScripts` は代役のまま。
  **`chrome.offscreen`**（S2b-15、D-380。manifest に `offscreen` 権限のある拡張だけ）: `createDocument({url})` でホストがプロファイル上にビューの無い `QWebEnginePage`（`OffscreenPage`）を 1 拡張 1 つ作り、`chrome-extension://<id>/<url>` を読み込む（コピーの HTML なのでページのシムが入る）。call は読み込みが終わってから答え、失敗なら "Page failed to load." で文書は無し。
  url は拡張の中だけ（先頭 `/` と自 id の完全 URL は可、`..` は生でも `%2e%2e` でも拒む: `OffscreenUrlOf`）。隠しページはダイアログ・`window.open`・権限・全画面・ファイル選択・認証・証明書・media・WebAuthn を拒み、音は mute、top の遷移は自拡張の中だけ。
  worker のシムは、manifest の `permissions` に `offscreen` がある拡張だけ（無い拡張には `offscreen.*` を尋ねず、`getContexts` はエンジンのまま）、`runtime.getContexts` をエンジンの本物の上に包み（filter は Chrome の形で検証、エンジンとホストの失敗は call の失敗）、ホストが言う文書を `OFFSCREEN_DOCUMENT` として足して filter を全キーで絞る。文書を持つ拡張の origin の Web 通知はプロファイルの presenter が捨てる（隠しページの通知が出ない代わりに、その拡張の見えるページの通知も出ない: D-380 追記 2）。文書の下のフレームも自拡張の origin（と `about:` / `data:` / `blob:`）だけ。文書は `closeDocument`、拡張が wanted でなくなったとき（controller の `Changed` の場で）、controller の消滅、ホストの消滅、プロファイルの削除（`WrapProfile` の deleter がプロファイルの `deleteLater` の前に同期で消す）で閉じる。worker ↔ 文書の会話はエンジンの `runtime.sendMessage`。Quick 根のプロファイルには作れない（not available）。
  worker とキーのあるページのシムは `runtime.openOptionsPage` もエンジンの物（Qt では常に失敗する）の上に書き、manifest の `options_ui.page` / `options_page` を `tabs.create` としてホストへ聞く（新しいタブ。D-433）。
  `chrome.sidePanel`（D-440a、Qt の widgets のビュー）: 窓ごとの `SidePanels`（`src/ui/sidepanels.*`）が 2 つのドック（objectName `sidepanel` = プロファイルの全体のパネル、`sidepaneltab` = 今のタブの専用のパネル。中はそれぞれ `QStackedWidget`）に拡張のページを出し、両方あるときは窓のドックのタブで重なる。プロファイル（controller）ごとに全体のパネルを 1 枚、タブごとに専用のパネルを 1 枚（開いた窓に留まる）。出すものが無いドックは隠す（Inspector と同じ）。options / behavior は controller の `ExtensionUi::SidePanel`（持ち越さない）、`open` / `close` と `onOpened` / `onClosed` はホスト。ページは `View::CreateExtensionView(…, ExtensionSidePanelPage, …)`。Edge のビューでは、ページはポップアップと同じ `EdgeExtensionView` を「ドック用」で作る（隠れても WebView2 を閉じずに表示だけ止め、捨てる・窓を閉じる・終了するときに `View::SidePanelShutdownEvent` で閉じる: D-441）。
  `tabs.captureVisibleTab`（D-439、Qt・Edge）: ホストが今のタブのビューを `View::CaptureVisible()`（物理ピクセル）で撮り data URL で返す。門は `<all_urls>`（http / https / data / 自分のページ）・`*://*/*`（http / https）・`activeTab` の一時的な許可（`ExtensionHostWire::ActiveTabs`: ユーザーがボタン・拡張の右クリック項目・ショートカットでその拡張を呼んだタブの、その読み込みにだけ効く。`ExtensionHost::Invoked`）。
  `chrome.identity`（D-437、Qt だけ、manifest に `identity` がある拡張だけ）: `getRedirectURL` と「未サインインの Chrome」の答え（`getProfileUserInfo` は空、`getAuthToken` は失敗）はシムが答え、`launchWebAuthFlow` はホストがプロファイルのページ（対話型は `AuthWindow` の窓）を開いて、メインフレームが `https://<id>.chromiumapp.org/` へ向かう遷移（302 も）を止めてその URL で答える。1 拡張 1 フロー、非対話は最長 1 分、対話は 25 分。待つ間ワーカーのシムは 20 秒ごとにエンジンの `runtime.getPlatformInfo` を呼んで延命する（保留した fetch では延命しない）。
  `alarms.onAlarm`（D-442、Qt の worker だけ）: エンジンの `chrome.alarms` はアラームを持ち予定どおり消化するが `onAlarm` をどこにも届けないので、worker のシムがエンジンの `getAll` で表を取り直し（見直しは同時に 1 本、シム経由の `create` / `clear` / `clearAll` の答え待ちの間は捨てる）、期限の来たものを 1 回ずつ鳴らす。見直しは次の予定か 20 秒後のタイマーで、アラームとリスナーがある間は worker が寝ない（代償）。寝ていた間に過ぎた予定と拡張のページの `onAlarm` は鳴らない。Edge はエンジン自身が届ける。
  `idle.onStateChanged`（D-450b、Qt の worker だけ）: エンジンは `queryState` に答えるがイベントを届けないので、リスナーがある間だけ worker のシムが 1 秒ごとにエンジンの `queryState`（`setDetectionInterval` で最後に与えた間隔、15 秒〜4 時間に収める）を問い、前回と違う状態を鳴らす（初めは `active`）。問いは同時に 1 本で、10 秒で諦め、諦めた後の答えは捨てる。リスナーがある間は worker が寝ない（代償）。拡張のページの `onStateChanged` は鳴らない。Edge はエンジン自身が届ける。
  `management`（D-450a）: 読むもの（`get` / `getAll` / `getSelf` / `getPermissionWarnings*`）とイベントはエンジンの物のまま。変えるもの（`setEnabled` / `uninstall` / `uninstallSelf` / `launchApp` / `setLaunchType` / `createAppShortcut` / `generateAppForLink` / `installReplacementWebApp`）はどちらのエンジンにも渡さず代役が「not available」で失敗する（エンジンの `setEnabled` はコピーだけを止め、設定と食い違う）。Qt ではイベントが届かない。`power` はエンジンの物がそのまま効く（`requestKeepAwake('display')` を測った）。
  `chrome.userScripts`（D-443 段 1・D-444 段 2a、Qt だけ、manifest に `userScripts` がある拡張だけ）: `register` / `update` / `unregister` / `getScripts` をホストが答える。拡張ごとの登録の帳面（コードを含む。`file` は名前だけ）はホストが持ち、拡張の版と一緒にプロファイルごとのファイル（`extension-user-scripts-<digest>.json`）へ書き、版が変われば捨てる。帳面からプロファイルのユーザースクリプトを作り（`ExtensionUserScripts`、場所は D-420 の `@include` の 1 行に glob の先読みを足したもの、`file` は入れるたびにコピーから読む）、`MAIN` はページの world、`USER_SCRIPT` は（拡張, `worldId`）ごとの isolated world（128 から振り、使い回さない）で走る。`Ready` / `Changed` で入れ直すので、再起動の最初のページから走る。world の設定・メッセージ・`execute` は無い（凍結 = D-474）。
  寝ている worker を起こす（D-446、Qt の widgets のビュー）: 右クリックの項目・ボタン・ショートカット（`contextMenus.onClicked`・`action.onClicked`・`commands.onCommand`）を、聞いている購読の無い拡張へ届けるとき、ホストはその時点で組んだイベントを `ExtensionHostWire::Wakes` に待たせ（この起動で一度でも購読した拡張だけ、16 件・15 秒）、拡張の見えないページ（`OffscreenPage`）でコピーの `vanilla_wake.html` を開く。そのスクリプト（`Cdp::WakeScript`）がエンジンの `runtime.sendMessage({__vanillaWake: 1})` を送ると worker が起き（ブリッジがシムを入れる）、worker のシムはこの言葉を拡張のリスナーへ渡さない。最初に現れた購読に待たせたものを流し、ページを閉じる。10 秒で購読が現れなければ閉じて失敗と数え、3 回続けば 60 秒は起こさない。user script のメッセージ（D-447）も、名乗った受け手が居なければ、この起動で名乗ったことのある拡張に限り宛先未定の保留として 15 秒待たせて起こし、名乗っていて購読もある token に 1 度だけ渡す。タブ・履歴・ダウンロード（D-451）: worker のシムはイベントの問いに今リスナーを持つ名前を添え、その後に変わったら `vanilla.eventNames` で送り直す。`Subscribers` は拡張ごとに最後に伝えたタブの表と最新の購読の名前（`Last`）を持ち、保留している購読が無い拡張は `Publish` の前の `Watch` で `Last` からの差分を組み、聞いている名前があれば起こす（無ければ `Last` を進める）。起きた worker の新しい購読は、ほかに保留している購読が無ければ `Last` から始まり、寝ている間の差分を受ける。名前の一覧には番号が付き、ホストは新しいものだけを取る。続きから始めるとき、保留していないほかの行は外して token を退役させ（以後の問いには `stale`）、待ち行列を 1 つだけ（user script のメッセージを除いて）移す。ダウンロードのイベントは聞いている名前なら `Wakes` に待たせて起こす。起こせなかった（休み・起こしている最中）間だけ `LOOK_AGAIN` で見直す。
  user script のメッセージ（D-445 段 2b）: `configureWorld` / `getWorldConfigurations` / `resetWorldConfiguration` は帳面に world の設定として持つ（保存は登録の並びの末尾の `{"worlds"}` の要素）。messaging を有効にした world には、その world の user script より前に前置き（`Cdp::UserScriptPrelude`）が入り、`chrome.runtime.sendMessage` を作る。送り先は `vanilla-extension://host/userScriptMessage` で、world の秘密（`ExtensionHostWire::WorldSecret`、プロセスの乱数・プロファイル・拡張・コピー・world の HMAC）をヘッダーにだけ載せる。ホストは秘密で拡張と world を引き、送り元のタブは D-356 のビューの刻印から、origin は要求の initiator から取る。受け手は `onUserScriptMessage` に最初のリスナーを足した worker が名乗った購読（`vanilla.userScriptListen`）だけで、イベントのループに `[message, sender, 切符]` として届く。各受け手は `vanilla.userScriptReply` で 1 度だけ答え（値 / 答え無し / リスナー無し）、ホストの `UserScriptMessages` が最初の値を返す（5 分、1 拡張 64 通、worker が消えたら答え無し）。寝ている worker は D-447 で起こす。
  **`chrome.downloads`**（S2b-13、D-375 追記 2。manifest に `downloads` 権限のある拡張だけ）: `download({url, filename})` は `blob:`（origin を問わない）か `data:` だけを受け、ホストへ `downloads.expect` を尋ねる。ホストは期待表（`ExtensionUi::Downloads`: 拡張ごと 32・60 秒）に入れてから、プロファイル上のビューの無いページ（`ExtensionHost::Downloader`、`OffscreenPage` の `about:blank`）で `QWebEnginePage::download(url, filename)` を呼び、エンジンが blob を自分の store から読んで普通のダウンロードとして `downloadRequested` に届ける。ホストが先に見て（`page()` がその隠しページで url が期待に一致する要求だけを結ぶ。何もしない）、Vanilla の `HandleDownload`（ポリシー・保存先・名前）が accept / cancel し、ホストは要求が終わったときに読んで `downloads.onCreated` / `onChanged`（complete / interrupted + `USER_CANCELED` / `FILE_FAILED`）を `Fire` し、`search({id})` に filename（実パス）・mime・bytes・startTime / endTime で答える（id 指定以外は `[]`）。60 秒たっても届かない期待は interrupted（`FILE_FAILED`）として告げる。
  worker のシムは自分の `download` がホストの答えを待つ間、downloads のイベントを留めて答えの次のターンに渡す（拡張は答えから id を知る）。`cancel` / `pause` / `resume` / `erase` / `show` は、ホストが record の id で持つ要求の `QPointer` に対してその場で行う（Chrome の文言で拒む: `ExtensionUi::WhyNotDownloadAct`。`pause` / `resume` は要求が `DownloadInProgress` のときだけ。`erase` は進行中なら cancel してから record を消し `onErased`。`show` はフォルダを `QDesktopServices` で。`open` は出さない = 保存した実行ファイルを起動できるため、D-405 追記 2。Vanilla が名前を決めたとき `onChanged({filename})`、`pause` / `resume` で `paused` / `canResume`、中断の理由は `ExtensionUi::InterruptReasonOf` が Qt の番号を Chrome の語に。D-405）。ページのシムは関わらない（鍵はページに出ない: D-355）。Quick 根は not available。
  ページが書いた設定を worker が知るのは、見えている文書の ping（20 秒ごと + 見えるようになった直後）が届いたときで、worker のシムがそのついでに `storage.local` を見直して `onChanged` を告げる（2 秒に 1 回まで。タイマーは持たない。D-364）。manifest の `sandbox.pages` と、UTF-16 などの触れない形の HTML はバイト単位で元のまま。
  このエンジンは `web_accessible_resources` を一切通さない（Qt 側の欠落。D-363 の実測）ので、Web ページの上に拡張のページを置く機能（Vimium の vomnibar・HUD）は出ない。
  ホストもシムも答えない名前（`windows.create`、`webNavigation` の `onBeforeNavigate` / `onErrorOccurred` / `onCreatedNavigationTarget` ほか）は、シムが「利用できない」と失敗を返すか、鳴らないイベントを置くだけで中身はまだ無い（凍結 = D-388 / D-427 / D-474。設定画面の説明は一覧を持たない）
- **chrome 層 API**: `tabs` / `windows` / `webNavigation` / `scripting` / `contextMenus` / `notifications` / `action`。
  **素の** Qt は build の schema から除外されていて undefined、Edge は存在する（動くかは行ごと）。互換レイヤー（`network/@ExtensionShims`、既定 on）が入った Qt の拡張では、
  上の「拡張の互換レイヤー」の項に挙げた名前だけが中身を持ち、ほかは失敗する代役。`tabs` 権限の無い拡張ではエンジンが起動後に素の `chrome.tabs` を作り直すので、
  worker とページのシムは `self.chrome` を `get` だけの Proxy で包み、代役を立てた名前は代役を、ほかは元のオブジェクトを読ませる（D-368 追記 2）
- **現在タブ**: `tabs.query({active:true,currentWindow:true})` の答え。 Edge は manifest に `tabs` 権限か、元ビューの住所に当たる `host_permissions` が要る（D-389。鍵入りのコピーの拡張は、ページのシムがホストに尋ね、木の現在タブが返る。D-378）。同 URL のビューが複数なら空。`view-source:` と文字列 document は解決しない
- **Service Worker**: Qt でも動く。content script の `sendMessage` が眠っている worker を起こし、最後の活動から 30 秒で止まる（実測、D-359）。素のエンジンでは chrome 層 API が無いので、
  先頭でそれに触る worker（Vimium）は 1 行目で死ぬ。互換レイヤーの代役がそれを防ぎ、worker が担う機能は代役に中身がある分だけ動く。
  Edge は Stands 2.1.71 の広告遮断で確認（自作 fixture には worker が無い）
- **badge / title / アイコン**: `action.setBadgeText` / `setTitle` / `setIcon`。Edge は SDK 1.0.4129.50 に受け口が無く、
  manifest の `action.default_icon` を表示し続ける
  （Vimium は `setIcon` で青に替えるが灰色のまま、2026-09-14 ユーザー報告）。互換レイヤーの Qt はホストが受けてバーに描く（D-374）
- **context menu**: 互換レイヤーの Qt はページの右クリックに出す（D-374。`contexts: ["action"]` は出さない）
- **context menu**: Edge は `ContextMenuRequested` の `MenuItems` から `Name == "extension"` の項目（拡張ごとの submenu か 1 項目）だけを値で写して Vanilla のメニューに載せ、選ばれた `CommandId` を deferral 越しに `SelectedCommandId` で返す（D-377）。書き戻せたら、そのビューの serial と `PageUri` を拡張ホストの `MenuPicks` に積み（8 件、10 秒）、Edge の worker のシムはエンジンの `onClicked` を代役の前で受けて `vanilla.menuTab`（`info.pageUrl`）で 1 件取り出し、`tab` の写しの id を Vanilla の id に替えて渡す（引けなければ `-1`。D-385）。続く `scripting.executeScript` / `insertCSS` / `removeCSS` は Edge ではエンジンの物のまま（Edge の content script は文字列の評価を CSP で拒むのでシムの link 越しの実行は使えない）、シムが `target.tabId` を link の `port.sender.tab.id` でエンジンの id に替えて呼ぶ（link が無ければ呼ばずに失敗。D-386）。アイコンは出ない。確認は probe 拡張と build tree で、実拡張（SingleFile）の項目の実行は D-386 で確かめた。radio / checkbox の切り替えの実機は未確認。
- **`scripting`（Edge）**: worker の包み（D-386）が Vanilla の id をエンジンの id へ引き直す。link の無いタブ（uBOL の運び手は Edge では引っ込む）は、その場でエンジンの `tabs.query` の候補へエンジンの `tabs.sendMessage` で nonce を尋ね、host が `vanilla.tabOf` で同じタブと答えたものを使う（D-434。対は覚えない）。拡張のページの `scripting` は Edge でも worker へ中継する。`func` つきの `executeScript` だけは、鍵のあるページが同じ引き直しをしてエンジンの `scripting` を直接呼ぶ（D-435。呼んですぐ閉じるページでは途中で消える）
- **`chrome.notifications`**: 互換レイヤーの Qt はホストが Web の通知と同じモードレスダイアログで出す（題は `<拡張の名前>: <title>`、拡張ごと 3 つまで、「開く」で `onClicked`、閉じると `onClosed`。D-418b）。Edge の `NotificationReceived` が worker 発の通知で鳴るかは未計測
- **`commands`**: Edge はホスト側に受け口が無い（`AcceleratorKeyPressed` は拡張のコマンドを知らない）

拡張を実用する経路はEdge。Edge ビューも同じコピーを読み（D-378、E-2a）、拡張自身のページ（ポップアップ・オプション）のシムはホストに尋ねるが、worker もコピーの中継ページを通してホストに尋ねる（D-379、E-2b）。Qt2種は、素のエンジンでは登録とcontent scriptまでで、静的DNRの遮断はVanillaが肩代わりする（D-347）。
WebEngineビューは互換レイヤー（D-350〜D-373）で、Vimium のキー操作・タブ操作・リンクヒント・タブのイベント・SPA の見直し・オプションページの読み書きと保存・ポップアップ（D-363 / D-364 / D-366）・vomnibar の候補（D-367）・平文の検索（D-369）・拡張のページの文言（`i18n`、D-373）と、Stands の content script と worker のやりとりまでが動く（Edge ビューの無い環境のための経路。動く範囲は上の「拡張の互換レイヤー」の項）。`extension_smoke` のQt i18n XFAILは素のエンジン（fixture は `key` が無くコピーを作らない）の既知の欠落の検知器として維持する。

#### Chrome拡張 API の名前空間の分類（2026-09-27）

[Chrome の API リファレンス](https://developer.chrome.com/docs/extensions/reference/api)の 85 名前空間から、ChromeOS 専用 17・Dev チャンネル 2・型だけの 3（`events` / `types` / `extensionTypes`）を除いた **63** を母数に、互換レイヤーの Qt ビューで数える。
中身の有無はシムの `hosted` / `SOUNDED` / `OWN`（[src/core/cdpshims.cpp](../src/core/cdpshims.cpp)）と上の「拡張の互換レイヤー」の項による。
Edge は WebView2 自身が持つものが多く、名前空間ごとには測っていない。Qt のエンジンが素で持つものは 2026-09-26 に測った（D-442。探針 `a31-measure/gen_ns.py`）。

**中身がある（27）**
- ほぼ一通り: `action` / `contextMenus` / `commands` / `i18n` / `offscreen` / `declarativeNetRequest` / `storage` / `extension` / `sidePanel`（D-440a / D-440b / D-441）
- 一部だけ: `tabs` / `windows` / `bookmarks`（読むだけ）/ `history` / `sessions`（`restore` だけ）/ `search` / `fontSettings`（`getFontList` だけ）/ `topSites`（最後に見た順: D-450a）/ `idle`（エンジンの物に `onStateChanged` をシムが足す: D-450b）/ `downloads`（`open` 無し）/ `webNavigation`（`onCommitted` / `onDOMContentLoaded` / `onCompleted` / 文書の中の遷移の 2 つと `getAllFrames`。`onBeforeNavigate` / `onErrorOccurred` / `onCreatedNavigationTarget` と URL フィルタと `getFrame` は無い: D-452）/ `scripting`（MAIN の `executeScript` 無し）/ `permissions`（manifest から答えるだけ）/ `runtime`（大部分はエンジンの物）/ `identity`（`launchWebAuthFlow` と未サインインの答え: D-437）/ `alarms`（エンジンの物に `onAlarm` をシムが足す: D-442）/ `userScripts`（`register` / `update` / `unregister` / `getScripts`、world の設定と `sendMessage`: D-443〜D-445。`connect` と `execute` は無い）/ `notifications`（Qt はモードレスダイアログ: D-418a / D-418b。Edge は未計測）。残りは凍結（D-388・D-474）
- 寝ている worker は、右クリック・ボタン・ショートカット・user script のメッセージ・タブ / 履歴 / ダウンロードのイベントで起こす（D-446 / D-447 / D-451。Qt の widgets のビューだけ）

**Qt のエンジンが素で持つ（6）**: `management`（変えるものは断る。イベントは届かない: D-450a）/ `power`（効く: D-450）/ `system.cpu` / `system.memory` / `system.display` / `system.storage`（manifest に権限があれば。呼び出しは答える。`system.*` のイベントが届くかは未測定。`alarms`・`idle`・`management` はエンジンのイベントが届かなかった: D-442・D-450）

未実装の 30 を、方針で 4 つに分ける。

| 分類 | 名前空間 | 理由 |
|---|---|---|
| やる意味がある | `omnibox` / `tts` / `publicSuffix` | ホストか Qt の部品で答えられ、バックエンドに依らない。`omnibox` はアドレス欄のキーワード、`tts` は QTextToSpeech（追加モジュール）が要る |
| 思想にそぐわない | `tabGroups` / `readingList` / `privacy` / `contentSettings` / `declarativeContent` | 木がブックマークと履歴を兼ねる（D-367）ところへ別の構造や保管庫を持ち込む、または Vanilla の設定を拡張が書き換える。`declarativeContent` は MV3 では `action.enable` / `disable` で足りる |
| 複数バックエンドなので触れない | `cookies` / `browsingData` / `webRequest` / `proxy` / `debugger` / `pageCapture` / `tabCapture` / `desktopCapture` / `devtools.*`（5）/ `dom` | 持ち主がエンジンごとにいる（cookie jar のミラーをやらない D-077 と同じ理由）。Edge では WebView2 自身のものが動き得る |
| 前提が無い | `gcm` / `instanceID` / `enterprise.hardwarePlatform` / `accessibilityFeatures` / `mimeHandler` / `webAuthenticationProxy` / `printerProvider` / `ttsEngine` | Google のサービス・Chrome 内部・特定の用途が前提（`identity` は未サインインとして答えて D-437 で入れた） |

着手は「やる意味がある」の需要の多い順（`omnibox` …）。エンジンが素で持つもののイベントが届くかも、要る拡張が出たら測る。

---

（Deodorized. This tree is published with its source comments
and internal annotations stripped; the diff against the original
is [deodorant.zip](deodorant.zip).）

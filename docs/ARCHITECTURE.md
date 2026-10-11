# アーキテクチャ

最終更新: 2026-10-03

vanilla の**いまの構造の地図**。何がどのファイルにあり、どう繋がっているかだけを書く。

- なぜそうなったか・測った事実・境界条件は  の D 番号にある。
  各節の終わりの「詳細:」がその入口
- 触る前の注意と再現手順は HANDOVER.md、残りの仕事は ROADMAP.md
- 拡張 API ごとの対応状況は [EXTENSIONS.md](EXTENSIONS.md)

コードは大きなファイルに集中している（`src/ui/treebar.cpp`、`src/gadgets/graphicstableview.cpp`、
`src/ui/treebank.cpp`、`src/view/view.cpp` はどれも 100KB を超える）。ここで当たりを付けてから読むこと。

---

## 全体像

```
main.cpp
  └─ Application                アプリ全体。設定・永続化・グローバル状態
       └─ MainWindow            ウィンドウ。メニューと周辺ウィジェット
            ├─ TreeBar          ツリー型タブのバー
            ├─ ToolBar          ツールバー（アドレスバーと拡張のボタン）
            ·  （NodePreview     タブに乗せたとき出るサムネイル。アプリで1つの独立ウィンドウ）
            └─ TreeBank         ★中枢。ツリー状態を保持し、View を作る
                 ├─ Node ツリー （ViewNode / LocalNode）
                 ├─ View        表示中のページ（エンジン別の実装）
                 ├─ Gadgets     サムネイル一覧（俯瞰）UI
                 ├─ Receiver    コマンド / URL / 検索の入力欄
                 └─ Notifier    ステータス表示
```

---

## ディレクトリ構成

ソースは `src/` の下に**責務ごと**に置く。

| どこ | 何が | 目印 |
|---|---|---|
| [src/core/](../src/core/) | `Application` にも GUI にも依存しない層 | テストが直接呼べる。外の機能は `Hooks` で受け取る |
| [src/app/](../src/app/) | 起動、グローバル状態、プロセスの外との口 | `main.cpp` / `Application` / `Receiver` / `NetworkController` |
| [src/input/](../src/input/) | アクション名の表と JS 公開口 | 割り当ての読み書きは [src/core/inputmap.hpp](../src/core/inputmap.hpp) |
| [src/ui/](../src/ui/) | ウィジェットと描画 | `MainWindow` / `TreeBank` / `TreeBar` / `Theme` |
| [src/view/](../src/view/) | 表示エンジンごとの `View` 実装 | `webengine/`、`edge/`、Native、Local |
| [src/gadgets/](../src/gadgets/) | サムネイル一覧（俯瞰 UI） | `QGraphicsScene` 側 |

- `#include` は**ファイル名だけ**で書く。各ディレクトリがインクルードパスに載っているので、
  ファイルを別のディレクトリへ移しても `#include` は書き換えない
- リポジトリのルートもインクルードパスにある（`#include <VERSION>` のため）
- テストのために切り出したコードは `src/core/` に来る。この層は増えていく
- テストは [tests/](../tests/)、リソースは [resources/](../resources/) と [qrc/](../qrc/)、
  翻訳は [translations/](../translations/)。ビルド成果物はリポジトリの外（[BUILD.md](BUILD.md)）

詳細: D-033、D-034、D-295

---

## 実行時のデータの置き場

根は2つある。アプリ自身の状態は `Application::BaseDirectory()` の下、
エンジンのプロファイルは**エンジンが名前から決める場所**にある。

| 何 | どこ |
|---|---|
| アプリの状態 | `<Base>/data/<MD5(実行ディレクトリ)>/`（`Application::StateDirectory()`） |
| 一時ファイル | `<Base>/temp/`（起動時に空にする） |
| WebEngine のプロファイル | `<AppDataLocation>/QtWebEngine/<MD5(実行パス + "/" + id)>` |
| WebEngine のキャッシュ | `<Base>/data/webenginecache/<同じMD5>` |
| WebView2 のプロファイル | `<Base>/data/edgewebview/EBWebView/WV2Profile_<同じMD5>` |
| QML ビューのプロファイル | `<Base>/data/webengine/<MD5(実行パス + "/quick:" + id)>` |
| Edge の拡張の登録の記録 | `<Base>/data/extension-registry/<SHA256>.json` |

`BaseDirectory()` は**実行ファイルの隣**。ただし `C:/Windows/` と `C:/Program Files/` の下では
`<AppLocalDataLocation>`（`%LOCALAPPDATA%/vanilla/`）になる。つまりインストール版の `data/` は
`%LOCALAPPDATA%` に、持ち運び用では実行ファイルの隣にある。
その場所はアプリ名だけで決まるので、状態はもう一段下の「実行ディレクトリの MD5」に分けて置く。

`StateDirectory()` の中身:

| ファイル / ディレクトリ | 何 |
|---|---|
| `config.json` | 設定 |
| `main_tree.json` / `trash_tree.json` | タブツリーとゴミ箱 |
| `cookie.json` | `QNetworkAccessManager` 側のクッキー（エンジンのものではない） |
| `icondata.json` | ファビコン |
| `<日時>-*.json` | 上の起動ごとのバックアップ（`Application::BackUp`） |
| `image/` / `history/` | ノードのサムネイルと履歴 |
| `extension-*.json`、`extension-copies/`、`serial-base` | 拡張の状態（「拡張機能」の節） |
| `password.json` | 旧版の残り。読み書きしない |

- Windows では `AppDataLocation` は Roaming、`AppLocalDataLocation` は Local。インストール版の状態は
  Local（`data/`）と Roaming（WebEngine のプロファイル）に分かれる。ローミングプロファイルの環境は未確認
- 古い配置（`<AppDataLocation>/QtWebEngine/<生の id>` など）は今のバイナリが読まないので消してよい
- `<AppLocalDataLocation>/vanilla/WebView2/` は QuickNative のビュー、`cache/qmlcache` などは Qt 自身が作る

詳細: D-044、D-045、D-098〜D-100、D-141、D-258

---

## 保存ファイルの形

保存ファイルはすべて JSON。0.2.2 以前の XML は読まない。

### 設定（`config.json` / `icondata.json`）

キーを入れ子にせず、`"group/subgroup/key"` のフラットなオブジェクトにする。

```json
{
    "application/@AutoSaveInterval": 300000,
    "application/@AllowedCertificates": ["example.com:<leaf SHA-256>"],
    "application/keymap/Ctrl+W": "Close",
    "mainwindow/geometry24387": {"@rect": [341, 192, 1365, 768]},
    "mainwindow/toolbar24387": {"@variant": "AAAADAAAAACA..."}
}
```

- JSON にない型は型名1つのオブジェクトにする（`@url` / `@size` / `@point` / `@rect` / `@color` など）。
  それ以外は `QDataStream` + Base64 の `{"@variant": ...}`
- **型名を間違えても落ちず、その設定が黙って既定値に戻る。** [tests/tst_settingsio.cpp](../tests/tst_settingsio.cpp) が型名を固定している
- 読み書きは [settingsio.cpp](../src/core/settingsio.cpp)（`Application` に依存しない。外の処理は `SettingsIO::Hooks`）
- 読むときは「本体 → `.prev` → バックアップ（新しい順）」の順に試す。書くときは `~name` に書いてから差し替える（`FileExchange`）

### セッション（`main_tree.json` / `trash_tree.json`）

`children` の入れ子だけでできている。フォルダは `children` を持ち、タブは `holdview: true` で URL などを持つ。

```json
{
"children": [{
"primary": false, "holdview": false, "folded": true, "title": "folder",
"children": [{
  "primary": true, "holdview": true, "title": "Google",
  "index": 1, "url": "https://www.google.com/", "scrollx": 0, "scrolly": 0, "zoom": 1,
  "history": "{uuid}.dat", "thumb": "{uuid}.jpg"
  }]
}]
}
```

- `index` は保存時にそのノードを表示していたウィンドウの id（0 = どこにも表示していない）
- 読み書きは [treeserializer.cpp](../src/core/treeserializer.cpp)。木を丸ごと `QJsonDocument` に持たず、ストリームで直接読み書きする
- `TreeSerializer` は `ViewNode` 以外に依存しない。ウィンドウの操作は `TreeSerializer::Hooks`（実体は `treebank.cpp` の `TreeHooks()`）

### `cookie.json`

id ごとに、生のクッキー文字列の配列を持つ: `{"root": ["name=value; expires=...; domain=..."], "trash": []}`

### ブックマークの入出力（セッションとは別）

[bookmarkio.cpp](../src/core/bookmarkio.cpp)。形式は XML / HTML のまま。

| 形式 | 読む | 書く |
|---|---|---|
| 内部形式（`<viewnode>` の XML） | ○ | ○ |
| XBEL | ○ | ○ |
| NETSCAPE HTML | ○ | ○ |
| Chrome の `Bookmarks`（Edge / Opera / Vivaldi も同じ形） | ○ | — |
| Firefox の `bookmarks-*.json` | ○ | — |
| IE の `Favorites` ディレクトリ | ○ | — |

URL はパーセントエンコードして書く。`Application::Import` / `Export` にあるのはダイアログと形式の選択だけ。

詳細: D-008、D-011、D-030、D-031、D-136、D-137、D-144、D-298

---

## コアのクラス

### `Application` — [application.cpp](../src/app/application.cpp)

`QApplication` の派生で、アプリのグローバル状態をすべて持つ。

- 設定の読み書き（型は `QMap<QString, QVariant>`。`QSettings` そのものは使っていない）
- 保存先ディレクトリの決定、翻訳、ブックマークの入出力、証明書の判断、アイコンのデータベース
- `BootApplication()` が起動時の初期化をまとめて行う
- 主窓は最初のフレームを描いてから見せる（`MainWindow::ShowWithFirstFrame`。`show()` の間 DWM の cloak で隠す）。PurgeView とユーザー操作の窓は作った時点で見せる。詳細: D-513
- 前の回の窓（設定の `mainwindow/<名前><id>`）は、最初の WebEngine プロファイルと木の読みより前に空の殻として作って見せる。木の読みは同じ id の殻に current を付け、木に無い id の殻は読みのあと消す。殻の間（`Application::IsRestoringWindows()`）は主窓への入力と閉じる操作を受けない。詳細: D-514

### `TreeBank` — [treebank.cpp](../src/ui/treebank.cpp)

**このアプリの中枢。** ツリーの状態を持ち、ノードと View の対応を管理する。

- ルートは静的メンバ `m_ViewRoot` / `m_TrashRoot`、現在位置は `m_CurrentViewNode`
- 戻る / 進むはビューに任せる（`View::CanGoBack` など）
- ノードから View を作る。既定は `EdgeWebView`（`EDGEWEBVIEW` のあるビルドで WebView2 ランタイムがあるとき）、
  それ以外は `WebEngineView`。ディレクトリの設定で他の実装を選べる
- 木を変えている最中かどうかを `TreeBank::ChangeScope` が数える。外から木を変える処理（拡張など）は
  `MayChangeFromOutside()` を見てから動く
- 木から出ていくノード（ゴミ箱の切り詰め・download carrier）はその場で解放しない。`Discard` で削除箱へ入れ、
  1 turn あとの関門で全部の窓（タブバー・ギャラリー）が手放してから、保存中でなければ解放する（詳細: D-512）

詳細: D-160、D-357、D-454

### `Node` 系 — [lightnode.hpp](../src/core/lightnode.hpp)

| クラス | 役割 |
|---|---|
| `ViewNode` | ツリーのノード。`HoldsView()` が真ならタブ、偽ならフォルダ |
| `LocalNode` | ローカルファイル |

`ViewNode` はタブ1つ分のデータ（URL、タイトル、サムネイル、履歴、スクロール位置、ズーム）を自分で持つ。
サムネイルと履歴は保存時に遅れて書き出し（`SaveImageIfNeed()` / `SaveHistoryIfNeed()`）、
必要になったときに読み戻す。履歴の中身は書いたビューの種類で違う（先頭 4 バイトで見分ける）。
ツリーの形は `lightnode.hpp` の冒頭コメントに図がある。

### `View` — [view.hpp](../src/view/view.hpp)

表示エンジンを抽象化するインターフェース。実装は5つ。

| 実装 | 有効条件 | 何 |
|---|---|---|
| `EdgeWebView` | `EDGEWEBVIEW` | `ICoreWebView2` を自前で持つ。Windows x64 の既定。ソースは `src/view/edge/`（一覧は `edgewebview_p.hpp` の冒頭） |
| `WebEngineView` | `WEBENGINEVIEW` | `QWebEngineView`。Windows 以外の既定。ソースは `src/view/webengine/` |
| `QuickWebEngineView` | `WEBENGINEVIEW` | QML の WebEngine（[quickwebengineview6.qml](../src/view/webengine/quickwebengineview6.qml)）。widgets 版の不具合からの逃げ場 |
| `QuickNativeWebView` | `NATIVEWEBVIEW` | Qt WebView。Windows では WebView2 だが、QML の API しか使えない |
| `LocalView` | `LOCALVIEW` | ローカルファイルの閲覧。`GraphicsTableView` と `View` の多重継承 |

- `EdgeWebView` のページは DirectComposition のビジュアルに描かれ、マウスはこちらが `SendMouseInput` で渡す
- [page.hpp](../src/view/page.hpp) の `Page` はエンジンに依らないページの共通処理
- **描画プロセスが落ちたら**、報せて読み直す。`RenderProcessLedger` が回数を数え、10 回目で読み直しをやめる
- ページの右クリックの既定のメニュー（`View::LoadSettings` の RegularMenu）は「新しいタブ」から始まる。
  ブックマークレットの項目は既定から外してある（コマンドからは使える）

詳細: D-024、D-061、D-070、D-166、D-168、D-190、D-249、D-253

### ビューの実装ごとの機能

`QuickNative` と `Edge` はどちらも Windows では WebView2 だが別物。前者は Qt WebView の小さな QML API 越しで、
後者は WebView2 の API を直に使う。

○ = 実機で確認 / △ = 配線はあるが実機未確認 / × = 未対応 / ― = 土台の API が無い / 無効 = 意図して切ってある。
`JS` は注入した JavaScript で実装しているもの。`※` は表の下に注がある。

| 機能 | WebEngine | QuickWebEngine | QuickNative | Edge |
|---|---|---|---|---|
| 描画・サムネイル | ○ | ○ | ○ | ○ |
| 俯瞰（Gadgets）との共存 | ○ | ○ | ○※ | ○※ |
| Notifier / Receiver との共存 | ○ | ○ | ○※ | ○※ |
| プロファイルの分離 | ○ | ○ | ― | ○ |
| Private | ○ | ○ | ― | ○ |
| Cookie の永続 | ○ | ○ | ○ | ○ |
| セッションクッキーの永続 | ○ | ○ | ― | ×※ |
| QNAM の jar への写し | ○ | ○ | ― | ○ |
| セッション履歴の保存復元 | ○ | △※ | △※ | ○※ |
| 戻る / 進む / 巻き戻し | ○ | ○ | ○ | ○ |
| スクロール・ズームの保存復元 | ○ | ○ | ○ | ○ |
| ページ内検索 | ○ | ○ | ○ JS | ○ JS |
| 編集（コピー・貼り付けなど） | ○ | ○ | ○ JS | ○ JS |
| 書式の編集（15本） | ○ | ○ | △ JS | ○ JS |
| 証明書エラーの続行 | ○ | ○ | ― | ○ |
| 通知の表示 | ○ | ○ | ― | ○ |
| 画面共有の共有先選択 | △ | △ | ― | × |
| WebAuthn / パスキー | ○※ | ○※ | ― | × |
| パスワードの保存 | ○※ | ○※ | ― | ○ |
| 権限（クリップボードの読み取り） | ○ | ○ | ― | ○ |
| 権限（local fonts） | ○ | ○ | ― | ○※ |
| 権限の永続 | ○ | ○ | ― | ○ |
| 閲覧データの消去 | ○ | △※ | × | ○ |
| リクエストのブロック / DNT | ○ | ○ | ― | ○※ |
| スペルチェック | △ | △ | ― | × |
| タブの休止 | ○ | △ | ― | ○※ |
| POST での遷移 | △ | × | ― | △ |
| キャッシュを無視した再読み込み | ○ | ○ | × | ○※ |
| 独自の右クリックメニュー | ○ | ○ | × | ○ |
| キーマップ | ○ | ○ | × | ○※ |
| マウスジェスチャ | ○ | ○ | × | ○ |
| 入力欄へのドロップ | ○ | ○ | × | ○ |
| ページ内のキーのフィルタ | ○ | ○ | ― | ○※ |
| パスワードマネージャ | 無効 | 無効 | ― | 無効 |
| 音の検知・ミュート | ○ | ○ | ― | ○ |
| アクセスキー | ○ | ○ | × | ○ |
| ダウンロード | ○ | ○ | × | ○ |
| ディレクトリの設定 | ○ | ○ | ○※ | ○※ |
| UA / AcceptLanguage | ○ | ○ | ○※ | ○ |
| `vanilla://` のページ | ○ | ○ | ―※ | ○ |
| ソース表示（view-source:） | ○ | ○ | ○ | ○ |
| 文字列の文書（`setHtml`） | △ | △ | △ | ○ |
| Inspector（ウィンドウ内にも） | ○ | ○ | × | ○ |
| 動画の再生位置 | ○ | ○ | ○ | ○ |
| 印刷 | ○ | ○ | ×※ | ○ |
| ページからの `window.print()` | ○ | △ | ― | ○ |
| フォントの設定（11件） | ○ | × | × | ×※ |
| 全画面 | ○ | ○ | × | ○ |
| 新しい窓 | ○ | ○ | × | ○ |
| ページから見える `chrome.webview` | ― | ― | ― | 隠す |

注:

- **俯瞰・Notifier / Receiver との共存**: QuickNative と Edge のページはネイティブ窓で、兄弟ウィジェットより上に来る。
  俯瞰は疑似的に重ね、Notifier と Receiver は独立したウィンドウとして出す（`View::ForbidToOverlap()`）
- **セッションクッキーの永続**: WebView2 に API が無い。Edge では期限の無いクッキーは再起動で消える
- **セッション履歴**: QuickWebEngine は保存だけ。QuickNative と Edge は URL だけ
- **WebAuthn / パスキー**: Windows では OS の「Windows セキュリティ」の窓が出て、vanilla のアカウント選択・PIN の窓は出ない。
  vanilla の窓（Windows 以外）は実機未確認
- **パスワードの保存**: WebEngine と QuickWebEngine はメインフレームのフォームだけで、保存先は OS の保管庫（QtKeychain）。入れるのは、欄のクリックか ↓ キーで出るメニューでアカウントを選んだときだけ。
  保存したものは設定画面の「保存したパスワード」で一覧し、1 件ずつか全部を消す（Edge の分は一覧に出ず、全部消すときに消える）
- **local fonts**: Edge の `file://` のページでは、再読み込みで許可が「確認する」に戻る
- **閲覧データの消去**: QuickWebEngine は訪問済みリンクだけ消せない（Qt に API が無い）
- **ブロック / DNT**: Edge は iframe の文書を見逃す
- **タブの休止**: Edge は凍結（Frozen）だけで、破棄（Discarded）は無い
- **キャッシュを無視した再読み込み**: Edge はヘッダだけで、次の1回だけ
- **キーマップ・ページ内のキー**: Edge は許可リスト制で、トップ文書だけ
- **ディレクトリの設定・UA**: QuickNative は一部（UA は効く）。Edge は JS とエラーページの設定だけ
- **`vanilla://`**: QuickNative では開けず、代わりに WebEngine が開く
- **印刷**: QuickNative は PDF の書き出しだけ
- **ダウンロード**: どの実装も履歴（`DownloadHistory`、新しい 500 件）に記録し、ツールバーのボタンの吹き出し（`DownloadPanel`。ツールバーが無いときは窓の右上）と設定画面の「ダウンロード」がそれを見せる。Notifier には出さない。ボタンは進行中・終わってから 1 分・吹き出しが開いている間だけ出す（詳細: D-506、D-519）
- **フォントの設定**: Edge には設定そのものが無い
- QuickNative の × の多くは Qt WebView の QML API が小さいため。`runJavaScript` はあるので、JS で書けるものは動く。
  ページの上の入力はネイティブ窓が先に取るので、API があっても届かない

詳細: 各行の D 番号は D-070〜D-091、D-160〜D-202、D-245〜D-279、D-302、D-332、D-339。
Edge の × が「API が無い」ことは D-196、QuickWebEngine の残る × は D-200 / D-201。

### `Gadgets` / `GraphicsTableView` — [src/gadgets/](../src/gadgets/)

タブとディレクトリを一覧して編集する UI（俯瞰）。`QGraphicsScene` の上に作る。

- `GraphicsTableView` が一覧の本体。`DisplayType` で ViewTree / TrashTree / LocalFolderTree などを切り替える
- 表示の種類は「現在のディレクトリ」「配下すべて（全展開）」「配下すべて（展開可）」の3つ（D-497 追記 4）
- 個々の項目は `AbstractNodeItem` / `Thumbnail` / `NodeTitle`
- 描画は `GadgetsStyle`。`GlassStyle`（暗い半透明、既定）と `FlatStyle`（明るい）の2つ
- 表示中のタブは淡い背景とアクセントの線で示す。折りたたみの開閉は 200ms で補間する

詳細: D-300〜D-305、D-308、D-338

### 入力とアクション

| ファイル | 役割 |
|---|---|
| [actionmapper.hpp](../src/input/actionmapper.hpp) | アクション名の一覧と、公開 JS API の名前の対応表 |
| [keymap.hpp](../src/input/keymap.hpp) | キー → アクション名の既定。修飾キーの無い単独キーは設定 `webview/@EnableSingleKeyShortcut`（既定オフ）のときだけ効く |
| [mousemap.hpp](../src/input/mousemap.hpp) | マウスジェスチャ → アクション名の既定 |
| [commandmap.hpp](../src/core/commandmap.hpp) | コマンドの綴り → アクション名。綴りの揺れはここにだけある |
| [receiver.hpp](../src/app/receiver.hpp) | 入力欄。Command / Query / UrlEdit / Search の4モード |
| [jsobject.hpp](../src/input/jsobject.hpp) | ページの JavaScript から呼べる API（`_Vanilla` / `_View`） |

アクション名・入力の綴り・公開 JS 名の3系統は互換性のために残る。API 名の全体は `tst_commandmap` が固定する。

詳細: D-054、D-123、D-295、D-430

### その他

| クラス | 役割 |
|---|---|
| `NetworkController` — [networkcontroller.hpp](../src/app/networkcontroller.hpp) | `NetworkAccessManager` の生成、Cookie、プロキシ、認証 |
| `Saver` — [saver.hpp](../src/app/saver.hpp) | 自動保存。GUI のスレッドで値を写し、別スレッドで順に書く |
| `Transmitter` — [transmitter.hpp](../src/app/transmitter.hpp) | 二重起動のとき、既にあるプロセスへ引数を渡して終了する |
| `UserAgent` — [useragent.hpp](../src/core/useragent.hpp) | 名乗るブラウザの表と、その設定の読み書き |
| `DownloadName` — [downloadname.hpp](../src/core/downloadname.hpp) | ダウンロード先のファイル名を作る規則 |
| `WindowLedger` — [windowledger.hpp](../src/core/windowledger.hpp) | どのウィンドウがあり、今どれかを持つ表 |
| `Notifier` — [notifier.hpp](../src/ui/notifier.hpp) | ステータスと進捗の表示（ダウンロードは D-506 から吹き出しへ） |
| `DownloadBar` / `DownloadPanel` — [downloadbar.hpp](../src/ui/downloadbar.hpp) | ツールバーのダウンロードのボタンと、その吹き出し |
| `Dialog` — [dialog.hpp](../src/ui/dialog.hpp) | 自前で描くダイアログ。モードレスは窓の右に積む別窓 |
| `EditMenu` — [editmenu.hpp](../src/ui/editmenu.hpp) | 入力欄（アドレス欄・コマンドライン・ダイアログ）のコンテキストメニュー。ページの文字メニューと同じ項目と言葉（D-522） |
| `MiniMap` — [minimap.hpp](../src/ui/minimap.hpp) | ページの脇の帯。注入した JS が集めた要素の矩形を図にする。文書が動かず中の要素がスクロールするページはその要素を描く（D-485）。設定 `application/@EnableMiniMap`（既定オフ） |

`Receiver` のローカルサーバ（パイプのコマンドの受け口）は同じユーザーだけが接続でき、1フレームは 1 MiB まで。

詳細: D-056、D-058、D-145、D-209〜D-238、D-264、D-279、D-287〜D-293、D-309

---

## 設定

設定は `group/subgroup/key` → 値の平らな表（`Application` の `Settings`）で、`config.json` に書かれる。
読む側は `s.value("application/@EnableAutoSave", true)` の形。

### 設定画面（`vanilla://settings`）

設定画面はタブで開く**ページ**。

| 何 | どこ |
|---|---|
| スキームのハンドラ（ページと JSON API） | [settingspage.hpp](../src/app/settingspage.hpp) |
| 何を描くかの表（型・ラベル・分類・既定値・選択肢） | [settingsschema.cpp](../src/core/settingsschema.cpp) |
| 選択肢の表示名（Choice の値 → 画面の言葉） | 同じファイルの `ChoiceLabels()`。無い選択肢は値の綴りのまま出る |
| キー・マウス・ジェスチャの表 | [inputmapschema.hpp](../src/core/inputmapschema.hpp) |
| ページ本体 | [resources/settings/](../resources/settings/) |

**設定を1つ画面に出すには、`settingsschema.cpp` の表に1行足すだけ。** HTML も JS も触らない。
行は表の順に出るので、同じ話題の行の隣に足す。

- **既定値は、そのキーを読むコードが `s.value` に渡す値と同じにする。** `tst_settingsschema` がソースと照合する
- 書けるのは表にあるキーだけ。型と選択肢も検査する。よそのページからは API に届かない
- 開きかたは `settings` コマンド、メニューの「設定」、拡張の一覧の「拡張機能を管理」、アドレスバーへの入力。
  前の3つは `TreeBank::OpenSettings(category)` で同じ1つのタブを使う
- 表示中のカテゴリは URL のフラグメント（`vanilla://settings/#network`）で持つ
- `vanilla://` のページは、ディレクトリが QuickNative を指していても `WebEngineView` が描く

**画面の文字列**: ページは `QTranslator` に触れないので、文字列は全部訳された状態で C++ から届ける
（ラベルは表の `QT_TRANSLATE_NOOP("SettingsSchema", ...)`、ページ自身の言葉は `SettingsSchema::PageStrings()`）。
**`QT_TRANSLATE_NOOP` をマクロで短くしないこと。** lupdate はマクロを展開せず、拾われなかった文字列は黙って英語のままになる。

詳細: D-020、D-218、D-431、D-455、D-462、D-475

### ディレクトリの設定（設定画面の「ディレクトリ」）

ディレクトリノードの名前は `名前;トークン;トークン` で、`;` の後ろが**そのディレクトリの下に効く設定**になる
（`ID` / `Private` / `!Javascript` / `RightGesture` / `Encoding EUC-JP` など）。`;` の後ろは画面に出さない。

継承の規則:

| どこ | どちらが勝つか |
|---|---|
| ディレクトリの間 | 近いほう。何も言っていない設定は、さらに上から降りてくる |
| 同じ名前の中の `X;!X` | 否定 |

- 上で設定を変えると、下でそれを打ち消す語を外す（`LetDescendantsFollow`）。上の変更は下まで行き渡る
- `ID` だけは継承でなく**ネットワークプロファイルの境目の印**で、中にもう1つ境目を作れる
- `Private` はビューごとに、自分のノードの継承した言葉で決める
- 自動読み込みは既定でしない。`!noload` を書いたディレクトリだけが起動時に裏で読み込む。タイマーの 1 回で歩くのは 1,000 ノードまで（詳細: D-510）
- 読む側はどれも `DirectoryPage::StateIn` に訊く（`1` / `0` / `-1`）

設定を書く口は2つで、実装は [directorypage.hpp](../src/app/directorypage.hpp) の1つ。

| 口 | 何 |
|---|---|
| 設定画面の「ディレクトリ」 | ディレクトリの木全体から 1 つを選び、そのディレクトリと祖先の設定を並べる。開くと、メニューや俯瞰から頼んだディレクトリか、設定のタブがいるディレクトリ。`vanilla://directory` は置き換えで ここへ来る |
| ノードの右クリック | TreeBar と俯瞰の「ディレクトリの設定」サブメニュー |

- 編集はすべてリネーム（`TreeBank::ReconfigureDirectory`）を通るので、ネットワークプロファイルの付け替えも同じ経路で働く
- ノードのリネームの欄は名前だけを見せ、元のトークンを付け直す。`;` は打てない
- ルートも編集できる（`application/@RootName` に残る）。ゴミ箱は編集できない
- 分解・合成・継承は純粋な関数で、`tst_directorypage` が固定する

詳細: D-047、D-048、D-078、D-116〜D-122、D-310、D-311、D-428

---

## アドレスバーに打った文字

判定は2段ある。**スキームを足すときは両方を見ること。**

| 関数 | 何を決めるか |
|---|---|
| [page.cpp](../src/view/page.cpp) の `ExtractUrlsFromText` | URL か検索語か。載っていないスキームは黙って検索される |
| [toolbar.cpp](../src/ui/toolbar.cpp) の `IsUrlToLoadInPlace` | 今のタブで開くか、`Receiver` を通して新しいタブで開くか |

`tst_inputmapping` の `tellsAUrlFromSomethingToSearchFor` が、どちらに転ぶべきかを固定している。

---

## ユーザーエージェント

既定は**エンジン自身の UA**。別のブラウザを名乗るのは、ディレクトリに `;useragent chrome` のように書いたときだけ。

- 文字列は [const.hpp](../src/core/const.hpp)。`NetworkAccessManager::SetUserAgent` が `%SYSTEM%` / `%LOCATION%` /
  `%CHROMIUM%`（積んでいる Chromium のメジャー版）を埋める
- **Chromium の版を直書きしないこと。** 版は `qWebEngineChromiumVersion()` から取る
- Google は「Chrome を名乗る Chrome でないブラウザ」のログインを拒むので、偽装は常用しない

詳細: D-058、D-101、D-102

---

## エンジンの起動時の設定

### Chromium のスイッチ

`chrome://flags` は無い（Chrome のブラウザ層の機能のため）。代わりに設定 `application/@ChromiumFlags`
（`--` で始まる語を空白か改行で区切る）を、`Application::ApplyChromiumFlags()` が環境変数 `QTWEBENGINE_CHROMIUM_FLAGS` へ、
Edge には `edgeenvironment.cpp` が `AdditionalBrowserArguments` として渡す。

- 反映は次の起動から。元から環境変数があれば、その後ろに足す
- エンジンが受け付けないスイッチを書くと起動しなくなりうる。戻すには `config.json` を手で直す
- リモートデバッグは通常は開かない。検証のときだけ `QTWEBENGINE_REMOTE_DEBUGGING` を外から設定する

### 描画 API

設定 `application/@GraphicsApi`（既定 `Auto`）を `Application::ApplyGraphicsApi()` が `QQuickWindow::setGraphicsApi` に渡す。
WebEngine のビューは中身が QQuickWindow なので、これが Web ビューの描画 API になる。

- 最初の QQuickWindow より前、`BootApplication` が設定を読んだ直後に呼ぶ
- 環境変数 `QSG_RHI_BACKEND` があれば設定は無視する。その OS で動かない API は `Auto` 扱い
- 主窓を最初から RHI にするのは `application/@EnableMainWindowRhi`（既定オフ）のときだけ

詳細: D-142、D-340〜D-343、D-453、D-458、D-463

---

## 外部コマンド（「別のブラウザで開く」）

個別のブラウザの決め打ちは無い。

| 何 | どこ |
|---|---|
| 既定のブラウザで開く | `Application::OpenUrlWithDefaultBrowser()`（`QDesktopServices::openUrl()`） |
| ユーザー定義のコマンド | 設定 `application/@ExternalCommands`。1行が `name = コマンドライン`、URL は `%u` の位置 |

ユーザー定義のコマンドは個数が変わるのでアクションにしていない。メニューを組むところで `QAction` を作り、
コマンド名を `EXTERNAL_COMMAND_PROPERTY` に載せて1つのスロットへ流す。そのメニューを組むのは
`View::AddExternalCommandActions`、`Gadgets::CreateNodeMenu`、TreeBar の `NodeItem`。**経路を足すならこの形に倣う。**

詳細: D-004、D-021

---

## 色とアイコン

描画に使う色は **[theme.cpp](../src/ui/theme.cpp) の1つの表にしかない**。呼び出し側は用途の名前で引く。

```cpp
painter->setBrush(Theme::Brush(Theme::BarBackground));
painter->setPen(Theme::Pen(Theme::GlassText));
```

- **他の場所に色のリテラルを書かないこと。** 名前は色ではなく用途（`BarButtonPressed`）
- 関数ローカルの `static const QBrush` に入れないこと（実行中に配色を変えられなくなる）
- 名前を足したら Light と Dark の両方の表と `tst_theme` に足す
- 配色は `application/@ColorScheme`（`Auto` / `Light` / `Dark`）。`Auto` はデスクトップに追従する
- 色を保持するもの（TreeBar の背景、`LineEdit` のスタイルシート、ページの背景色、`ExtensionBar` など）は
  `Application::UpdateAllWidgets()` が配り直す。**保持するものを作ったらここに足す**
- スタイルシートの色は `#RRGGBB` で書く。解釈できない色は黙って捨てられる

アイコン:

- `resources/` のビットマップは黒インクで描いてある。アプリが描く面に載せるものは `Theme::Pixmap(path, role)` を通す
  （Dark で塗り直す）。明るいチップの上に載るものは通さない
- 各ビットマップの隣に `@2x.png` がある。表示スケールが 100% を超えると、それを縮めて使う
- 線画のアイコンは [scripts/draw_icons.py](../scripts/draw_icons.py) が作る。PNG ではなくスクリプトを直す

詳細: D-017〜D-019、D-039、D-113、D-131

---

## 重なり順（z-order）

2つの系統があり、どちらも [const.hpp](../src/core/const.hpp) に定数がある。

1. **`QGraphicsScene` の z 値**（俯瞰と LocalView の中身）。`HIDDEN_CONTENTS_LAYER` (-10) から `MULTIMEDIA_LAYER` (60) まで
2. **`QWidget` の重なり**。順序は親の子リストの並びだけなので、序列は
   [`TreeBank::RestackChildWidgets`](../src/ui/treebank.cpp) の1か所が持つ

   | 定数 | 誰 |
   |---|---|
   | `SCENE_WIDGET_LAYER` (0) | グラフィクスビュー（見せる物が無いとき） |
   | `VIEW_WIDGET_LAYER` (10) | ウェブビュー |
   | `COVERING_SCENE_WIDGET_LAYER` (20) | グラフィクスビュー（俯瞰を載せているとき） |
   | `NOTIFIER_WIDGET_LAYER` (30) | Notifier |
   | `RECEIVER_WIDGET_LAYER` (40) | Receiver |

**ウィジェットを重ねるなら、まず 1 に載せられないか考える。** ウィジェットで重ねるなら `const.hpp` に層を足して
`RestackChildWidgets` に1行足す。**そこ以外に `raise()` を書かない。**
窓どうしの順序（タイトルバー、ダイアログ、切り離した Inspector）はこの序列とは別の話。

詳細: D-092、D-306、D-307

---

## ビルドの構成

- `main.cpp` 以外のソースは静的ライブラリ **`vanilla_core`** にまとまり、実行ファイル `vanilla` はそれをリンクする。
  テストも同じライブラリをリンクするので、実行ファイルと同じオブジェクトを検証できる
- インクルードパスと機能のマクロ（`WEBENGINEVIEW` / `LOCALVIEW` / `NATIVEWEBVIEW` / `EDGEWEBVIEW`）は
  `vanilla_core` の PUBLIC な要件。マクロでヘッダの中身が変わるため
- `.qrc` は実行ファイルの側に置く。テストはリソースを持たない（要るテストはそのターゲットに `.qrc` を足す）
- [switch.hpp](../src/core/switch.hpp) の機能スイッチで残っているのは `USE_WEBCHANNEL`（無効）だけ

---

## 拡張機能

Chrome の拡張（Manifest V3）を、展開済みのフォルダーから読み込む。Chrome の登録元は変更・コピー・削除しない。
対象は `EdgeWebView`、`WebEngineView`、`QuickWebEngineView` で、private のプロファイルには入れない。
API ごとの対応状況は [EXTENSIONS.md](EXTENSIONS.md)。

エンジンによって、拡張 API の出どころが違う。

- **Edge**: API の大部分は WebView2 自身のもの。vanilla が答えるのはタブ・ウィンドウ・ブックマーク・履歴など
  アプリの木に関わる名前と、アプリの画面に出るもの（ボタン、サイドパネル）
- **Qt WebEngine**: エンジンが持つ API が少ない。vanilla は拡張の**コピー**にシム（JavaScript の代役）を入れ、
  足りない API をシムとホスト（C++）で埋める（設定 `network/@ExtensionShims`、既定オン）

### 部品

| 部品 | どこ | 役割 |
|---|---|---|
| `ExtensionController` | [src/app/extensioncontroller.cpp](../src/app/extensioncontroller.cpp) | プロファイルごとの拡張の状態。設定 `network/@Extensions` / `@DisabledExtensions` / `@PinnedExtensions` と照合し、非同期の操作を1列に並べる |
| エンジンの橋渡し | [webengineextensions.cpp](../src/view/webengine/webengineextensions.cpp)、[edgeextensions.cpp](../src/view/edge/edgeextensions.cpp) | エンジンへの登録・有効化・一覧。Edge は中継のビューも持つ |
| `ExtensionCopy` | [src/core/extensioncopy.cpp](../src/core/extensioncopy.cpp) | 拡張のコピーを作り、シムを入れる |
| シム | [src/core/cdpshims.cpp](../src/core/cdpshims.cpp) | worker 用・content script 用・拡張のページ用の JavaScript |
| `ExtensionHost` | [src/app/extensionhost.cpp](../src/app/extensionhost.cpp) | シムからの問いに答える C++ の側 |
| `ExtensionHostWire` | [src/core/extensionhostwire.cpp](../src/core/extensionhostwire.cpp) | ホストの純粋な規則: 呼び元の確認、タブを見せるか、イベントの差分、待ち行列 |
| `ExtensionUi` | [src/core/extensionui.cpp](../src/core/extensionui.cpp) | ボタンの状態、右クリックの項目、ショートカット、ダウンロードの純粋な部分 |
| `ExtensionMainScripts` / `ExtensionUserScripts` | [src/core/](../src/core/) | MAIN world の登録と `chrome.userScripts` を、エンジンのユーザースクリプトにする |
| `ExtensionBar` | [src/ui/extensionbar.cpp](../src/ui/extensionbar.cpp) | アドレスバー右の一覧のボタンとピン留めしたボタン、ポップアップ |
| `SidePanels` | [src/ui/sidepanels.cpp](../src/ui/sidepanels.cpp) | `chrome.sidePanel` のドック |

### 読み込みの流れ

1. 登録のキーは登録したフォルダーのパス。Chrome の `Extensions/<id>/<版>` を登録していて Chrome が版を上げたら、隣の最新版を読む
2. `ExtensionCopy` が `StateDirectory()/extension-copies/<id>/` にコピーを作り、次を足す:
   - worker の入口 `vanilla_worker.js`（worker のシム + 元の worker）
   - 各 HTML の先頭に `<script src="/vanilla_page_shim.js">`（拡張のページのシム）
   - content script のシム。uBOL のように content script を持たない拡張には運び手 `vanilla_carrier.js`
   - Edge 用の中継ページ `vanilla_relay.html`、寝ている worker を起こす `vanilla_wake.html`
   - `_locales` から作った `i18n` の辞書
3. エンジンへの登録は、そのプロファイルで**最初のビューがページを読もうとしたとき**（`ExtensionController::Start()`）。
   `Ready` を待ってからページを読む
4. コピーが変わった起動では、拡張を無効化 → 有効化して worker を入れ直す（エンジンは古い登録の worker を使い続けるため）
5. Edge にはコピーそのものでなく junction `extension-copies/<id>/edge` を渡す。コピーが変わっても登録を外さないので storage が残る

シムの一部はホストに尋ねるための鍵を持つ。`web_accessible_resources`（Web ページが読める拡張のファイル）が鍵の入るファイルに届く拡張、
またはその範囲を読み切れない拡張では、全部のシムを鍵なしにする。鍵なしにしたことは設定画面の拡張の行に出る。

### シムとホストのやり取り

```
 拡張の worker ──(vanilla-extension://host/...)──> ExtensionHost ──> TreeBank など
   │  ↑                                              ↑
   │  └ エンジン自身のポート                          │ Edge は worker から fetch できないので
   │                                                 │ 中継ページ（見えない WebView2）が代わりに尋ねる
 content script のシム ── 名乗り（/bind の nonce）──┘
```

- **シムがホストに尋ねる口**はカスタムスキーム `vanilla-extension`。要求はヘッダで運ぶ（大きい引数だけ本文）
- **木を変える要求**（`tabs.create` / `update` / `remove` など）は、その場では検査だけして1本の列（`OneAtATime`）に積む。
  誰も木を変えていないとき（`TreeBank::MayChangeFromOutside()`）にイベントループから1件ずつ実行し、そのあとで答える
- **読むだけの要求**（`tabs.query`、`bookmarks.getTree`、`history.search` など）はその場で答える。
  このアプリにはブックマークも履歴も木のほかに無いので、どちらも**タブの木そのもの**を答える
- **タブの `url` / `title` を見せる**のは、拡張が `tabs` 権限を持つか、`host_permissions` がそのタブの住所に当たるとき（`ExtensionHostWire::Sight`）
- **どのタブからのメッセージか**: ページごとのリクエストインターセプタが `vanilla-extension://` の要求にビューの印を付け、
  content script のシムが置いた nonce を worker が `vanilla.tabOf` でタブの id に引く
- **worker と content script の間**はホストを通らず、エンジン自身のポート（`runtime.connect`）で運ぶ。
  見えている文書が 20 秒ごとに ping するので、その拡張のタブが見えている間は worker が眠らない
- **イベント**（`tabs.onUpdated` など）: worker の `vanilla.events` をホストが答えずに保留する。木が変わったら
  `TreeBank::SomethingChanged()` で合図し、ホストがタブの一覧を取り直して、前に渡した写しとの差分（`Diff`）をイベントにして返す
- **寝ている worker**: 購読の無い拡張へボタンや右クリックのイベントを届けるときは、見えないページで
  `vanilla_wake.html` を開いて worker を起こし、現れた購読に渡す（`Wakes`）
- **Edge の中継**: Edge の worker はホストへ `fetch` できないので、拡張ごとに見えない WebView2（`EdgeRelayView`）が
  中継ページを開き、ポート `__vanilla_relay__` で worker の問いを受けて代わりに尋ねる
- シムは先頭でエンジンを判別し（`navigator.userAgentData.brands`）、Edge ではエンジン自身が持つ API に代役を立てない

### ホストが持つ状態

| 状態 | どこ |
|---|---|
| 静的 DNR の選択・動的ルール | `extension-rules-<digest>.json`（session のルールはメモリだけ） |
| 右クリックの項目の登録 | `extension-menus-<digest>.json` |
| `runtime.onInstalled` を配ったか | `extension-installed-<digest>.json` |
| MAIN world の登録（永続のもの） | `extension-main-scripts-<digest>.json` |
| `chrome.userScripts` の登録 | `extension-user-scripts-<digest>.json` |
| タブの id の基点 | `serial-base`（タブの id は起動ごとに新しい範囲から振る） |
| Edge のコピーの台帳 | `<Base>/data/extension-registry/<hash>-copies.json` |
| ボタンの状態、サイドパネル、ダウンロードの期待表 | メモリだけ（`ExtensionController` / `ExtensionUi`） |

`<digest>` はプロファイルのキー（`NetworkController::ProfileKey`）の SHA-256 の先頭 24 桁。ファイルは `StateDirectory()` の下にある。

### エンジンの外で肩代わりしているもの

- **静的 DNR**（Qt）: エンジンが評価しないので、vanilla のリクエストインターセプタが評価する（`Dnr::Rules` / `ExtensionNetRules`）。Edge は WebView2 が評価する。redirect はその拡張のホスト権限の内側だけに効く（詳細: D-499）
- **見えないページ**（`OffscreenPage`）: `chrome.offscreen` の文書、`chrome.downloads` のダウンロード、worker を起こすページに使う
- **MAIN world のスクリプト**: ページより先に走らせる口がエンジンのユーザースクリプトしか無いので、ホストがコピーのファイルを読んで `QWebEngineScript` にする
- **ポップアップの大きさ**: ホストが中身の大きさを測って窓を合わせる（`View::ExtensionPopupSizeJsCode`）
- isolated world での `scripting.executeScript` は、自前ビルドの QtWebEngine のパッチが要る（公式のエンジンでは CSP で拒まれる）

### UI

- アドレスバーの右に一覧のボタン（パズル）とピン留めしたボタンを置く（`ExtensionBar`）。寸法は `const.hpp` の `EXTENSION_*`
- 一覧の「追加」はフォルダー選択を Chrome の `Default/Extensions` から開く（`ExtensionController::DefaultPickDirectory()`）
- ポップアップは Qt では `WidgetPopup` / `QuickPopup`、Edge では `EdgeExtensionView`。プロファイルやビューが変わると閉じる
- オプションページは一覧からはポップアップと同じ窓で開き、拡張が `runtime.openOptionsPage` を呼んだときは新しいタブで開く。
  ポップアップの `window.open` は、`tabs.create` と同じ検査を通ったものだけ新しいタブで開く
- ボタンの右クリックには、拡張の `action` の項目と vanilla の項目（設定・外す・ピン留めなど）を並べる

詳細: 全体の決定は D-313〜D-315、互換レイヤーは D-350〜D-373、Edge は D-377〜D-379、
読み込みと worker は D-382・D-396・D-397、凍結した名前と残りは D-474、公開の表は D-476。
API ごとの D 番号は、[EXTENSIONS.md](EXTENSIONS.md) の名前空間の名前で引く。
メソッド単位の表は `tst_cdpshims::theTableOfWhatIsAnsweredIsTheShimsOwn` がシムから作る
（シムの名前を変えたら `VANILLA_WRITE_COVERAGE=1` で走らせて書き直す）。

---

（Deodorized. This tree is published with its source comments
and internal annotations stripped; the diff against the original
is [deodorant.zip](deodorant.zip).）

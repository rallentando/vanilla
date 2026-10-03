# Vanilla

ツリー型タブを中心に据えた、Qt 製のデスクトップ Web ブラウザ。

タブを一次元の並びではなく**木構造**として持ち、リンクから開いたページを親の子として繋いでいく。
サムネイル一覧、キーボードとマウスジェスチャによる操作、コマンド入力を組み合わせて、
大量のページを開いたまま構造を保って扱うことを狙っている。

## 主な特徴

- **ツリー型タブ** — ページの親子関係を保持したまま開閉・移動・折りたたみができる
- **サムネイル一覧** — 木構造をサムネイルのテーブルとして俯瞰し、そのまま整理できる（`Gadgets` / `GraphicsTableView`）
- **ディレクトリ固有の設定** — フォルダ名にトークンを書くと、その配下だけプロファイル分離・JavaScript 無効・エンジン切り替えなどが効く。設定はツリーを下へ継承される（`vanilla://directory`）
- **複数のレンダリングエンジン** — Qt WebEngine（既定）のほか、Windows では **Edge WebView2 を直接ホストするビュー**（`EdgeWebView`）をディレクトリ単位で選べる。ほかに QML 版 WebEngine と Qt WebView 経由の実装がある
- **ミニマップ** — ページの構造（テキスト行・画像・コントロール）を図式化した縮小スクロールバー（VSCode 方式、`MiniMap`）
- **キーボード操作とマウスジェスチャ** — 大半の操作にキーとジェスチャを割り当て可能（`src/input/keymap.hpp` / `src/input/mousemap.hpp`）
- **コマンド入力** — URL / 検索 / コマンドを1つの入力欄から受け付ける（`Receiver`）
- **ローカルファイル閲覧** — 画像・動画をブラウザ内で一覧・再生できる（`LocalView`）
- **Light / Dark 配色** — デスクトップのテーマに自動追従（固定も可）
- **日本語 / 英語** の UI 翻訳を同梱

## 動作環境

| 項目 | 内容 |
|---|---|
| Qt | **6.11（Windows）と 6.10（Linux）で動作確認済み**（CMake の要求は 6.5 以上）。Qt5 では動かない |
| コンパイラ | **MSVC 2022（Windows）** と **GCC 15（Linux）** で確認済み。macOS はまだビルドしていない |
| 必須 Qt モジュール | Core, Gui, Widgets, Network, Xml, OpenGL(+Widgets), WebChannel, Multimedia(+Widgets), Qml, Quick(+Widgets)。ビルド時のみ LinguistTools |
| 任意 Qt モジュール | **WebEngine**（Windows 以外の既定のビュー。無いとページを表示できるビューが無くなる）, PrintSupport, WebView（`QuickNativeWebView` が有効になる） |
| 任意 SDK | `third_party/webview2` に WebView2 SDK があると、Windows x64 で `EdgeWebView` が有効になり、既定のビューになる（WebView2 ランタイムが無ければ WebEngine。D-454） |

ビルドシステムは CMake。
手順とテストの回し方は [docs/BUILD.md](docs/BUILD.md) を参照。

## ドキュメント

- [docs/BUILD.md](docs/BUILD.md) — ビルド手順・テスト・配布物の作り方
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — 主要クラスと責務、データの置き場
- [docs/EXTENSIONS.md](docs/EXTENSIONS.md) — Chrome 拡張の対応状況（名前空間ごとの分類と、メソッド単位の表）

## 配布物について

- 配布は [GitHub の Releases](https://github.com/rallentando/vanilla/releases)。zip 形式の
  ポータブル構成で、設定とセッションは実行ファイルの隣の `data/` に作られる
- 同梱している Qt / Qt WebEngine / Edge WebView2 ローダのライセンス通知は zip の `licenses/` にある
- 実行ファイルは**コード署名をしていない**。初回起動時に Windows の SmartScreen が
  警告を出すことがある（「詳細情報」→「実行」で起動できる）。
  配布 zip の SHA-256 をリリースノートで公開するので、照合のうえ実行してほしい

### 画面がちらつく場合

ページに黒い小さなボックスが並んだり、画面の半分が一瞬黒くなったりするときは、
Chromium が描いたフレームを Qt Quick が受け取る段の同期が GPU やドライバと噛み合っていない。
設定画面（`vanilla://settings`）の「描画とエンジン」にある **「描画 API (WebEngineView)」** を
`OpenGL` に変えて再起動すると、多くの環境で収まる。既定の `Auto` は Qt に任せる
（Windows は Direct3D 11、macOS は Metal）。選べるのは `Software` / `OpenGL` / `Direct3D11` /
`Direct3D12` / `Vulkan` / `Metal` で、その OS で動かないものを選んだときは無視して `Auto` と同じになる。
環境変数 `QSG_RHI_BACKEND` を設定して起動した場合はそちらが優先される。

### Qt WebEngine へのパッチ（拡張の `web_accessible_resources`）

Qt WebEngine（6.11.2 と、Qt 6.12.0 と組む 6.140.0 で確認）は、Web ページから拡張のリソース（manifest の `web_accessible_resources`）を
読む要求を必ず `chrome-extension://invalid/` に書き換える（Qt が自前の判定表を持ち、そこに誰も書かない）。
このため、Vimium の vomnibar（`o` / `b` / `T`）・HUD・ページの上のヘルプなど、拡張が Web ページの中に
自分のページを iframe で置く機能が動かない。**配布物にはこのパッチを当てた Qt WebEngine を入れていない。**
必要なら自分でビルドした Qt WebEngine に当てる。

パッチは tools/patches/ の 3 本（Qt WebEngine 6.140.0 と 6.11.2 の Sources にそのまま当たる）:

- `qtwebengine-6.140.0-web-accessible-resources.patch`: `src/core/renderer/` の 3 ファイル。Qt 側の `WillSendRequest` が
  Chromium 本体の判定を呼ぶようにする（上の `web_accessible_resources`）
- `qtwebengine-6.140.0-isolated-world-unsafe-eval.patch`: MV3 の content script の world で eval を許す。
  Qt のビューの `scripting.executeScript` に関数を渡す拡張（SingleFile など）に要る
- `qtwebengine-6.140.0-ime-to-active-widget.patch`: IME の入力を別プロセスの iframe（vomnibar など）へも送る

用意と適用:

```powershell
# 1. Qt の Maintenance Tool で Qt WebEngine の Sources を入れる（例: C:\Qt\6.12.0\Src\qtwebengine）
# 2. 当てる（Git for Windows の patch。-p1 は先頭の a/ b/ を落とす。--binary は改行を書き換えないため。PowerShell の < は使えないので -i で渡す）
$patch = "$env:ProgramFiles\Git\usr\bin\patch.exe"   # Git for Windows に入っている patch（PATH には無い）
foreach ($p in Get-ChildItem tools\patches\*.patch) { & $patch --binary -p1 -d C:\Qt\6.12.0\Src\qtwebengine -i $p.FullName }
# 3. Qt WebEngine を configure してビルドし、Qt のインストール先へ入れる（docs/BUILD.md「この環境の QtWebEngine」）
# 4. Vanilla を再ビルドする
```

パッチは当てる先のファイルの改行に合わせてある（`isolated-world-unsafe-eval` だけが LF。`.gitattributes` で変換を止めている）。
当たっているかは、Vimium を入れた Qt のビューで `o` を押して vomnibar が出るかで分かる。
`patch` が当たらないときは、Qt 側が直っているか形が変わっている
（`extensions_renderer_client_qt.cpp` の `WillSendRequest` が `extensions::ExtensionsRendererClient::WillSendRequest` を呼んでいれば直っている）。
パッチを外すには `patch --binary -R -p1` で戻す。パッチ無しでも、拡張自身のページ（オプションなど）とページ内の操作は動く。

## ライセンス

BSD 3-Clause License. Copyright (c) 2016, rallentando. 全文は [LICENSE](LICENSE) を参照。

---

（Deodorized. This tree is published with its source comments
and internal annotations stripped; the diff against the original
is [deodorant.zip](docs/deodorant.zip).）

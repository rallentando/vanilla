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
| 任意 Qt モジュール | **WebEngine**（既定のビュー。無いとページを表示できるビューが無くなる）, PrintSupport, WebView（`QuickNativeWebView` が有効になる） |
| 任意 SDK | `third_party/webview2` に WebView2 SDK があると、Windows x64 で `EdgeWebView` が有効になる |

ビルドシステムは CMake（qmake は移行期間中のみ残置）。
手順とテストの回し方は [docs/BUILD.md](docs/BUILD.md) を参照。

## ドキュメント

- [docs/BUILD.md](docs/BUILD.md) — ビルド手順・テスト・配布物の作り方
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — 主要クラスと責務、データの置き場

## 配布物について

- 配布は [GitHub の Releases](https://github.com/rallentando/vanilla/releases)。zip 形式の
  ポータブル構成で、設定とセッションは実行ファイルの隣の `data/` に作られる
- 同梱している Qt / Qt WebEngine / Edge WebView2 ローダのライセンス通知は zip の `licenses/` にある
- 実行ファイルは**コード署名をしていない**。初回起動時に Windows の SmartScreen が
  警告を出すことがある（「詳細情報」→「実行」で起動できる）。
  配布 zip の SHA-256 をリリースノートで公開するので、照合のうえ実行してほしい

## ライセンス

BSD 3-Clause License. Copyright (c) 2016, rallentando. 全文は [LICENSE](LICENSE) を参照。

---

（Deodorized. This tree is published with its source comments
and internal annotations stripped; the diff against the original
is [deodorant.zip](docs/deodorant.zip).）

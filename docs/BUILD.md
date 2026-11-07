# ビルド手順

最終更新: 2026-11-03

この文書のパスは例で、版は vanilla の開発者が確かめたもの。パスは自分の環境に置き換えて読む。

ビルドシステムは **CMake**（[CMakeLists.txt](../CMakeLists.txt)）。qmake project は
0.3.0だけ移行用に残し、公開後に削除した（D-296）。

---

## 必要なもの

### Qt

**Windows は Qt 6.12.0 + QtWebEngine 6.140.0、Linux はディストリの Qt 6.10.2 で動作確認済み。Qt5 では動かない**（D-003）。
CMake は 6.5 以上を要求する。

必要なモジュール:

| 区分 | モジュール |
|---|---|
| 必須 | `core` `gui` `widgets` `network` `xml` `opengl` `openglwidgets` `webchannel` `multimedia` `multimediawidgets` `qml` `quick` `quickwidgets` |
| 必須（ビルド時のみ） | `linguisttools`（`lupdate` / `lrelease`） |
| 任意（あれば有効化） | `webenginecore` `webenginewidgets` `webenginequick` — **無いと Qt WebEngine のビューなしでビルドされる** |
| 任意（あれば有効化） | `printsupport` |
| 任意（あれば有効化） | `webview` — `QuickNativeWebView` が有効になる |

任意モジュールの有無は CMake の `find_package(Qt6 QUIET COMPONENTS ...)` と `if(TARGET Qt6::...)` で
判定し、対応する定義（`WEBENGINEVIEW` など）を自動で付ける。
WebEngine が見つからないときは configure 時に警告が出る。

> Qt Maintenance Tool で Qt をインストールする際、**WebEngine は既定で選択されない**ことがある。
> 明示的にチェックすること。
> **Qt 6.12 から QtWebEngine は Qt とは別の製品で、版も別**（Qt 6.12.0 と組むのは QtWebEngine 6.140.0。
> 6.140 は土台の Chromium 140 を表す）。Maintenance Tool では Qt の版の下の「Extensions」に出る。
> 既定の場所へ入れると `C:/Qt/6.12.0/msvc2022_64` になる。

#### Qt WebEngine へのパッチ

Qt WebEngine（6.11.2 と、Qt 6.12.0 と組む 6.140.0 で確認）は、Web ページから拡張のリソース（manifest の `web_accessible_resources`）を
読む要求を必ず `chrome-extension://invalid/` に書き換える（Qt が自前の判定表を持つが、そこに誰も登録しないため）。
このため、Vimium の vomnibar（`o` / `b` / `T`）・HUD・ページの上のヘルプなど、拡張が Web ページの中に
自分のページを iframe で置く機能が動かない。**配布物にはパッチを当てた Qt WebEngine を入れていない。**
Edge のビューには関係しない。パッチ無しでも、拡張自身のページ（オプションなど）とページ内の操作は動く。

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
# 3. Qt WebEngine を configure してビルドし、Qt のインストール先へ入れる（下の「Qt WebEngine をビルドする」）
# 4. vanilla を再ビルドする
```

パッチは当てる先のファイルの改行に合わせてある（`isolated-world-unsafe-eval` だけが LF。`.gitattributes` で変換を止めている）。
当たっているかは、Vimium を入れた Qt のビューで `o` を押して vomnibar が出るかで分かる。
`patch` が当たらないときは、Qt 側で直ったか、当てる先のコードが変わっている
（`extensions_renderer_client_qt.cpp` の `WillSendRequest` が `extensions::ExtensionsRendererClient::WillSendRequest` を呼んでいれば直っている）。
パッチを外すには `patch --binary -R -p1` で戻す。

#### Qt WebEngine をビルドする

公式の Qt WebEngine をそのまま使うなら、この節は要らない。上のパッチを当てるとき、または H.264 / AAC（公式バイナリには入っていない）を
再生したいときだけ、自分でビルドする。全体のビルドには数時間かかる。

下の例は Qt 6.12.0 を既定の場所へ入れたときのもの。`<...>` は自分の環境の場所に置き換える。

```bat
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
set VSLANG=1033
set PYTHONUTF8=1
set QTDIR=C:\Qt\6.12.0\msvc2022_64
set PATH=C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;<bison・flex・gperf の場所>;<node の場所>;<Python の場所>;%QTDIR%\bin;%PATH%;C:\Program Files\Git\usr\bin
cd /d <空のビルドディレクトリ>
call "%QTDIR%\bin\qt-configure-module.bat" C:/Qt/6.12.0/Src/qtwebengine -webengine-proprietary-codecs -- -G Ninja -DCMAKE_BUILD_TYPE=Release -DQT_SHOW_EXTRA_IDE_SOURCES=OFF
ninja
cmake --install . --prefix <出力先>
```

- **`-webengine-proprietary-codecs`** が H.264 / AAC を有効にする。
- **`PYTHONUTF8=1`**: 日本語の Windows では、これが無いと GN の `gn_find_mocables.py` が `UnicodeDecodeError` で落ちる。
- **`VSLANG=1033`** は Visual Studio の英語の言語パックが入っていて初めて効く（`...\MSVC\<ver>\bin\Hostx64\x64\1033\` があること）。
- **ツール**: bison と flex は winflexbison 2.5.25 の `win_bison.exe` / `win_flex.exe` を `bison.exe` / `flex.exe` の名前でも置く。
  gperf は `winget install oss-winget.gperf` で入れる。perl は Git for Windows のもの（上の PATH の末尾）でよく、無いと opus の一部の最適化が外れた旨の警告が configure に出る。
- **入れかた**: `<出力先>` の中身を `%QTDIR%` へ上書きコピーする。公式に戻せるよう、上書きされるファイルを先に退避しておく。
  Qt を更新すると、上書きしたファイルは公式のものに戻る。
- **入れ替えたら vanilla を再ビルドし、package も作り直す。** エンジンの DLL は package に配置される。
- **確かめかた**: vanilla の Qt WebEngine のビューで `document.createElement('video').canPlayType('video/mp4; codecs="avc1.42E01E"')` が
  `probably` なら H.264 入り、空文字なら入っていない。
- **画面のちらつき**: 自分でビルドした Qt WebEngine を Direct3D 11 で動かすと、動画の再生やスクロールのときに黒いボックスが並ぶ、
  または画面の半分が黒くなることがある。OpenGL では消える。設定画面の「描画とエンジン」にある「描画 API（WebEngineView）」で選ぶ（Windows の既定は `OpenGL`）。

### WebView2 SDK（任意。Windows x64 のみ）

`EdgeWebView`（Edge WebView2 を直接ホストするビュー）を有効にするには、
`third_party/webview2/` に SDK のヘッダ2つとローダ2つが要る。
**無くてもビルドは通り、そのビューだけが外れる**（CMake が自動判定し、
configure 時にどちらに転んだかが出る）。

この checkout の `third_party/webview2/` に `include/` と `x64/` が無ければ、
NuGet の `Microsoft.Web.WebView2`（1.0.4129.50）から同じ4ファイルを取って置く。
**取るファイル・配置・各ファイルの SHA-256 は
[third_party/webview2/README.md](../third_party/webview2/README.md) の表にある。**
Edge WebView2 の**ランタイム**は SDK とは別物で、同梱もしない ——
入っていないマシンではこのビューが `Failed` を表示するだけで落ちない。

### コンパイラ

- Windows: **MSVC 2022**（Visual Studio 2022 の「C++ によるデスクトップ開発」ワークロード）
- macOS: Xcode の clang（未確認）
- Linux: **GCC 15.2 で確認済み**（clang は未確認）

---

## ビルドディレクトリ

**ビルド成果物はリポジトリの外、`vanilla/` の隣の `vanilla-build/` に置く。**
1つに揃えてあるので、他の置き場を作らないこと。

```
vanilla/                 <- このリポジトリ
    src/                 <- ソース（core / app / input / ui / view / gadgets）
    tests/  docs/  resources/  qrc/  translations/
vanilla-build/
    debug/               <- CMake + Ninja、Debug
    release/             <- CMake + Ninja、Release
    package/             <- 配布用（cmake --install の出力。下の「配布物を作る」）
    qtcreator-*/         <- Qt Creator のシャドウビルド
```

**この置き場は [CMakePresets.json](../CMakePresets.json) に定義してある。**
プリセットの `binaryDir` は `${sourceParentDir}/vanilla-build/${presetName}` ——
`sourceParentDir` はこのチェックアウトを含むディレクトリなので、**絶対パスを
どこにも書かずに**リポジトリをどこへ置いても同じ関係になる。
`release` と `debug` の2つがあり、`--preset` を使えば置き場を毎回打たなくてよい
（下の「Windows でのビルド」）。

**Qt の場所だけはプリセットに書けない**（マシンごとに違い、このファイルは
コミットされる）ので `QTDIR` から取る。`QTDIR` を設定してから使うこと。

ソースの分け方は [ARCHITECTURE.md](ARCHITECTURE.md) の「ディレクトリ構成」（D-033）。
**`#include` はファイル名だけの裸書き**で、`src/` の各ディレクトリが
インクルードパスに載っている（`target_include_directories`）。
**ソースを足したら CMakeLists.txt に足すこと。**

**なぜリポジトリの外か**

- **リポジトリ内に置くと、コードを探すたびに生成物が引っかかる。**
  `moc_*.cpp` や `*_autogen/` は本物のソースと同じ識別子を持つので、
  全文検索の結果が倍以上になり、実装かコピーかを毎回見分けることになる。
- **`git add -A` が巻き込む。** 一度実際に起きた。
  `.gitignore` の `build/` / `build-*/` は**その事故の名残として残してある**が、
  外に置いていれば `.gitignore` に頼る必要がそもそもない。
- **設定とセッションは実行ファイルの隣の `data/` に作られる**ので、
  ビルドディレクトリごとに独立する。既存の設定を壊さずに試せる反面、
  設定を引き継ぎたいときは `data/` を手でコピーすること。

---

## Windows でのビルド

`vcvars64.bat` で MSVC 環境を用意し、Qt と Qt 同梱の CMake / Ninja に PATH を通す。

```bat
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
set QTDIR=C:\Qt\6.12.0\msvc2022_64
set PATH=C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;%QTDIR%\bin;%PATH%

cmake --preset release
cmake --build --preset release
```

> **`VSLANG=1033` を入れておくこと**（`set VSLANG=1033`）。ninja がヘッダ依存を拾うのは
> `cl.exe` の `Note: including file:` を読めたときだけで、**日本語の `cl.exe` は別の行を出す**。
> 読めないと **ヘッダだけを直したときに何も作り直されない**。

プリセットが置き場・ジェネレータ・ビルド種別を持っているので、
**`-B` も `-G` も `-DCMAKE_BUILD_TYPE=` も `-DCMAKE_PREFIX_PATH=` も要らない**
（[CMakePresets.json](../CMakePresets.json)。上の「ビルドディレクトリ」）。
`QTDIR` だけは先に設定しておくこと。

プリセットを使わない書き方も従来どおり通る。効果は同じ:

```bat
cmake -S . -B ..\vanilla-build\release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QTDIR%
cmake --build ..\vanilla-build\release
```

- デバッグビルドは `cmake --preset debug`（プリセットを使わないなら
  `-DCMAKE_BUILD_TYPE=Debug`）。置き場は `..\vanilla-build\debug`。
- 実行時に Qt の DLL が必要なので、`%QTDIR%\bin` に PATH を通したまま起動するか、
  配布用ディレクトリを作る（下の「配布物を作る」）。

### テストを走らせる

```bat
cmake --build --preset release
ctest --preset release
```

`--output-on-failure` はテストプリセットに入っているので付けなくてよい。
プリセットを使わないなら従来どおり:

```bat
cmake --build ..\vanilla-build\release
ctest --test-dir ..\vanilla-build\release --output-on-failure
```

- テストは既定でビルドされる。要らないときは `-DVANILLA_BUILD_TESTS=OFF`。
- 実行時に Qt の DLL が要るので、**`%QTDIR%\bin` に PATH を通したまま**回すこと。
- 失敗したテストだけ見たいときは `ctest --test-dir ..\vanilla-build\release -R tst_treeserializer -V`。
- 中身は [tests/](../tests/) を参照。

### Qt Creator / VS Code

`CMakeLists.txt` を開き、Qt 6.12 + MSVC 2022 のキットを選ぶだけでよい。
**ビルドディレクトリは `../vanilla-build/` の下に向けること**（上の「ビルドディレクトリ」）。
Qt Creator は既定でリポジトリ内の `build/` に作るので、キットを選んだあとに
「ビルド設定」でシャドウビルドの場所を直すこと。

どちらも [CMakePresets.json](../CMakePresets.json) を読むので、**プリセットを
選べば置き場は勝手に合う**（Qt Creator は「プロジェクト」画面、VS Code は
CMake Tools の "Select Configure Preset"）。ただし `QTDIR` は IDE を起動した
環境に無いことが多いので、その場合は従来どおり手で向けること。

## macOS でのビルド

```sh
export PATH=~/Qt/6.12.0/macos/bin:$PATH
cmake -S . -B ../vanilla-build/release -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/macos
cmake --build ../vanilla-build/release
```

macOS 固有のソース（`src/ui/mainwindowsettings.mm`）は `if(APPLE)` で追加され、`AppKit` をリンクする。
`.app` バンドルとして作られ、アイコンは `vanilla.icns`。

> **macOS ではまだ一度もビルドしていない。** CMakeLists は `.pro` を翻訳しただけなので、
> 初回は素直に通らない可能性がある。Linux は 2026-08-27 にビルドとテストを通した ——
> 下の「Linux でのビルド」を参照。

---

## Linux でのビルド

**2026-08-27 に Ubuntu 26.04 LTS（WSL2）で通した。** 2026-09-28 にディストリの Qt 6.10.2 で ctest 59/59（Windows 専用の `#ifdef` 漏れと Windows 形のパスのテストを 1 つずつ直した）。ディストリの Qt でよく、
Qt のインストーラは要らない。

```sh
sudo apt install build-essential cmake ninja-build libgl1-mesa-dev
sudo apt install qt6-base-dev qt6-base-dev-tools qt6-declarative-dev
sudo apt install qt6-webengine-dev qt6-webengine-dev-tools qt6-webchannel-dev qt6-webview-dev
sudo apt install qt6-multimedia-dev qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools
cmake -S . -B ../vanilla-build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build ../vanilla-build/release
```

WebEngine のビューのパスワードの保存は Qt D-Bus があれば入り、GNOME Keyring か KWallet に置く。
libsecret を使わせるなら `sudo apt install libsecret-1-dev` も入れる（無くてもビルドは通る）。
QuickWebEngine のビューで iframe の中のフォームも扱うには、Qt の `WebEngineQuickPrivate` モジュールとそのヘッダが要る（無ければメインフレームだけ）。apt の `qt6-webengine-dev` はヘッダを入れないので、メインフレームだけになる。

`qt6-l10n-tools` は `lupdate` を持っている。Qt を自分で入れた場合は
`-DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/gcc_64` を足す。

テストは画面なしで走る:

```sh
QT_QPA_PLATFORM=offscreen ctest --test-dir ../vanilla-build/release --output-on-failure
```

`EdgeWebView` は Windows 専用なので configure で外れる（そう出る）。
`vc_redist` も `windeployqt` も関係しない。

### プリセットを使ってよいのは、ソースと build tree が同じ側にあるときだけ

プリセットの `binaryDir` は `${sourceParentDir}/vanilla-build/${presetName}` なので、
**WSL から `/mnt/c` のソースに対して `cmake --preset release` を実行すると、
Windows 側の build tree を指して `CMakeCache.txt` を壊す。**
その構成では `-S` と `-B` を明示し、build tree は WSL 側に置くこと:

```sh
cmake -S /mnt/c/Users/<user>/vanilla -B ~/vanilla-build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release
```

ソースも build tree も Linux 側にあるなら、プリセットはそのまま使える
（`ninja` が要り、Qt は `QTDIR` から取る: `export QTDIR=$HOME/Qt/6.12.0/gcc_64`。
macOS なら `.../macos`）。

### WSLg で動かすときに出るもの

GUI は WSLg でそのまま出る（Windows 側からは `msrdc` の窓として見える）。
ただし **WebEngine の描画プロセスが起動できないことがある** ——
`MESA: ZINK: failed to choose pdev` / `libEGL: failed to create dri2 screen` に続いて
`zygote_communication_linux.cc(301)` が出る。サンドボックスの問題ではなく
（`--no-sandbox` を渡しても変わらない）、GPU/EGL の初期化が本筋と見ている。
この状態のページは復帰を 10 回試したところで打ち止めになる（`RenderProcessLedger`）。

---

## 翻訳

翻訳ファイルは [translations/](../translations/) にある。

| ファイル | 由来 |
|---|---|
| `vanilla_en.ts` / `vanilla_ja.ts` | このプロジェクトの `tr()` から `lupdate` が生成する |
| `custom_en.ts` / `custom_ja.ts` | **手書き**。Qt 側の文字列を訳したもの。lupdate には渡さない |

```sh
cmake --build ../vanilla-build/release --target update_translations   # .ts を更新する（lupdate）
cmake --build ../vanilla-build/release                                # .qm は通常のビルドで作られる
```

- `.qm` は `<build>/translations/` に作られ、ビルド後に実行ファイルの隣へコピーされる
  （[application.cpp](../src/app/application.cpp) の `BootApplication` が
  `applicationDirPath() + "/translations"` から読むため）。
- 翻訳をリソースに埋め込んでいないのはこのため。
- **`.pro` の `lupdate_only` ブロックのような二重管理は無くなった。** CMake 側は
  ターゲットのソースを自動で拾い、QML だけ `SOURCES` で明示している。

### 翻訳を足すときの注意

- UIでは `ViewNode` / ビューノードを `Tab` / タブと表示する。内部の型名・アクションID・
  設定キーは維持する。互換性のためアクション名と `tr()` のキーは元の
  `NewViewNode` / `OpenViewNode` / `NextView` / `PrevView` / `BuryView` / `DigView` 等を維持し、
  タブ表記への変更は英日 `.ts` の訳だけで行う。
  既存の短い動詞と語順に合わせ、文章や括弧をキーへ持ち込まない。新規・複製は対象を明記し、
  複数の対象を扱う項目は「複製（タブ/ディレクトリ）」のように併記する。
  アプリケーションメニューは「タブ一覧」、前後移動・最前面/最背面への移動も「タブ」と表示する。
- 日本語の文章は「：」「、」「。」を使い、コロン前後の半角空白を除く。
  三点リーダーは「…」。URL・正規表現・拡張子・小数点・設定書式・禁止文字一覧は
  実際の入力に必要な記号を保持する。

- **`tr()` / `translate()` / `QT_TRANSLATE_NOOP()` をマクロで短くしないこと。**
  **lupdate はマクロを展開せず、しかも何も言わない。** 短縮した文字列は
  `.ts` に現れないまま英語で出続ける。これは実際に起きていて、
  [settingsschema.cpp](../src/core/settingsschema.cpp) の 101 個が 7 個しか拾われていなかった。
- 拾えた数は `update_translations` の出力（`Found N source text(s)`）で確かめる。
- **通常の英文原文は、英語側の訳を空の `<translation></translation>` にする。**
  アクションの識別用キーは例外で、英語の表示文言も `.ts` の訳へ書く。
  `type="unfinished"` を残すと lrelease が未訳として数える。空の訳は原文に落ちる。
- **設定画面（`vanilla://settings`）のページには英語を直書きしないこと。**
  ページは Chromium の中にいて翻訳に触れないので、
  表示するものはすべて `/api/schema` 経由で訳された状態を受け取る（D-022）。

---

## 配布物を作る

```bat
cmake --install ..\vanilla-build\release --prefix %BUILDROOT%\package
```

`%BUILDROOT%` は `..\vanilla-build` の**絶対パス**（`--prefix` は相対だと止まる。下の注意）。
この文書では以後もその意味で使う。

> **人に配る package は公式の QtWebEngine で作ること**（D-241 追記4）。install はそのとき Qt に
> 入っている方の DLL をそのまま配布物へ入れるので、自分でビルドしたものを入れているなら先に公式へ戻す。

Qt の入っていないマシンで動くディレクトリが1発でできる。中身は `windeployqt`（macOS では
`macdeployqt`）が決めるので、こちらで DLL を並べる必要はない。**実測 426MB / 1600 ファイル**（2026-08-30、0.3.0 の配布物。FFmpeg 除外後、ライセンス通知込み）。

- 出力先はビルドツリーの隣の `vanilla-build\package`。
- **`--prefix` は絶対パスで渡すこと。** 相対パスだと `qt.conf` を書く段で

  ```
  CMake Error at .../Qt6CoreDeploySupport.cmake:38 (message):
    Given qt.conf path is not an absolute path: '..\vanilla-build\package/./qt.conf'
  ```

  で止まる。**そこまでに `vanilla.exe` と `translations/` だけは置かれている**ので、
  出来かけのディレクトリが残る —— 「途中で失敗したのに何か入っている」のはこれ。
  `qt6_deploy_runtime_dependencies` が `${CMAKE_INSTALL_PREFIX}` をそのまま
  `qt6_deploy_qt_conf` に渡すため（2026-08-13、Qt 6.11.1 で確認）。
- **上書きであって作り直しではない。** 消えたファイルは残るので、
  数が合わないときは一度ディレクトリごと消してから入れ直すこと。
- **動作確認のために package の exe を起動したら、`data/` を消してから配ること。**
  設定とセッションは実行ファイルの隣に作られる（上の「ビルドディレクトリ」）ので、
  一度動かすとプロファイルが丸ごと残る。**実測で 229 ファイル / 100MB 増えた。**
  `temp/` も同じ。
  - **WebEngine のプロファイルだけは `data/` の外にできる**（D-099）——
    `%APPDATA%/vanilla/QtWebEngine/<MD5(実行パス + id)>`。配布物には混ざらないが、
    **その exe を動かしたマシンには残る**。ハッシュに実行パスが
    入っているので、package の exe を試したぶんは package 専用のディレクトリになる。
- `windeployqt` が `qtposition_nmea.dll` の依存（`Qt6SerialPort.dll`。Qt の
  インストールに入っていない）を解決できず警告を出す。位置情報プラグインは
  遅延ロードなので、使わない限り実害は無い。

- Windows では**実行ファイルと同じ階層にすべてを置く**（`data/` が実行ファイルの隣にできる
  ポータブル方式のため）。CMake の既定は `bin/` に置くので、`QT_DEPLOY_BIN_DIR` を上書きしている。
- 同梱されるもの: Qt の DLL、`plugins/`、`qml/`、`resources/`（WebEngine の `.pak` と `icudtl.dat`）、
  `QtWebEngineProcess.exe`、`translations/`（Qt の `qt_*.qm` ＋ 本体の `vanilla_*.qm` / `custom_*.qm` ＋
  `qtwebengine_locales/*.pak`）、`vc_redist.x64.exe`、
  本体の `LICENSE`、`licenses/webview2/` の `LICENSE.txt` / `NOTICE.txt`、
  `licenses/qt/` の `NOTICE.txt` と LGPLv3 / GPLv3 / `LICENSE.Chromium`。
- `qml/` は `QtQuick` / `QtWebEngine` / `QtWebView` とその実行時依存だけを残す。
  `windeployqt` がoptional import経由で拾う未使用moduleは、生成deploy scriptの実行後に
  CMakeのinstall ruleで削除する（D-297）。Windows styleが必須importするFusionと、
  WindowsのMenuが使うEffectsは実行時依存なので削除しない。
- **Qt を LGPLv3 で配るための通知は `third_party/qt/` に置いてある**（D-241 追記3）。
  `NOTICE.txt` は Qt と Chromium の版番号を持つので、**ビルドに使う Qt を上げたら
  書き換えること** —— 版が食い違うと configure が warning を出す。
- **FFmpeg は package の種類で分かれる**（D-241 追記6）。
  - **配布用（既定）は入らない**: deploy に `--no-ffmpeg` と
    `--exclude-plugins ffmpegmediaplugin` が渡り、`plugins/multimedia/` に残るのは
    `windowsmediaplugin.dll` だけになる。
  - **人に配らない、自分のマシンだけで使う package には入れてよい**:
    `cmake --preset release -DVANILLA_PACKAGE_FFMPEG=ON` で configure してから
    install すると av* の 5 つの DLL と `ffmpegmediaplugin.dll` が同梱され、
    **Qt Multimedia は FFmpeg バックエンドを選ぶ**（`LocalView` は WMF で
    再生できない形式もこちらで動く）。**キャッシュに残るので、配布物を作る前に
    OFF へ戻すこと**。
- **`vc_redist.x64.exe` が入るのは vcvars を通した環境だけ。** windeployqt は
  既定でコンパイラランタイムを配るが、置き場を `VCINSTALLDIR` から探すので、
  **素のシェルで install すると警告も出さずに落とす**（`--compiler-runtime` を
  明示したときだけ "VCINSTALLDIR is not set" と言う。`--dry-run` で
  VCINSTALLDIR の有無 × 既定 / `--compiler-runtime` の4通りを確認、2026-08-24）。
  **配る package は必ず vcvars のシェルで作り、`vc_redist.x64.exe` の有無を
  確かめること**。
  CMake は `--compiler-runtime` を明示して渡すが、**それで気づけるわけではない** ——
  2026-08-27 に Qt 6.11.1 で測り直したところ、`VCINSTALLDIR` の無いシェルでは
  明示しても何も言わず、`--no-compiler-runtime` との差も出なかった
  （`--dry-run` の両者とも vc_redist に触れず終了コードも同じ）。
  **有無の確認は人がやるしかない。**
- `qt.conf` は `windeployqt` が `Prefix = .` で書き出す。**リポジトリの `qt.conf` は同梱していない**
  （`Data = .` は `Prefix = .` に含まれるため）。

配布の方針は D-241（zip、コード署名は保留で SHA-256 公開）。
CI は当面入れない（D-016）。

## Windows Sandbox で smoke test（2026-09-28）

Windows 11 Pro の Windows Sandbox は、起動のたびにまっさらな Windows になる。VC++ ランタイムは入っておらず、
WebView2 ランタイムは入っている。有効にするのは管理者の PowerShell で
`Enable-WindowsOptionalFeature -Online -FeatureName Containers-DisposableClientVM -All` と再起動。

1. 上の「配布物を作る」の手順で package を作る（公式 QtWebEngine）。
2. `.wsb` を書く。package を `ReadOnly` で `C:\pkg` へ渡し、`LogonCommand` で中のデスクトップへ
   `robocopy` してから `vanilla.exe` を起動する（vanilla は exe の隣に `data/` を書くので、読み取り専用のままでは動かない）。
   GPU なしは `<vGPU>Disable</vGPU>`。
3. 最初は「VCRUNTIME140.dll が見つからない」で止まるのが正しい。同梱の vc_redist は画面つきだと Sandbox で
   止まるので、PowerShell で `Start-Process <デスクトップ>\vanilla\vc_redist.x64.exe -ArgumentList '/install','/quiet','/norestart' -Wait`。
4. 見るもの: 起動、既定のビュー（`msedgewebview2.exe` の有無）、メニューの言語、設定の保存と再起動。

Sandbox の表示言語は日本語の次に英語で、日本語だけの Windows では起きない、翻訳を読み込む順の不具合が出た（D-460）。
DirectComposition の使えない環境は作れない。

---

## データの保存先

Windows では**実行ファイルと同じディレクトリ**の `data/` 以下に設定・セッションが保存される
（[application.cpp](../src/app/application.cpp) `Application::BaseDirectory`）。
ただし `C:\Windows\` や `C:\Program Files\` 配下に置いた場合は `AppLocalDataLocation` へ退避する。
macOS / Linux では `AppLocalDataLocation` を使う。

つまり **Windows ではビルドディレクトリごとに設定が独立する**。
新しいビルドを試しても既存の設定を壊さない反面、設定を引き継ぎたいときは `data/` を手でコピーする必要がある。

---

## 既知の問題

- deprecation 警告（MSVC の C4996）は `QSsl::TlsV1_0` / `QSsl::TlsV1_1` の2件だったが、
  D-269 でその2つを削除した。**その後の件数は数え直していない。**
  **CMake ビルドでは既定で見えない。** `CMakeLists.txt` は `/W` を指定しないので
  MSVC 既定の `/W1` になり、レベル3の C4996 は出力されない。数えるときは
  `-DCMAKE_CXX_FLAGS="/DWIN32 /D_WINDOWS /EHsc /W3"` で別ツリーを建てる。
- **デバッグビルドが起動直後に Chromium の DCHECK で落ちる**ことがある
  （`DCHECK failed: used_count == used_items_`）。qmake が残っていた時期にも CMake と
  同じように起きたため、**ビルドシステムとは無関係**。
  Release ビルドでは起きない。

---

（Deodorized. This tree is published with its source comments
and internal annotations stripped; the diff against the original
is [deodorant.zip](deodorant.zip).）

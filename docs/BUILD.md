# ビルド手順

最終更新: 2026-08-30

ビルドシステムは **CMake**（[CMakeLists.txt](../CMakeLists.txt)）。
**qmake（[vanilla.pro](../vanilla.pro)）は 0.3.0 まで並存させると決めた**（D-241。
リリースノートにも「this release only」と書いた）。0.3.0 は公開済みなので次版で削除する
（ROADMAP.md D）。それまではソースを足すとき両方に足すこと。

---

## 必要なもの

### Qt

**Qt 6.11 で動作確認済み。Qt5 では動かない**（D-003）。
CMake は 6.5 以上を要求する。

必要なモジュール:

| 区分 | モジュール |
|---|---|
| 必須 | `core` `gui` `widgets` `network` `xml` `opengl` `openglwidgets` `webchannel` `multimedia` `multimediawidgets` `qml` `quick` `quickwidgets` |
| 必須（ビルド時のみ） | `linguisttools`（`lupdate` / `lrelease`） |
| 任意（あれば有効化） | `webenginecore` `webenginewidgets` `webenginequick` — **無いとページを表示できるビューが無くなる** |
| 任意（あれば有効化） | `printsupport` |

任意モジュールの有無は CMake の `find_package(Qt6 QUIET COMPONENTS ...)` と `if(TARGET Qt6::...)` で
判定し、対応する定義（`WEBENGINEVIEW` など）を自動で付ける。
WebEngine が見つからないときは configure 時に警告が出る。
（qmake 側は `qtHaveModule()` で同じことをしている）

> Qt Maintenance Tool で Qt をインストールする際、**WebEngine は既定で選択されない**ことがある。
> 明示的にチェックすること。

#### この環境の QtWebEngine —— 自前ビルドと公式を行き来できる

**開発機には QtWebEngine が2つある。** どちらが `C:/Qt/6.11.1/msvc2022_64`
に入っているかで挙動が変わるので、**不具合を追う前に今どちらかを見ること。**

| | どこ | 見分け方 |
|---|---|---|
| 自前ビルド | 成果物一式が `C:/qtwe-staging`（526 ファイル / 294MB）、ビルドツリーは `C:/qtwe` | `bin/Qt6WebEngineCore.dll` の SHA256 が `B3E99957…` |
| 公式バイナリ | 退避してある一式が `C:/qtwe-official-backup`（同 526 ファイル） | 同 `02FFF5F4…` |

**入れ替えは [scripts/swap-webengine.ps1](../scripts/swap-webengine.ps1) でやる。**

```powershell
.\scripts\swap-webengine.ps1 -To selfbuilt   # H.264 / AAC が使える方
.\scripts\swap-webengine.ps1 -To official    # MaintenanceTool が入れる方
.\scripts\swap-webengine.ps1 -To selfbuilt -WhatIf
```

- **初回の `selfbuilt` で、上書きする公式ファイルを丸ごと退避する**
  （`C:/qtwe-official-backup` に同じ相対配置で。対応が無いファイルは
  一覧に控えて、戻すときに消す）。だから**どちらへも往復できる**
- **Qt を掴んでいるプロセスがあると中断する。** 途中まで上書きした状態が
  一番たちが悪いので、消極的に止める側にしてある
- **入れ替えたら vanilla を再ビルドして package も作り直すこと。**
  エンジンの DLL は package に配置されるので、古い方が混ざる
- **Qt 本体を更新すると上書きは失われる**（公式で塗り替えられる）。
  そのときは `-To selfbuilt` をもう一度

**2026-08-13 時点では自前ビルドが入っている**（A-8 の切り分けで一度公式に
戻し、済んだので戻した。D-096）。確かめ方は `vanilla://` で
`document.createElement('video').canPlayType('video/mp4; codecs="avc1.42E01E"')`
—— 自前ビルドなら `probably`、公式なら空文字。

ソースはどちらも `C:/Qt/6.11.1/Src/qtwebengine`（インストーラが展開したまま、未改変）。
`C:/qtwe/config.summary` のうち効いているもの:

| | |
|---|---|
| Proprietary Codecs | **yes** —— 恐らくこれが自前ビルドの動機。公式バイナリには H.264 / AAC が入っていない |
| Extensions | **yes** —— `QWebEngineExtensionManager` が使えるのはこのため |
| Spellchecker | yes |
| Jumbo Build | yes |
| Developer build | no（Release） |

**これを知らないと解釈を誤る場面がある。**

- **動画（H.264 / AAC）が再生できるのは自前ビルドのときだけ。**
  公式が入っている間は同じページが再生できない
- **`vanilla-build/package` は入れた側の DLL を同梱する。**
  配る前に、どちらが入っているか確かめること
- エンジン由来の挙動を追うときは、**両方で試せる**のが強みになる。
  A-8 は実際それで決着した（自前ビルドのせいではなかった —— D-096）

### WebView2 SDK（任意。Windows x64 のみ）

`EdgeWebView`（Edge WebView2 を直接ホストするビュー）を有効にするには、
`third_party/webview2/` に SDK のヘッダ2つとローダ2つが要る。
**無くてもビルドは通り、そのビューだけが外れる**（CMake / qmake とも自動判定。
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
    qmake/               <- qmake（移行期間中のみ。中に debug/ release/ ができる）
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
インクルードパスに載っている（`target_include_directories` / `INCLUDEPATH`）。
**ソースを足したら CMakeLists.txt と vanilla.pro の両方に足すこと。**

**なぜリポジトリの外か**

- **リポジトリ内に置くと、コードを探すたびに生成物が引っかかる。**
  `moc_*.cpp` や `*_autogen/` は本物のソースと同じ識別子を持つので、
  全文検索の結果が倍以上になり、実装かコピーかを毎回見分けることになる。
- **`git add -A` が巻き込む。** 一度実際に起きた（HANDOVER.md の「6.」）。
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
set QTDIR=C:\Qt\6.11.1\msvc2022_64
set PATH=C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;%QTDIR%\bin;%PATH%

cmake --preset release
cmake --build --preset release
```

> **`VSLANG=1033` を入れておくこと**（`set VSLANG=1033`）。ninja がヘッダ依存を拾うのは
> `cl.exe` の `Note: including file:` を読めたときだけで、**日本語の `cl.exe` は別の行を出す**。
> 読めないと **ヘッダだけを直したときに何も作り直されない**（HANDOVER.md の
> 「ビルド」に症状）。

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
  配布用ディレクトリを作る（Phase 4-2）。

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

`CMakeLists.txt` を開き、Qt 6.11 + MSVC 2022 のキットを選ぶだけでよい。
**ビルドディレクトリは `../vanilla-build/` の下に向けること**（上の「ビルドディレクトリ」）。
Qt Creator は既定でリポジトリ内の `build/` に作るので、キットを選んだあとに
「ビルド設定」でシャドウビルドの場所を直すこと。

どちらも [CMakePresets.json](../CMakePresets.json) を読むので、**プリセットを
選べば置き場は勝手に合う**（Qt Creator は「プロジェクト」画面、VS Code は
CMake Tools の "Select Configure Preset"）。ただし `QTDIR` は IDE を起動した
環境に無いことが多いので、その場合は従来どおり手で向けること。

### qmake でビルドする（移行期間中のみ）

```bat
mkdir ..\vanilla-build\qmake
cd ..\vanilla-build\qmake
qmake C:\path\to\vanilla\vanilla.pro
nmake -f Makefile.Release
```

- `.pro` を触ったときだけ `qmake` を撃ち直す。それ以外は `nmake` だけでよい。
- **フルビルドは数分かかる。** 前のビルドが走っている間に次を始めると
  `moc: Cannot create ... Permission denied` で落ちるので、必ず終わってから回すこと。
- 2026-08-16（D-143）に CMake とのソース一覧・`qtHaveModule(webview)` 条件を同期し、
  Windows Release の再生成から最終リンクまで確認済み。

---

## macOS でのビルド

```sh
export PATH=~/Qt/6.11.1/macos/bin:$PATH
cmake -S . -B ../vanilla-build/release -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$HOME/Qt/6.11.1/macos
cmake --build ../vanilla-build/release
```

macOS 固有のソース（`src/ui/mainwindowsettings.mm`）は `if(APPLE)` で追加され、`AppKit` をリンクする。
`.app` バンドルとして作られ、アイコンは `vanilla.icns`。

> **macOS ではまだ一度もビルドしていない。** CMakeLists は `.pro` を翻訳しただけなので、
> 初回は素直に通らない可能性がある。Linux は 2026-08-27 にビルドとテストを通した ——
> 下の「Linux でのビルド」を参照。

---

## Linux でのビルド

**2026-08-27 に Ubuntu 26.04 LTS（WSL2）で通した。** ディストリの Qt でよく、
Qt のインストーラは要らない。

```sh
sudo apt install build-essential cmake ninja-build libgl1-mesa-dev
sudo apt install qt6-base-dev qt6-base-dev-tools qt6-declarative-dev
sudo apt install qt6-webengine-dev qt6-webengine-dev-tools qt6-webchannel-dev qt6-webview-dev
sudo apt install qt6-multimedia-dev qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools
cmake -S . -B ../vanilla-build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build ../vanilla-build/release
```

`qt6-l10n-tools` は `lupdate` を持っている。Qt を自分で入れた場合は
`-DCMAKE_PREFIX_PATH=$HOME/Qt/6.11.1/gcc_64` を足す。

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
（`ninja` が要り、Qt は `QTDIR` から取る: `export QTDIR=$HOME/Qt/6.11.1/gcc_64`。
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

- **`tr()` / `translate()` / `QT_TRANSLATE_NOOP()` をマクロで短くしないこと。**
  **lupdate はマクロを展開せず、しかも何も言わない。** 短縮した文字列は
  `.ts` に現れないまま英語で出続ける。これは実際に起きていて、
  [settingsschema.cpp](../src/core/settingsschema.cpp) の 101 個が 7 個しか拾われていなかった。
- 拾えた数は `update_translations` の出力（`Found N source text(s)`）で確かめる。
- **英語側の訳は空の `<translation></translation>` にする。**
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

> **配る package は公式の QtWebEngine で作ること**（D-241 追記4）。開発機は
> 自前ビルド（H.264 / AAC 入り）を入れていることがあり、**install はそのとき Qt に
> 入っている方の DLL をそのまま配布物へ入れる**。作る前に
> `.\scripts\swap-webengine.ps1 -To official` を通し、`Qt6WebEngineCore.dll` の
> SHA256 が `02FFF5F4…` であることを確かめる。

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
    `%APPDATA%/vanilla/QtWebEngine/<MD5(実行パス + id)>`。配布物には混ざらないので
    消し忘れても配り物は汚れないが、**この機械には残る**。ハッシュに実行パスが
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
- **Qt を LGPLv3 で配るための通知は `third_party/qt/` に置いてある**（D-241 追記3）。
  `NOTICE.txt` は Qt と Chromium の版番号を持つので、**ビルドに使う Qt を上げたら
  書き換えること** —— 版が食い違うと configure が warning を出す。
- **FFmpeg の DLL と `ffmpegmediaplugin` は入らない**（D-241）。deploy に
  `--no-ffmpeg` と `--exclude-plugins ffmpegmediaplugin` を渡しているため、
  `plugins/multimedia/` に残るのは `windowsmediaplugin.dll` だけになる。
- **`vc_redist.x64.exe` が入るのは vcvars を通した環境だけ。** windeployqt は
  既定でコンパイラランタイムを配るが、置き場を `VCINSTALLDIR` から探すので、
  **素のシェルで install すると警告も出さずに落とす**（`--compiler-runtime` を
  明示したときだけ "VCINSTALLDIR is not set" と言う。`--dry-run` で
  VCINSTALLDIR の有無 × 既定 / `--compiler-runtime` の4通りを確認、2026-08-24）。
  2026-08-24 の監査 prefix に無かったのはこれが原因で、8/13 の package には入っている。
  **配る package は必ず vcvars のシェルで作り、`vc_redist.x64.exe` の有無を
  確かめること**（下の「リリース手順」）。
  CMake は `--compiler-runtime` を明示して渡すが、**それで気づけるわけではない** ——
  2026-08-27 に Qt 6.11.1 で測り直したところ、`VCINSTALLDIR` の無いシェルでは
  明示しても何も言わず、`--no-compiler-runtime` との差も出なかった
  （`--dry-run` の両者とも vc_redist に触れず終了コードも同じ）。
  **有無の確認は人がやるしかない。**
- `qt.conf` は `windeployqt` が `Prefix = .` で書き出す。**リポジトリの `qt.conf` は同梱していない**
  （`Data = .` は `Prefix = .` に含まれるため）。

配布の方針は D-241（zip、コード署名は保留で SHA-256 公開）。**0.3.0 は 2026-08-30 に
この手順で公開した** —— 通しの手順は次の「リリース手順」。次版へ向けた課題は
ROADMAP.md D。CI は当面入れない（D-016）。

## リリース手順（0.3.0 で実際に踏んだ手順、2026-08-30）

公開は GitHub（github.com/rallentando/vanilla）への**スナップショット方式**（D-242）。
私有リポジトリ（Bitbucket）の履歴は公開側に繋がない。順番は次のとおりで、
**成果物はすべてタグのコミットから作る**。途中で私有側にコミットを足したら、タグを
打ち直して package から作り直す（0.3.0 はそれを一度やった）。

### 0. 出す前に確かめること

- `VERSION` の版番号。exe の VersionInfo は `vanilla.rc` が `<VERSION>` を include して
  同じ値になる（qmake は `RC_INCLUDEPATH`）。出したあと exe のプロパティで見る
- `.ts` の未訳が英日とも 0（`grep -c 'type="unfinished"' translations/*.ts`）
- `third_party/qt/NOTICE.txt` の Qt と Chromium の版が、ビルドに使う Qt と合っている
  （食い違うと configure が warning）。WebView2 の通知は無いと configure が止まる
- **QtWebEngine が公式ビルドであること**（上の「配布物を作る」の冒頭）
- ROADMAP D に、その版で片づけると決めた項目が残っていないこと

### 1. 私有側: タグと push

```bat
git tag 0.3.0
git push origin master
git push origin 0.3.0
```

タグは lightweight（0.2.x までと同じ形）。打ち直すときは `git tag -f 0.3.0 <commit>` と
`git push --force origin 0.3.0`。既存の `0.2.2` は 2016 年の別履歴を指すので触らない。

### 2. package と zip（vcvars のシェルで）

```bat
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
set PATH=C:\Qt\Tools\CMake_64\bin;C:\Qt\6.11.1\msvc2022_64\bin;%PATH%
for %I in (..\vanilla-build) do set BUILDROOT=%~fI
cmake --build %BUILDROOT%\release
ctest --test-dir %BUILDROOT%\release --output-on-failure
rmdir /s /q %BUILDROOT%\package\vanilla-0.3.0
cmake --install %BUILDROOT%\release --prefix %BUILDROOT%\package\vanilla-0.3.0
cd /d %BUILDROOT%\package
C:\Windows\System32\tar.exe -a -c -f ..\vanilla-0.3.0-x64.zip vanilla-0.3.0
certutil -hashfile ..\vanilla-0.3.0-x64.zip SHA256
```

`for %I … set BUILDROOT=%~fI` は `..\vanilla-build` を絶対パスにする（`--prefix` が相対を断るため）。
バッチファイルに書くなら `%%I`。

- prefix を `package\vanilla-0.3.0` にするのは、zip の根に展開先ディレクトリを1つ持たせるため。
  zip の名前は過去のリリースに合わせて `vanilla-<版>-x64.zip`
- **zip は Windows 同梱の `tar.exe`（bsdtar）で作る。** .NET の
  `ZipFile.CreateFromDirectory` はエントリ名の区切りが `\` になり（ZIP の仕様は `/`）、
  Windows 以外の展開ツールで壊れる。Git Bash の GNU tar は zip を作れない。
  作ったら `\` を含むエントリ数、FFmpeg の有無、`vc_redist.x64.exe`、`licenses/` の中身を数える
- **zip はタイムスタンプを含むので、同じ手順で作り直すとハッシュが変わる。**
  （余談だがこの文書のコードブロックでは行を `\` で継がない —— リポジトリは CRLF なので
  Linux で `\` の直後に CR が来て継続にならず、2026-08-30 には継ぎの `\` が
  文字どおりの `\r` に化けたまま公開された。1コマンド1行にする）
  Release に上げた実物をそのまま配り、ノートの SHA-256 はその実物の値にする
- package の exe を起動して試したなら `data/` `temp/` を消してから zip にする
  （検証用の probe を置いたなら、それも）

### 3. 公開ツリー: スナップショットと2コミット

```bat
python scripts\make-public-snapshot.py --dest ..\vanilla-gh --ref 0.3.0 ^
    --report %BUILDROOT%\snapshot-report.txt
```

レポートの **`needs a human` と `must not ship` が 0** であること、除外一覧が想定どおり
（AGENTS / CLAUDE / 内部 docs / `docs/relay/` / リリースノート / このスクリプト /
WebView2 のヘッダ）であることを見る。`third_party/qt/` と `third_party/webview2/` の
通知、`docs/deodorant.zip` は出る。

vanilla-gh では**コミットを2つに分ける**（0.2.2 までの作法）:

1. **変更コミット** —— `VERSION` 以外の全部（`git add -A` して `git restore --staged VERSION`）。
   メッセージは、触った領域を小文字で並べて `and periodic update.` で締める要約1行、
   空行、`* …` の箇条書き（過去の版の本文と同じ形）。
2. **リリースコミット** —— `VERSION` だけ。メッセージは版番号だけ（`0.3.0`）。
   `RELEASE_NOTES.md` はリポジトリに置かない（ノートは Release の本文だけ）。

どちらにも `Co-Authored-By:` を3行（Claude Opus / Claude Fable / Codex CLI）。
Codex の行は上流の既定に合わせ `Codex CLI (<model>) <noreply@openai.com>`
（このアドレスが GitHub 上の Codex bot に対応する）。

```bat
git tag 0.3.0
git push origin master
git push origin 0.3.0
```

### 4. GitHub Release

本文は `docs/RELEASE_NOTES-<版>.md`（この側にだけ置く。公開ツリーには出ない）。
形は過去のリリースに合わせ、**リリース名は空**（タグ名が見出しになる）、本文は英語の
`## What's new in vanilla <版>.` ＋箇条書き。末尾に zip の名前と SHA-256、Credits。

```powershell
gh release create 0.3.0 -R rallentando/vanilla --verify-tag `
    --notes-file docs\RELEASE_NOTES-0.3.0.md `
    ..\vanilla-build\vanilla-0.3.0-x64.zip
```

- **`--title ""` を渡さないこと。** PowerShell 5.1 は空文字の引数を落とすので、
  `--title` が次の `--notes-file` を題名として食い、本文が空になってノートの `.md` が
  資産として上がる（0.3.0 で踏んだ）。題名を省けば gh が空にする。
  直すなら `gh release delete-asset` で余計な資産を消し、
  `gh api --method PATCH repos/rallentando/vanilla/releases/<id> --input body.json` で
  `name` と `body` を書き直す。**`--input` の JSON は BOM 無し**（`Set-Content -Encoding utf8`
  は BOM を付けて 400 になる。`[IO.File]::WriteAllText` に `UTF8Encoding($false)` を渡す）
- 作ったら `gh release view 0.3.0 --json name,tagName,assets,body` で、名前が空、
  資産が zip 1本、バイト数が手元と一致することを見る

### 5. あとに

- ROADMAP D を次版に向けたものへ書き換え、決定の記録（D-241 / D-242）に結果を追記する
- `vanilla-build\package` と zip は次に作り直すまで残しておく（配った実物）
- 引継ぎ資料の「まだ確かめていないこと」に、その版で見ていないもの（別環境の起動など）を書く

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
  qmake ビルドは既定が `-W3` なのでそのまま出る。
- **デバッグビルドが起動直後に Chromium の DCHECK で落ちる**ことがある
  （`DCHECK failed: used_count == used_items_`）。
  qmake / CMake の両方で同じように起きるので**ビルドシステムとは無関係**。
  Release ビルドでは起きない。詳細は HANDOVER.md。

---

（Deodorized. This tree is published with its source comments
and internal annotations stripped; the diff against the original
is [deodorant.zip](deodorant.zip).）

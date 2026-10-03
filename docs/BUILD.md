# ビルド手順

最終更新: 2026-09-06

ビルドシステムは **CMake**（[CMakeLists.txt](../CMakeLists.txt)）。qmake project は
0.3.0だけ移行用に残し、公開後に削除した（D-296）。

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

> Qt Maintenance Tool で Qt をインストールする際、**WebEngine は既定で選択されない**ことがある。
> 明示的にチェックすること。
> **Qt 6.12 から QtWebEngine は Qt とは別の製品で、版も別**（Qt 6.12.0 と組むのは QtWebEngine 6.140.0。
> 6.140 は土台の Chromium 140 を表す）。Maintenance Tool では Qt の版の下の「Extensions」に出る。
> 入る先は従来どおり `C:/Qt/6.12.0/msvc2022_64`。

#### この環境の QtWebEngine —— 自前ビルドと公式を行き来できる

**開発機には QtWebEngine が2つある。** どちらが `C:/Qt/6.12.0/msvc2022_64`
に入っているかで挙動が変わるので、**不具合を追う前に今どちらかを見ること。**

| | どこ | 見分け方 |
|---|---|---|
| 自前ビルド | 成果物一式が `C:/qtwe-staging`（534 ファイル / 295MB）、ビルドツリーは `C:/qtwe` | `bin/Qt6WebEngineCore.dll` の SHA256 が `65BE5235…`（2026-10-02、QtWebEngine 6.140.0、パッチ 3 本入り。Qt 6.11.2 のときは `3E977233…`） |
| 公式バイナリ | 退避してある一式が `C:/qtwe-official-backup` | 同 `475A605D…`（Qt 6.11.2 のときは `D6995CE6…`。その一式は `C:/qtwe-official-backup-6.11.2`） |

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
- **入れ替えたら、その exe が使う WebEngine プロファイルの `GPUCache` も消すこと**
  （別ビルドの blob を拾わないための衛生。ちらつきの原因そのものではなかった。
  下の「入れ替え後のちらつき: D3D11 の受け渡し競合」）。
  インストール版なら vanilla を完全終了してから:

  ```powershell
  Get-ChildItem "$env:APPDATA\vanilla\QtWebEngine" -Directory -Recurse -Filter GPUCache | Remove-Item -Recurse -Force
  ```
- **Qt 本体を更新すると上書きは失われる**（公式で塗り替えられる）。
  そのときは `-To selfbuilt` をもう一度

**2026-10-02 時点ではパッチ入りの自前ビルド（QtWebEngine 6.140.0）が入っている**。configure の結果（`config.summary`）は
6.11.2 の自前ビルドと全く同じで、`Build with rust` も `no`（6.140.0 では Rust は任意。公式の DLL にも Rust の痕跡は無い）。
以下は 6.11.2 の自前ビルドの経緯（Qt 6.11.2 への移行時に
同じ configure で再ビルドし、2026-09-16 に英語版 cl で全体を作り直したもの。
公式より画面がちらつく件で一度公式へ戻し、共有 GPUCache を疑って自前へ戻したが、
ちらつきは再発した。**正体は Chromium → Qt Quick の D3D11 テクスチャ受け渡しの競合で、
`QSG_RHI_BACKEND=opengl` で消える**（2026-09-18）。下の「英語の言語パック」と
「入れ替え後のちらつき: D3D11 の受け渡し競合」。
6.11.1 時代に A-8 の切り分けで一度公式に戻した経緯は D-096）。確かめ方は `vanilla://` で
`document.createElement('video').canPlayType('video/mp4; codecs="avc1.42E01E"')`
—— 自前ビルドなら `probably`、公式なら空文字。

ソースはどちらも `C:/Qt/6.12.0/Src/qtwebengine`。**2026-09-22 から `tools/patches/qtwebengine-6.140.0-web-accessible-resources.patch`
が当たっている**（3 ファイル。当てる前の原本は `C:/qtwe-src-orig/` に同じ相対配置で退避。D-365）。
**2026-09-23 から `tools/patches/qtwebengine-6.140.0-isolated-world-unsafe-eval.patch` も当たっている**（1 ファイル `csp_info.cc`。MV3 の
content script の world で eval を許す。拡張の `scripting.executeScript` に要る。D-376 追記 2）。
**2026-09-25 から `tools/patches/qtwebengine-6.140.0-ime-to-active-widget.patch` も当たっている**（1 ファイル `render_widget_host_view_qt_delegate_client.cpp`。
IME を別プロセスの iframe へも送る。Vimium の vomnibar の日本語と候補窓の位置に要る。D-432）。
3 本とも 6.140.0 のソースにそのまま当たった（2026-10-01。原本は `C:/qtwe-src-orig/`、6.11.2 の原本は `C:/qtwe-src-orig-6.11.2/`）。
`csp_info.cc` は LF のファイルなので、`isolated-world-unsafe-eval` のパッチだけは LF で持つ（`.gitattributes` の `-text`）。3 本とも `patch --binary` でそのまま当たる。ファイル名の版は最後に確かめた QtWebEngine の版（2026-10-03 に 6.11.2 から改名。D-464 追記 2）。
staging の `bin/Qt6WebEngineCore.dll` は `65BE5235…`。
この 1 ファイルだけの再ビルドは ninja で 4 分ほど（2026-09-25 実測。ソースを直して `ninja` → install → `swap-webengine.ps1 -To selfbuilt`）。
Qt を上げたら Src が公式に戻るので、`patch --binary -p1 -d C:/Qt/6.x.y/Src/qtwebengine < tools/patches/...` で当て直してから configure する
（当たらなければ Qt 側が直ったか、形が変わったか。`src/core/renderer/extensions/extensions_renderer_client_qt.cpp` の `WillSendRequest`
が本体の `extensions::ExtensionsRendererClient::WillSendRequest` を呼んでいれば直っている）。
再ビルドの手順は `C:/qtwe/config.redo.bat`（実体は `qt-configure-module.bat` に
`-webengine-proprietary-codecs` を渡すだけ）→ `ninja` → `cmake --install . --prefix C:/qtwe-staging`。
全体のビルドは約 4 時間 15 分（2026-10-01 20:05〜10-02 00:21、途中で vanilla のビルドとテストも並べて回した）。
**日本語 Windows では `PYTHONUTF8=1` を設定してから走らせること** —— 6.11.2 の
ソースに cp932 で読めないバイトがあり、GN の `gn_find_mocables.py` が
`UnicodeDecodeError` で落ちる（2026-09-01 に踏んだ）。perl（Git 付属で可）を
PATH に入れないと opus の一部最適化が外れた旨の警告が configure に出る。
bison / flex / gperf は `C:/qtwe-tools/bin`（winflexbison 2.5.25 の exe を `bison.exe` /
`flex.exe` の名前でも置いたもの ＋ `winget install oss-winget.gperf` の gperf 3.1）、
node はユーザーのホームの `.local/node`（`%USERPROFILE%\.local\node`）。
**英語の言語パック**: `VSLANG=1033` は VS の英語言語パックが入っていて初めて効く
（`...\MSVC\<ver>\bin\Hostx64\x64\1033\` があること）。2026-09-01 の自前ビルドは
`VSLANG=1033` を指定していたが英語パックが無く、日本語版 cl で作られていた。
公式より画面がちらつく件の切り分けとして 2026-09-16 に英語パックを入れ、`C:/qtwe` を
空にして全体を作り直した（config.summary は 09-01 と同一）が、**ユーザー実機で
ちらつきは変わらなかった**（2026-09-17）。cl の言語は原因ではなかった（真の原因は次項）。
手順一式は `.claude/skills/qt-upgrade/SKILL.md`。

**入れ替え後のちらつき: D3D11 の受け渡し競合**（2026-09-17〜18、ユーザー実機）。
症状は、動画再生・スクロール・タブ切替・リサイズ・窓移動のときに、黒い 40px 程度のボックスが
等間隔に並ぶ、または画面の半分が黒くなり、1〜数フレームで戻るもの。次の 3 つの**どれでも消える**:

| 変更 | 触る段 |
|---|---|
| `QSG_RHI_BACKEND=opengl`（環境変数。Chromium 側は既定のまま） | Qt Quick が D3D11 の共有テクスチャを受け取る段 |
| `--disable-gpu-compositing`（`application/@ChromiumFlags`） | Chromium の GPU 合成と共有テクスチャ |
| `--disable-gpu-rasterization`（同上） | GPU ラスタしたタイルの完了待ち |

つまり Chromium のプロセス内 GPU スレッドが描き終える前に Qt 側が読む競合で、経路か時間差を
変えればどこでも消える。上流でも「Qt 6 の D3D11 RHI と WebEngine の同期が甘く、
`QQuickWindow::setGraphicsApi(OpenGL)` か `QSG_RHI_BACKEND=opengl` で逃がす」のが定番
（[Qt Forum](https://forum.qt.io/topic/156183/qwebengineview-and-opengl-rendering-problem)、
[anki #4470](https://github.com/ankitects/anki/issues/4470)）。公式ビルドで出にくい理由は
未特定（コード生成の差でタイミングが変わる、までは言えるが確認していない）。
回避策は設定にした: `application/@GraphicsApi`（設定画面「描画とエンジン」の「描画 API (WebEngineView)」、
`Auto|Software|OpenGL|Direct3D11|Direct3D12|Vulkan|Metal`、既定 `Auto`、再起動で反映。D-340 / D-342）。
この機械の自前ビルドでは `OpenGL` にしておく。環境変数 `QSG_RHI_BACKEND` があればそちらが勝つ。

その前段で疑った**共有 GPUCache**（結論: 交絡要因ではあるが原因ではない）:
widget の WebEngine プロファイルは `%APPDATA%/vanilla/QtWebEngine/<MD5(実行ディレクトリ + id)>`
で、鍵が実行パスなので**中のエンジンを公式と自前で入れ替えても同じディレクトリを使い続ける**。
その `GPUCache`（Chromium のシェーダ／プログラムキャッシュ）は「シェーダのハッシュ ＋ Chromium 版
＋ GPU/ドライバ」を鍵にするため、同じ Chromium 版・同じドライバの公式と自前では鍵が衝突し、
片方の ANGLE が作った blob をもう片方が拾う。インストール版のプロファイルは 8/1 から
6.11.1 公式 → 自前 → 6.11.2 公式 → 自前 → 公式 → 自前と同じ `GPUCache` で通していた。
自前に入れ替えたあと `GPUCache` だけを消して起動した直後は、ちらつきが本家 Chrome と同程度の
頻度まで減ったように見えたが、**しばらく使うと再発し、もう一度消しても変わらなかった**
（2026-09-18）。以前より頻度が下がった印象は残るものの、観測が短い区間の比較なので
ノイズの可能性がある。したがって GPUCache は「別ビルドの blob を拾い得る交絡要因」であって
原因ではなく、公式と自前の差の説明はついていない。
この見立ては Codex（Sol）への相談で「原因ではなくても最初に排除すべき交絡要因」として出たもの。
裏付けとして、公式と自前の `chrome://gpu` は Graphics Feature Status・ANGLE（D3D11、GL ES 2.0）・
Driver Bug Workarounds・ANGLE Features まで全項目同一で、差は Video Acceleration の HEVC
（proprietary codecs で加わる）だけだった（2026-09-17 ユーザー採取）。Qt WebEngine は
`--in-process-gpu` で Direct composition も overlay も使わない構成なので、残るちらつきに
DComp／overlay 系のフラグは効かない。
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
set QTDIR=C:\Qt\6.12.0\msvc2022_64
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
  タブ表記への変更は英日 `.ts` の訳だけで行う（2026-09-07 ユーザー訂正）。
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

> **配る package は公式の QtWebEngine で作ること**（D-241 追記4）。開発機は
> 自前ビルド（H.264 / AAC 入り）を入れていることがあり、**install はそのとき Qt に
> 入っている方の DLL をそのまま配布物へ入れる**。作る前に
> `.\scripts\swap-webengine.ps1 -To official` を通し、`Qt6WebEngineCore.dll` の
> SHA256 が上の表の公式バイナリ（Qt 6.12.0 / QtWebEngine 6.140.0 では `475A605D…`）であることを確かめる。
> 表の値は Qt か QtWebEngine を上げるたびに変わる（6.11.2 の公式は `D6995CE6…`、6.11.1 は `02FFF5F4…` だった）。
> **自前ビルドには 2026-09-22 から Vanilla のパッチ（`tools/patches/`）が入っている。配布物にパッチ済みの
> QtWebEngine を入れない**（ユーザー方針 2026-09-22）: 配るのは差分だけで、リリースノートに
> 「拡張の `web_accessible_resources`（Vimium の vomnibar・HUD）を出すには QtWebEngine にこのパッチが要る」と書く。
> IME のパッチ（D-432）も同じ扱いで、「別プロセスの iframe（vomnibar など）で IME を使うにはこのパッチが要る」と書く。

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
  - **ローカル用（この機械から出さない package）は入れてよい**:
    `cmake --preset release -DVANILLA_PACKAGE_FFMPEG=ON` で configure してから
    install すると av* 5 DLL と `ffmpegmediaplugin.dll` が同梱され、
    **Qt Multimedia は FFmpeg バックエンドを選ぶ**（`LocalView` は WMF で
    再生できない形式もこちらで動く）。**キャッシュに残るので、配布物を作る前に
    OFF へ戻すこと**（下の「リリース手順」0.）。エンジンの対応表:
    ローカル = 自前 WebEngine ＋ FFmpeg あり、配布 = 公式 WebEngine ＋ FFmpeg なし。
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

Sandbox の表示言語は日本語の次に英語で、この機械（日本語だけ）では出ない翻訳の順の不具合が出た（D-460）。
DirectComposition の使えない環境は作れない。

## リリース手順（0.3.0 で実際に踏んだ手順、2026-08-30）

公開は GitHub（github.com/rallentando/vanilla）への**スナップショット方式**（D-242）。
私有リポジトリ（Bitbucket）の履歴は公開側に繋がない。順番は次のとおりで、
**成果物はすべてタグのコミットから作る**。途中で私有側にコミットを足したら、タグを
打ち直して package から作り直す（0.3.0 はそれを一度やった）。

### 0. 出す前に確かめること

- `VERSION` の版番号。exe の VersionInfo は `vanilla.rc` が `<VERSION>` を include して
  同じ値になる。出したあと exe のプロパティで見る
- `.ts` の未訳が英日とも 0（`grep -c 'type="unfinished"' translations/*.ts`）
- `third_party/qt/NOTICE.txt` の Qt と Chromium の版が、ビルドに使う Qt と合っている
  （食い違うと configure が warning）。WebView2 の通知は無いと configure が止まる
- **QtWebEngine が公式ビルドであること**（上の「配布物を作る」の冒頭）
- **release ツリーの `VANILLA_PACKAGE_FFMPEG` が OFF であること**（ローカル package 用に
  ON にしてあることがある。`cmake --preset release -DVANILLA_PACKAGE_FFMPEG=OFF` で戻す。
  D-241 追記6）
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
set PATH=C:\Qt\Tools\CMake_64\bin;C:\Qt\6.12.0\msvc2022_64\bin;%PATH%
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

vanilla-gh のコミット作者は公開の履歴に合わせて `rallentando <materialismyearnings@gmail.com>`
（vanilla-gh の `.git/config` に local で設定済み。0.3.0 の 2 コミットはマシン名のまま出た）。

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
- **デバッグビルドが起動直後に Chromium の DCHECK で落ちる**ことがある
  （`DCHECK failed: used_count == used_items_`）。qmake が残っていた時期にも CMake と
  同じように起きたため、**ビルドシステムとは無関係**。
  Release ビルドでは起きない。詳細は HANDOVER.md。

---

（Deodorized. This tree is published with its source comments
and internal annotations stripped; the diff against the original
is [deodorant.zip](deodorant.zip).）

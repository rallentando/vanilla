# 拡張機能の対応状況

Vanilla で Chrome 拡張（Manifest V3）がどこまで動くかを、拡張の作者と利用者が期待値を合わせるためにまとめる。
仕組みは [ARCHITECTURE.md](ARCHITECTURE.md)「拡張機能」にある。

## 前提

- **導入できるのは展開済みのフォルダーだけ。** Chrome ウェブストアや CRX からは入れられない。アドレスバー右のパズルのボタンから登録・有効 / 無効・ピン留めをする
- 拡張が動くのは、ページを表示するエンジンが用意する仕組みの上。エンジンは 2 つあり、拡張 API の出どころが違う
  - **Edge WebView2**（Windows の既定）: API の大部分は WebView2 自身のもの。Vanilla が答えるのは、アプリの木構造に関わる部分（タブ・ウィンドウ・ブックマーク・履歴）と、ボタンやサイドパネルなどアプリの画面に出るもの
  - **Qt WebEngine**（Windows 以外の既定。Windows でもディレクトリ単位で選べる）: エンジンが素で持つ API が少ないため、Vanilla は拡張のコピーに互換レイヤー（シム）を入れて足りない API を埋める（設定 `network/@ExtensionShims`）。Windows 以外の OS の分はビルドも確認もしていない
- Qt WebEngine では、ページから拡張のリソース（`web_accessible_resources`）を読む機能に Qt WebEngine へのパッチが要る（[README](../README.md)「Qt WebEngine へのパッチ」）
- private（OTR）のプロファイルでは拡張を動かさない
- 開発中に実機で使って確かめてきた拡張は Vimium、uBlock Origin Lite、Stands、SingleFile、Dark Reader。どこまで動くかは拡張の版とエンジンによる

## 方針: 仕様には追従しない

足りない API を Chrome の仕様に合わせて埋め続けることはしない。下の表にある範囲で止めていて、残りは**実際に使う拡張で不具合が出たときに**、その拡張が要る分だけ足す。

## 名前空間ごとの対応（Qt WebEngine）

Chrome の拡張 API の名前空間のうち、ChromeOS 専用・Dev チャンネル・型だけのものを除いた 63 を数えた（2026-09-27）。

- **対応（27）**: `action` / `alarms` / `bookmarks` / `commands` / `contextMenus` / `declarativeNetRequest` / `downloads` / `extension` / `fontSettings` / `history` / `i18n` / `identity` / `idle` / `notifications` / `offscreen` / `permissions` / `runtime` / `scripting` / `search` / `sessions` / `sidePanel` / `storage` / `tabs` / `topSites` / `userScripts` / `webNavigation` / `windows`。多くは一部のメンバーだけで、どれが動くかは下の表にある
- **エンジンが素で持つ（6）**: `management` / `power` / `system.cpu` / `system.memory` / `system.display` / `system.storage`
- **未対応（30）**: `accessibilityFeatures` / `browsingData` / `contentSettings` / `cookies` / `debugger` / `declarativeContent` / `desktopCapture` / `devtools.*`（5）/ `dom` / `enterprise.hardwarePlatform` / `gcm` / `instanceID` / `mimeHandler` / `omnibox` / `pageCapture` / `printerProvider` / `privacy` / `proxy` / `publicSuffix` / `readingList` / `tabCapture` / `tabGroups` / `tts` / `ttsEngine` / `webAuthenticationProxy` / `webRequest`

Edge WebView2 では、WebView2 自身が持つものが多く、名前空間ごとには数えていない。

## メソッド単位の表

印: **○** = 答える / **△** = 条件つきで答える（Qt 列は manifest にその権限があるときだけ。Edge 列は一部しか確かめていない）/ **×** = 答えない（呼ぶと失敗する）/ **※** = 制限や注意がある。
表に出すのは、Vanilla が答えるか、Vanilla が答えずにエンジンへ任せるメンバー。Edge 列は、Vanilla が答えるものと WebView2 自身が答えるものを区別しない。

<!-- BEGIN: tst_cdpshims::theTableOfWhatIsAnsweredIsTheShimsOwn -->
| 名前空間 | メンバー | Qt WebEngine | Edge WebView2 |
|---|---|---|---|
| `action` | `disable` | ○ | ○ |
|  | `enable` | ○ | ○ |
|  | `getBadgeBackgroundColor` | ○ | ○ |
|  | `getBadgeText` | ○ | ○ |
|  | `getBadgeTextColor` | ○ | ○ |
|  | `getTitle` | ○ | ○ |
|  | `getUserSettings` | ○ | ○ |
|  | `isEnabled` | ○ | ○ |
|  | `onClicked` | ○ | ○ |
|  | `openPopup` | ○ | ○ |
|  | `setBadgeBackgroundColor` | ○ | ○ |
|  | `setBadgeText` | ○ | ○ |
|  | `setBadgeTextColor` | ○ | ○ |
|  | `setIcon` | ○ | ○ |
|  | `setTitle` | ○ | ○ |
| `alarms` | `clear` | △ | ○ |
|  | `clearAll` | △ | ○ |
|  | `create` | △ | ○ |
|  | `onAlarm` | △ | ○ |
| `bookmarks` | `get` | ○ | ○ |
|  | `getChildren` | ○ | ○ |
|  | `getSubTree` | ○ | ○ |
|  | `getTree` | ○ | ○ |
|  | `search` | ○ | ○ |
| `commands` | `getAll` | ○ | ○ |
|  | `onCommand` | ○ | △※ |
| `contextMenus` | `create` | ○ | ○ |
|  | `onClicked` | ○ | ○ |
|  | `remove` | ○ | ○ |
|  | `removeAll` | ○ | ○ |
|  | `update` | ○ | ○ |
| `declarativeNetRequest` | `getDynamicRules` | ○ | ○ |
|  | `getEnabledRulesets` | ○ | ○ |
|  | `getSessionRules` | ○ | ○ |
|  | `updateDynamicRules` | × | ○ |
|  | `updateEnabledRulesets` | ○ | ○ |
|  | `updateSessionRules` | × | ○ |
| `downloads` | `cancel` | ○ | ○ |
|  | `download` | ○ | ○※ |
|  | `erase` | ○ | ○ |
|  | `onChanged` | ○ | ○ |
|  | `onCreated` | ○ | ○※ |
|  | `onErased` | ○ | ○ |
|  | `pause` | ○ | ○ |
|  | `resume` | ○ | ○ |
|  | `search` | ○ | ○ |
|  | `show` | ○ | ○ |
| `fontSettings` | `getFontList` | ○ | ○ |
| `history` | `onVisitRemoved` | ○ | ○ |
|  | `onVisited` | ○ | ○ |
|  | `search` | ○ | ○ |
| `identity` | `clearAllCachedAuthTokens` | △ | × |
|  | `getAuthToken` | △ | × |
|  | `getProfileUserInfo` | △ | ○※ |
|  | `getRedirectURL` | △ | ○ |
|  | `launchWebAuthFlow` | △ | △※ |
|  | `removeCachedAuthToken` | △ | × |
| `idle` | `onStateChanged` | △ | ○ |
|  | `setDetectionInterval` | △ | ○ |
| `management` | `createAppShortcut` | × | × |
|  | `generateAppForLink` | × | × |
|  | `installReplacementWebApp` | × | × |
|  | `launchApp` | × | × |
|  | `setEnabled` | × | × |
|  | `setLaunchType` | × | × |
|  | `uninstall` | × | × |
|  | `uninstallSelf` | × | × |
| `notifications` | `clear` | ○ | ○ |
|  | `create` | ○ | ○ |
|  | `getAll` | ○ | ○ |
|  | `getPermissionLevel` | ○ | ○ |
|  | `onClicked` | ○ | △※ |
|  | `onClosed` | ○ | ○ |
|  | `update` | ○ | ○ |
| `offscreen` | `closeDocument` | △ | ○ |
|  | `createDocument` | △ | ○ |
|  | `hasDocument` | △ | ○ |
| `permissions` | `contains` | ○ | ○ |
|  | `getAll` | ○ | ○ |
|  | `request` | ○ | ○※ |
| `runtime` | `getContexts` | △ | ○ |
|  | `onConnect` | ○ | ○ |
|  | `onInstalled` | ○ | ○ |
|  | `onMessage` | ○ | ○ |
|  | `onStartup` | ○ | ○ |
|  | `onUserScriptMessage` | △ | ×※ |
|  | `openOptionsPage` | ○ | ○ |
| `scripting` | `executeScript` | △ | △ |
|  | `getRegisteredContentScripts` | △ | ○ |
|  | `insertCSS` | △ | ○ |
|  | `registerContentScripts` | △ | ○ |
|  | `removeCSS` | △ | ○ |
|  | `unregisterContentScripts` | △ | ○ |
| `search` | `query` | ○ | ○ |
| `sessions` | `restore` | ○ | ○ |
| `sidePanel` | `close` | ○ | ○ |
|  | `getOptions` | ○ | ○ |
|  | `getPanelBehavior` | ○ | ○ |
|  | `onClosed` | ○ | ○ |
|  | `onOpened` | ○ | ○ |
|  | `open` | ○ | ○ |
|  | `setOptions` | ○ | ○ |
|  | `setPanelBehavior` | ○ | ○ |
| `storage` | `onChanged` | ○ | ○ |
|  | `sync` | ○ | ○ |
| `tabs` | `captureVisibleTab` | ○ | ○ |
|  | `create` | ○ | ○ |
|  | `duplicate` | ○ | ○ |
|  | `get` | ○ | ○ |
|  | `getZoom` | ○ | ○ |
|  | `move` | ○ | ○ |
|  | `onActivated` | ○ | ○ |
|  | `onCreated` | ○ | ○ |
|  | `onHighlighted` | ○ | ○ |
|  | `onMoved` | ○ | ○ |
|  | `onRemoved` | ○ | ○ |
|  | `onUpdated` | ○ | ○ |
|  | `onZoomChange` | ○ | ○ |
|  | `query` | ○ | ○ |
|  | `reload` | ○ | ○ |
|  | `remove` | ○ | ○ |
|  | `sendMessage` | ○ | ○ |
|  | `setZoom` | ○ | ○ |
|  | `update` | ○ | ○ |
| `topSites` | `get` | ○ | ○ |
| `userScripts` | `configureWorld` | △ | × |
|  | `getScripts` | △ | × |
|  | `getWorldConfigurations` | △ | × |
|  | `register` | △ | × |
|  | `resetWorldConfiguration` | △ | × |
|  | `unregister` | △ | × |
|  | `update` | △ | × |
| `webNavigation` | `getAllFrames` | ○ | ○ |
|  | `onCommitted` | ○ | ○ |
|  | `onCompleted` | ○ | ○ |
|  | `onDOMContentLoaded` | ○ | ○ |
|  | `onHistoryStateUpdated` | ○ | ○ |
|  | `onReferenceFragmentUpdated` | ○ | ○ |
|  | `onTabReplaced` | ○ | ○ |
| `windows` | `getAll` | ○ | ○ |
|  | `getCurrent` | ○ | ○ |
|  | `onFocusChanged` | ○ | ○ |
|  | `update` | ○ | ○ |
<!-- END -->

注: 表はテスト（`tst_cdpshims`）が互換レイヤーから作って文書と照合しており、2026-10-03 の時点の状態。Edge 列のうち WebView2 自身が答えるものは、2026-10-03 に実機（WebView2 154.0.4258.48）で確かめた。

---

（Deodorized. This tree is published with its source comments
and internal annotations stripped; the diff against the original
is [deodorant.zip](deodorant.zip).）

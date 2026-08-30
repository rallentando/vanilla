# WebView2 SDK (vendored)

`EdgeWebView` hosts the Edge WebView2 control itself, instead of going through
Qt WebView, so it needs the SDK's headers and loader (ROADMAP C-7, D-166).

Taken from the NuGet package, unmodified:

| | |
|---|---|
| package | `Microsoft.Web.WebView2` |
| version | **1.0.4129.50** |
| source | `https://api.nuget.org/v3-flatcontainer/microsoft.web.webview2/1.0.4129.50/microsoft.web.webview2.1.0.4129.50.nupkg` |
| package SHA256 | `D3934F482D484B89FB4825DF720C710664E1143A1E90F7B3A60794EF33F473D2` |
| taken on | 2026-08-20 |

`LICENSE.txt` and `NOTICE.txt` are the package's own. They are kept here
rather than fetched with the rest, because a build which redistributes
`WebView2Loader.dll` has to carry them.

## What belongs here, and what does not

| file | from the package | SHA256 |
|---|---|---|
| `include/WebView2.h` | `build/native/include/` | `DFF1E3181EC7EC203A34EF6EFA966590E0EF0BA1A5C3FE3B69DA6508C2F8A02E` |
| `include/WebView2EnvironmentOptions.h` | `build/native/include/` | `06F44F0569F1415C37CCD9EB6BADE28B803646A73EDCE78B40F7AA8548D015B9` |
| `x64/WebView2Loader.dll` | `build/native/x64/` | `A9A09232C25805323D4CFB3FC8F545A190A9C8A99C93262EA99D0B88DF99EC90` |
| `x64/WebView2Loader.dll.lib` | `build/native/x64/` | `BFC8CCAAA056BE95243A5B66A827E5849D2BB39676FCA4DCC2053796D8E15C6D` |

**x64 only.** The build looks for this directory and turns `EDGEWEBVIEW` off
when it is not there, so a tree without it still builds -- without that view.
For x86 or ARM64, add the matching directory from the same package version and
give it to both `CMakeLists.txt` and `vanilla.pro`.

**`WebView2LoaderStatic.lib` is deliberately not here.** It is 10.4MB against
the DLL's 161KB, and the DLL is a shim which finds the installed Edge runtime;
the size is the whole difference. The build copies the DLL next to
`vanilla.exe` and the install rules put it beside the executable.

**The Edge WebView2 runtime is not here either** and is not ours to ship. It is
what `msedgewebview2.exe` comes from; a machine without it gets the view's
`Failed` state and a message, not a crash.

## Updating

Take the same four files from a newer package -- and `LICENSE.txt` /
`NOTICE.txt` too, if that package's have changed -- replace the versions and
the hashes above, and rebuild. Nothing in this directory is edited by hand.

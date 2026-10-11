# QtKeychain (vendored)

A WebEngine view keeps the passwords it saves in the OS's own store -- the
Windows credential manager, the macOS keychain, the Secret Service or KWallet
on Linux -- through QtKeychain. Nothing else in vanilla uses it.

Taken from the release tag, unmodified. Every file in this directory except
this one is upstream's.

| | |
|---|---|
| project | `https://github.com/frankosterfeld/qtkeychain` |
| version | **0.14.0** (tag `v0.14.0`) |
| source | `https://github.com/frankosterfeld/qtkeychain/archive/refs/tags/v0.14.0.tar.gz` |
| archive SHA256 | `AD78F2D9E0A5EE80F40CBAACE4F55D11C2557BC6E930CF4A9479E39A7D78BB60` |
| licence | modified BSD (`COPYING`), installed under `licenses/qtkeychain` |
| taken on | 2026-10-07 |

## How it is built

Not with its own `CMakeLists.txt`: that one installs its headers, its library
and its CMake package files, and they would end up in vanilla's install. The
top-level `CMakeLists.txt` builds a static `vanilla_qtkeychain` from the same
sources and definitions per platform:

| platform | sources | store |
|---|---|---|
| Windows | `keychain.cpp`, `keychain_win.cpp` with `USE_CREDENTIAL_STORE` | credential manager |
| macOS | `keychain.cpp`, `keychain_apple.mm` | keychain |
| Linux | `keychain.cpp`, `keychain_unix.cpp`, `gnomekeyring.cpp`, `libsecret.cpp`, `plaintextstore.cpp`, the KWallet D-Bus interface | Secret Service (libsecret, when found), GNOME Keyring or KWallet |

Where none of these can be built (Linux without Qt D-Bus, other platforms),
the target is left out, `VANILLA_PASSWORDS` is not defined, and a WebEngine
view does not save passwords. The translations are not built: no message of
the library is shown.

On Windows the store names an entry by its key alone, without the service.
The key vanilla makes starts with `vanilla-<digest of the executable's
directory>`, which is what keeps two installations apart.

## Updating

Replace every upstream file with a newer tag's, update the version and the
hash above, and check that the source lists in `CMakeLists.txt` still match
upstream's own `CMakeLists.txt`.

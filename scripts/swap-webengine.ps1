<#
    Swap the QtWebEngine inside the Qt installation, keeping a way back.

        .\swap-webengine.ps1 -To selfbuilt   # the local build, with H.264 / AAC
        .\swap-webengine.ps1 -To official    # what the MaintenanceTool installs
        .\swap-webengine.ps1 -To selfbuilt -WhatIf   # say what it would do

    Why this exists: the development machine keeps two QtWebEngine builds, and
    which one is installed changes what the browser can do -- see BUILD.md,
    "この環境の QtWebEngine". Engine level bugs have to be told apart from
    "this build of the engine" bugs, and that means swapping back and forth
    (ROADMAP A-8 was settled exactly that way).

    The first swap to 'selfbuilt' copies the official counterpart of every
    file it is about to overwrite into the backup directory, in the same
    relative layout, and lists any file that had no counterpart. Going back
    restores the first and deletes the second.

    Paths are the ones on the development machine. Change the three below to
    use it anywhere else.
#>
param(
    [ValidateSet("selfbuilt","official")][string]$To = "selfbuilt",
    [switch]$WhatIf
)

$qt      = "C:\Qt\6.12.0\msvc2022_64"
$staging = "C:\qtwe-staging"          # output of the local build (C:\qtwe)
$backup  = "C:\qtwe-official-backup"  # made on the first swap

$added = Join-Path $backup "added-by-selfbuild.txt"

function Show-Dll {
    param([string]$why)
    $d = Join-Path $qt "bin\Qt6WebEngineCore.dll"
    if(-not (Test-Path $d)){ "$why : (no dll)"; return }
    $h = (Get-FileHash $d).Hash
    $name = switch ($h) {
        "65BE5235AA1F0FA3EEEA0268AD2492BD9DF94D8A37680493ADD1661361FA9181" { "self built (Qt 6.12.0 / WebEngine 6.140.0, patched: WAR + isolated world eval + IME to active widget, 2026-10-02)" }
        "475A605D6A2A2A089F3C6AAD4B072BD0F14265D1974B47C5068C98BE655979AD" { "official (Qt 6.12.0 / WebEngine 6.140.0)" }
        # Qt 6.11.2
        "3E977233F5DBE7FAFD099CB3C490DD1C21AB20B0B5C092B192F4247B828527C4" { "self built (patched: WAR + isolated world eval + IME to active widget, 2026-09-25)" }
        "124EEE96BE6EC896BD7287E517708A67A2CA29E9F3BD29B4C41AB23277DDF6D2" { "self built (patched: WAR + isolated world eval, 2026-09-23)" }
        "ECCDF8D52EBE122CC18472625EA0A676B080C6E4BDBF555D70547E1D720F1F75" { "self built (patched: WAR, 2026-09-22)" }
        "1E63D1738F1AF3078AAD45AFFB810683C15162EFC1E23316F01BF3D1E712B34E" { "self built (unpatched, 2026-09-16)" }
        "D6995CE685FEC54B28B3E2BD7C891666E3DFF825696C30EF46B009EF5EB1A831" { "official" }
        default { "unknown -- update the hashes in this script" }
    }
    "$why : $name ($($h.Substring(0,8))...)"
}

# a running Qt application holds the dlls open and the copy fails halfway,
# which leaves a half swapped installation. So: refuse instead.
$busy = Get-Process -ErrorAction SilentlyContinue |
        Where-Object { $_.Path -like "$qt\*" -or $_.Path -like "*vanilla-build*" }
if($busy){
    "these are holding Qt open, stop them first:"
    $busy | Select-Object Id, Path
    exit 1
}

Show-Dll "before"

if($To -eq "selfbuilt"){
    if(-not (Test-Path $staging)){ "no staging tree at $staging"; exit 1 }
    $files = Get-ChildItem $staging -Recurse -File

    # "already backed up" means it holds files, not merely that the directory
    # is there -- a -WhatIf run leaves the empty skeleton behind.
    $haveBackup = (Test-Path $backup) -and
                  ((Get-ChildItem $backup -Recurse -File -ErrorAction SilentlyContinue).Count -gt 0)

    if(-not $haveBackup){
        New-Item -ItemType Directory -Path $backup -Force | Out-Null
        $additions = @()
        foreach($f in $files){
            $rel = $f.FullName.Substring($staging.Length + 1)
            $src = Join-Path $qt $rel
            if(Test-Path $src){
                $dst = Join-Path $backup $rel
                New-Item -ItemType Directory -Path (Split-Path $dst) -Force | Out-Null
                if(-not $WhatIf){ Copy-Item $src $dst -Force }
            } else {
                $additions += $rel
            }
        }
        if(-not $WhatIf){ $additions | Set-Content $added -Encoding utf8 }
        "backed up the official files to $backup " +
            "($($files.Count - $additions.Count) files, $($additions.Count) had no counterpart)"
    } else {
        "backup already exists at $backup, leaving it alone"
    }

    foreach($f in $files){
        $rel = $f.FullName.Substring($staging.Length + 1)
        $dst = Join-Path $qt $rel
        New-Item -ItemType Directory -Path (Split-Path $dst) -Force | Out-Null
        if(-not $WhatIf){ Copy-Item $f.FullName $dst -Force }
    }
    "copied $($files.Count) files from $staging"
}
else {
    if(-not (Test-Path $backup)){ "no backup at $backup, cannot go back"; exit 1 }
    $files = Get-ChildItem $backup -Recurse -File | Where-Object { $_.FullName -ne $added }
    foreach($f in $files){
        $rel = $f.FullName.Substring($backup.Length + 1)
        $dst = Join-Path $qt $rel
        New-Item -ItemType Directory -Path (Split-Path $dst) -Force | Out-Null
        if(-not $WhatIf){ Copy-Item $f.FullName $dst -Force }
    }
    "restored $($files.Count) official files"
    if(Test-Path $added){
        $n = 0
        foreach($rel in (Get-Content $added | Where-Object { $_.Trim() })){
            $dst = Join-Path $qt $rel
            if(Test-Path $dst){ if(-not $WhatIf){ Remove-Item $dst -Force }; $n++ }
        }
        "removed $n files the self build had added"
    }
}

Show-Dll "after"
"remember to rebuild vanilla and remake the package: the engine dlls are deployed into it"
"and delete the GPUCache of every vanilla profile that will run the new engine (same exe path ="
"same profile = same shader cache, and a cache made by the other build flickers -- BUILD.md):"
'  Get-ChildItem "$env:APPDATA\vanilla\QtWebEngine" -Directory -Recurse -Filter GPUCache | Remove-Item -Recurse -Force'

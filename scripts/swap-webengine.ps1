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

$qt      = "C:\Qt\6.11.1\msvc2022_64"
$staging = "C:\qtwe-staging"          # output of the local build (C:\qtwe)
$backup  = "C:\qtwe-official-backup"  # made on the first swap

$added = Join-Path $backup "added-by-selfbuild.txt"

function Show-Dll {
    param([string]$why)
    $d = Join-Path $qt "bin\Qt6WebEngineCore.dll"
    if(-not (Test-Path $d)){ "$why : (no dll)"; return }
    $h = (Get-FileHash $d).Hash
    $name = switch ($h) {
        "B3E99957A940480E0F5E0F00E1B16F70ABC0887781BECFE248EEA4187CC00F6B" { "self built" }
        "02FFF5F4AC7C449A2EB541BC81CB840419339A367EEDC4ED713BB04BE84D340C" { "official" }
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

<#
    Move an installation's state into 'data/<MD5(実行ディレクトリ)>/' (D-100).

        .\split-state-directory.ps1 -ExeDir "C:\Program Files\vanilla" -WhatIf
        .\split-state-directory.ps1 -ExeDir "C:\Program Files\vanilla"
        .\split-state-directory.ps1 -All -WhatIf

    Why by hand: for this one change the migration was left out of the
    application (D-100; the usual thing is to do it in there, as every
    earlier move of a directory did). The new build looks in the new place,
    and if nothing is there it starts with a clean state -- the old files are
    still where they were, nothing is lost, but the session and the settings
    will look empty until this script has run. So run it **before** starting
    a build that has D-100 in it.

    What moves: '*.json' and '*.xml' at the top of 'data/' (the six state
    files and every dated backup), and the 'image/' and 'history/'
    directories. What stays: 'webengine/', 'webenginecache/' and
    'edgewebview/' (D-258), whose own
    names already carry the same hash.

    The move is a rename within one volume. To undo it, move the contents of
    the hash directory back up one level.
#>
param(
    [string]$ExeDir,
    [switch]$All,
    [switch]$WhatIf
)

$ErrorActionPreference = "Stop"

# every executable the development machine has seen. '-All' walks these and
# skips the ones with no 'data/'.
$knownExeDirs = @(
    "C:\Program Files\vanilla",
    "C:\Users\mater\vanilla-build\release",
    "C:\Users\mater\vanilla-build\debug",
    "C:\Users\mater\vanilla-build\package"
)

# 'Application::BaseDirectory()': beside the executable, unless it sits in a
# UAC protected directory, and then '<AppLocalDataLocation>'.
function Get-BaseDirectory {
    param([string]$exeDir)
    $d = $exeDir.Replace('\','/').TrimEnd('/') + '/'
    foreach($protected in @('C:/Windows/','C:/Program Files/','C:/Program Files (x86)/')){
        if($d.StartsWith($protected)){
            return (Join-Path $env:LOCALAPPDATA 'vanilla') + '\'
        }
    }
    return $d.Replace('/','\')
}

# 'QCoreApplication::applicationDirPath()' is forward slashed and has no
# trailing slash; the hash is of exactly that, in UTF-8, lower case hex.
function Get-StateHash {
    param([string]$exeDir)
    $path = $exeDir.Replace('\','/').TrimEnd('/')
    $md5  = [System.Security.Cryptography.MD5]::Create()
    $sum  = $md5.ComputeHash([System.Text.Encoding]::UTF8.GetBytes($path))
    return -join ($sum | ForEach-Object { $_.ToString("x2") })
}

function Split-One {
    param([string]$exeDir)

    $data = (Get-BaseDirectory $exeDir) + 'data\'
    if(-not (Test-Path $data)){ "skip  $exeDir : no '$data'"; return }

    $hash  = Get-StateHash $exeDir
    $state = Join-Path $data $hash

    $files = @(Get-ChildItem $data -File -Filter *.json -EA SilentlyContinue) +
             @(Get-ChildItem $data -File -Filter *.xml  -EA SilentlyContinue)
    $dirs  = @('image','history') | Where-Object { Test-Path (Join-Path $data $_) }

    if($files.Count -eq 0 -and $dirs.Count -eq 0){
        "skip  $exeDir : nothing left at the top of '$data'"
        return
    }

    "move  $exeDir"
    "      $data"
    "   -> $state\   ($($files.Count) files, $($dirs.Count) directories)"

    if($WhatIf){ return }

    # a live instance holds its json files open; moving them under it would
    # leave it writing into the old place.
    $running = Get-Process vanilla -EA SilentlyContinue |
               Where-Object { $_.Path -and (Split-Path $_.Path) -ieq $exeDir.TrimEnd('\') }
    if($running){ throw "vanilla is running from $exeDir (pid $($running.Id)). Close it first." }

    if(-not (Test-Path $state)){ New-Item -ItemType Directory $state | Out-Null }

    foreach($f in $files){
        Move-Item $f.FullName (Join-Path $state $f.Name) -Force
    }
    foreach($d in $dirs){
        $to = Join-Path $state $d
        if(Test-Path $to){ throw "$to already exists; merge it by hand" }
        Move-Item (Join-Path $data $d) $to
    }
    "      done"
}

if($All){
    foreach($d in $knownExeDirs){ Split-One $d }
} elseif($ExeDir){
    Split-One $ExeDir
} else {
    "give -ExeDir <path> or -All. -WhatIf says what it would do."
}

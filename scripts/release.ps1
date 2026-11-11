# Publishes the co-op as a release of the fork (remote "origin", alosoygetsu/asuka-link): the launcher's
# Update button reads that repo's releases (launcher/release.c REPO) and downloads dw2003-native-<tag>.zip.
# Versions: the co-op's own, semver from v1.0.0 (-Bump minor for new things, the default; patch for fixes only; major
# for a big change), or -Version 1.2.3. Each release names the port it is based on: Feerdus's v0.18.9, frozen since
# v1.0.0 (its tags are kept as upstream/vX.Y.Z, history only). Before v1.0.0 the versions were v0.18.901-918;
# the launcher only takes vX.Y.Z and must see them in order, and v1.0.0 is past every one of those.
# .\native\scripts\release.ps1 -DryRun builds the zips without tagging or publishing.
param([switch]$DryRun, [string]$Notes = "", [ValidateSet("major", "minor", "patch")][string]$Bump = "minor", [string]$Version = "")
$ErrorActionPreference = "Stop"
$root = Split-Path (Split-Path $PSScriptRoot)  # the project's folder: native\, toolchain\, dw3_decomp\
$native = "$root\native"
$fork = "alosoygetsu/asuka-link"

function Step($text) { Write-Host "== $text" -ForegroundColor Cyan }
function Fail($text) { Write-Host "FALLO: $text" -ForegroundColor Red; exit 1 }
function Version($t) { if ($t -match '^v(\d+)\.(\d+)\.(\d+)$') { [version]"$($Matches[1]).$($Matches[2]).$($Matches[3])" } }
function Own($t) { $v = Version $t; [bool]($v -and $v.Major -ge 1) }  # the co-op's tags, v1.0.0 on

if ((git -C $native rev-parse --abbrev-ref HEAD) -ne "coop") { Fail "native\ no está en la rama coop" }
if ((git -C $native status --porcelain --untracked-files=no) -ne $null) { Fail "hay cambios sin commit en native\" }

$base = "v0.18.9"  # the port Asuka Link is based on, frozen since v1.0.0
$here = git -C $native tag --points-at HEAD | Where-Object { Own $_ } | Select-Object -First 1
$last = git -C $native tag -l "v*" | Where-Object { Own $_ } | Sort-Object { Version $_ } -Descending | Select-Object -First 1
if ($here) {
    $tag = $here  # this commit has its tag already (a release that failed halfway): publish it again
} elseif ($Version) {
    if (-not (Own "v$Version")) { Fail "-Version tiene que ser X.Y.Z, de 1.0.0 en adelante" }
    if ($last -and (Version "v$Version") -le (Version $last)) { Fail "v$Version no es mayor que $last" }
    $tag = "v$Version"
} elseif (-not $last) {
    $tag = "v1.0.0"  # the first of the co-op's own
} else {
    $v = Version $last
    $tag = switch ($Bump) {
        "major" { "v{0}.0.0" -f ($v.Major + 1) }
        "minor" { "v{0}.{1}.0" -f $v.Major, ($v.Minor + 1) }
        "patch" { "v{0}.{1}.{2}" -f $v.Major, $v.Minor, ($v.Build + 1) }
    }
}
$net = (Select-String -Path "$native\platform\coop_net.c" -Pattern '^#define COOP_VERSION (\d+)').Matches[0].Groups[1].Value
Step "Release $tag del cooperativo, sobre el port $base (red: versión $net)"
if (-not $Notes -and -not (Select-String -Path "$native\CHANGELOG-COOP.md" -Pattern "^## $([regex]::Escape($tag))\s*$" -Quiet)) {
    Fail "CHANGELOG-COOP.md no tiene la sección '## $tag': escríbela antes"
}

if (-not $DryRun -and -not $here) { git -C $native tag -a $tag -m "Asuka Link $tag, on the port's $base"; if ($LASTEXITCODE) { Fail "tag" } }

Step "Compilando juego y launcher"
& "$PSScriptRoot\build.ps1"; if ($LASTEXITCODE -ne 0) { Fail "compilación" }
$env:LLVM_MINGW = "$root\toolchain\llvm-mingw-20250114-ucrt-x86_64"
$env:PATH = "$env:LLVM_MINGW\bin;C:\Program Files\Git\usr\bin;$env:PATH"
Push-Location $native
try {
    # the launcher's version is git describe's tag: without -DryRun, the new one
    python -I tools\build_launcher.py; if ($LASTEXITCODE -ne 0) { Fail "launcher" }
    Step "Empaquetando"
    $out = "$native\dist\release"
    New-Item -ItemType Directory -Force $out | Out-Null
    Remove-Item "$out\*" -ErrorAction SilentlyContinue
    python -I tools\package.py; if ($LASTEXITCODE -ne 0) { Fail "package.py" }
    $update = "$out\dw2003-native-$tag.zip"   # the launcher's update: no native.ini, so the player's stays
    Copy-Item "$native\dist\dw2003-native.zip" $update
    # its SHA-256, which the launcher (v1.1.0 on) checks before installing it (launcher/self_update.c)
    $sum = (Get-FileHash $update -Algorithm SHA256).Hash.ToLower()
    [System.IO.File]::WriteAllText("$update.sha256", "$sum  dw2003-native-$tag.zip`n")
    python -I tools\package_coop.py; if ($LASTEXITCODE -ne 0) { Fail "package_coop.py" }
    $install = "$out\asuka-link-$tag.zip"    # the first install: with the co-op's native.ini and LEEME-COOP.txt
    Copy-Item "$native\dist\dw2003-coop.zip" $install
} finally { Pop-Location }
Get-ChildItem $out | ForEach-Object { Write-Host ("  {0} ({1:N1} MB)" -f $_.Name, ($_.Length / 1MB)) }
if ($DryRun) { Step "DryRun: ni tag ni publicación"; exit 0 }

Step "Publicando en $fork"
git -C $native push -q origin coop; if ($LASTEXITCODE) { Fail "push de coop" }
git -C $native push -q origin $tag; if ($LASTEXITCODE) { Fail "push del tag" }
if (-not $Notes) {
    # this version's section of CHANGELOG-COOP.md ("## vX.Y.Z" up to the next "## ")
    $log = Get-Content "$native\CHANGELOG-COOP.md" -Raw -Encoding UTF8
    $changes = if ($log -match "(?s)## $([regex]::Escape($tag))\s*\r?\n(.*?)(?=\r?\n## |\z)") { $Matches[1].Trim() } else { "" }
    if (-not $changes) { Fail "CHANGELOG-COOP.md no tiene la sección '## $tag': escríbela antes" }
    $Notes = @"
$changes

---
- **Asuka Link $tag**, el cooperativo de alosoygetsu: lo de arriba.
- **Basado en el port nativo $base**, de Feerdus ([Feerdus95/dw3-native-port](https://github.com/Feerdus95/dw3-native-port), [sus cambios](https://github.com/Feerdus95/dw3-native-port/releases/tag/$base)): el juego nativo en sí, el launcher y sus mejoras.
- **Red: versión $net.** Para jugar juntos, todos tenéis que tener la misma.

- **Si ya lo tienes instalado:** en el launcher, *Buscar actualizaciones* y luego *Actualizar*. Tu ``native.ini`` y tus partidas no se tocan.
- **Primera vez:** descarga ``asuka-link-$tag.zip`` y sigue ``LEEME-COOP.txt``.

``dw2003-native-$tag.zip`` es el que usa el launcher para actualizar. Ninguno lleva nada del juego: cada uno usa su propio disco.
"@
}
# the notes through a file: Windows PowerShell splits an argument at its own quotes ("...") on the way to gh
$notesFile = [System.IO.Path]::GetTempFileName()
[System.IO.File]::WriteAllText($notesFile, $Notes, (New-Object System.Text.UTF8Encoding $false))
gh release create $tag --repo $fork --target coop --title "Asuka Link $tag (port $base)" --notes-file $notesFile $update "$update.sha256" $install
$created = $LASTEXITCODE
Remove-Item $notesFile
$LASTEXITCODE = $created
if ($LASTEXITCODE) { Fail "gh release create" }
Step "Publicada: https://github.com/$fork/releases/tag/$tag"

# Una prueba sin tocar el teclado (solo para nosotros): prepara una copia del juego (test5 junto a native\, o -Copy),
# carga una partida si se da, salta a un mapa (--field), hace lo del caso, espera su línea en el log, enseña solo lo
# que importa y cierra esa copia. native\build solo se lee: ni su native.ini ni sus partidas se tocan, y su juego
# nunca se cierra.
#   powershell -File native\scripts\probar.ps1 digilog -Save D:\ruta\card1.mcd [-Slot 0..2]
#   powershell -File native\scripts\probar.ps1 campo -Field 0x22B -Progress 12 -Shot
#   powershell -File native\scripts\probar.ps1 campo -Press "200:triangle,260:down" -Until "algo del log" -Shot
# Casos:
#   digilog  abre el Digilog solo (DW3_TEST=digilog) y lo escribe en texto (tools/digilog_dump.py)
#   campo    llega al mapa y espera -Seconds (o -Until); falla si el juego se cae o se cierra
# Opciones: -Save tarjeta .mcd (sin ella, partida nueva con el equipo -Party), -Slot (si no, el último guardado),
#   -Field modo del mapa (si no, el de la partida o 0x230), -Progress paso de la historia, -Set "CODE=V,..." marcas y
#   objetos (main/game/events.c), -Press botones a destiempo (DW3_PRESS, "frame:botón,..."), -Lang 2..6 idioma,
#   -Until expresión del log que da la prueba por buena, -Shot captura reducida en test-shots\, -Keep no cierra,
#   -Log todas las líneas del juego.
param([Parameter(Position = 0)][ValidateSet("digilog", "campo")][string]$Case = "campo",
      [string]$Save, [ValidateRange(-1, 2)][int]$Slot = -1, [string]$Field, [int]$Progress = -1, [string]$Set,
      [int]$Party = 0, [string]$Press, [ValidateRange(0, 6)][int]$Lang = 0, [string]$Until, [int]$Seconds = 15,
      [int]$Timeout = 90, [string]$Copy = "test5", [switch]$Shot, [switch]$Keep, [switch]$Log)
$ErrorActionPreference = "Stop"
$root = Split-Path (Split-Path $PSScriptRoot)  # la carpeta del proyecto: native\, test2..test5
$build = "$root\native\build"
$dir = "$root\$Copy"
if ($Copy -notmatch '^test\d$') { throw "-Copy tiene que ser una copia de prueba (test2..test5)" }

# la partida: el bloque del juego en la tarjeta (como launcher/team.c), su ranura y su mapa
$saveArg = $null
if ($Save) {
    $d = [IO.File]::ReadAllBytes((Resolve-Path $Save))
    $base = -1
    for ($f = 1; $f -lt 16 -and $base -lt 0; $f++) {
        if (($d[$f * 128] -band 0xF0) -eq 0x50 -and [Text.Encoding]::ASCII.GetString($d, $f * 128 + 0xA, 20) -eq "BESLES-03936DMW3-EUR") { $base = $f * 0x2000 }
    }
    if ($base -lt 0) { throw "$Save no tiene partida de DW2003" }
    if ($Slot -lt 0) { $Slot = if ([Text.Encoding]::ASCII.GetString($d, $base + 0x204, 4) -eq "DMW3") { $d[$base + 0x201] } else { 0 } }
    $off = $base + 0x300 + $Slot * 0x2700
    $New = "$dir\prueba.mcd"
    $saveArg = "prueba.mcd@0x{0:X}" -f $off
    if (-not $Field) { $m = [BitConverter]::ToInt32($d, $off + 0x34); if ($m -ge 0x200 -and $m -le 0x2FF) { $Field = "0x{0:X}" -f $m } }
}
if (-not $Field) { $Field = "0x230" }

# la copia: el ejecutable de native\build, los datos enlazados, un native.ini en ventana pequeña sin cooperativo
$p = Get-Process dw2003 -ErrorAction SilentlyContinue | Where-Object { (Split-Path $_.Path) -eq $dir }
if ($p) { $p | Stop-Process; Start-Sleep 1 }
New-Item -ItemType Directory -Force "$dir\saves" | Out-Null
if (-not (Test-Path "$dir\data")) { New-Item -ItemType Junction -Path "$dir\data" -Target "$build\data" | Out-Null }
Copy-Item "$build\dw2003.exe", "$build\dw2003.fill", "$build\SDL3.dll" $dir -Force
if ($Save) { Copy-Item $Save $New -Force }
$ini = (Get-Content "$build\native.ini" | Where-Object { $_ -notmatch '^\s*(coop\w*|scale|fullscreen|debug)\s*=' }) -join "`n"
Set-Content "$dir\native.ini" "$ini`ncoop = 0`nscale = 1`nfullscreen = 0`ndebug = 1`n" -NoNewline
$out = "$dir\prueba.log"
Remove-Item $out, "$dir\prueba.out" -ErrorAction SilentlyContinue

$env:DW3_TEST = @{ digilog = "digilog"; campo = "" }[$Case]
$env:DW3_COOP_SAVE = $saveArg
$env:DW3_PRESS = $Press
$env:DW3_LANGUAGE = if ($Lang) { "$Lang" } else { $null }
$gameArgs = @("--field", $Field, "--party", "$Party")
if ($Progress -ge 0) { $gameArgs += "--progress", "$Progress" }
if ($Set) { $Set.Split(",") | ForEach-Object { $gameArgs += "--set", $_.Trim() } }
$game = Start-Process "$dir\dw2003.exe" -ArgumentList $gameArgs -WorkingDirectory $dir -PassThru `
    -RedirectStandardError $out -RedirectStandardOutput "$dir\prueba.out"
Remove-Item Env:DW3_TEST, Env:DW3_COOP_SAVE, Env:DW3_PRESS, Env:DW3_LANGUAGE -ErrorAction SilentlyContinue

$done = @{ digilog = 'DIGILOG end'; campo = $null }[$Case]
if ($Until) { $done = $Until }
$start = Get-Date; $inField = $null; $result = $null
while (-not $result) {
    Start-Sleep -Milliseconds 500
    $text = if (Test-Path $out) { Get-Content $out -Raw -ErrorAction SilentlyContinue } else { "" }
    if ($text -match 'CRASH') { $result = "SE HA CAÍDO" }
    elseif ($game.HasExited) { $result = "SE HA CERRADO (código $($game.ExitCode))" }
    elseif ($done -and $text -match $done) { $result = "bien" }
    elseif (-not $done -and $text -match 'field: mode') {
        if (-not $inField) { $inField = Get-Date }
        if (((Get-Date) - $inField).TotalSeconds -ge $Seconds) { $result = "bien" }
    }
    elseif (((Get-Date) - $start).TotalSeconds -ge $Timeout) { $result = "SIN TERMINAR ($Timeout s)" }
}
$ok = $result -eq "bien"
Write-Host "== $Case ($Copy, mapa $Field$(if ($saveArg) { ", partida ranura $Slot" })): $result en $([int]((Get-Date) - $start).TotalSeconds) s" -ForegroundColor $(if ($ok) { "Green" } else { "Red" })

if ($Shot -or -not $ok) {  # la ventana (scale = 1: 320x240)
    Add-Type -Name Shot -Namespace Probar -MemberDefinition @'
[DllImport("user32.dll")] public static extern bool GetWindowRect(System.IntPtr h, out RECT r);
[DllImport("user32.dll")] public static extern bool PrintWindow(System.IntPtr h, System.IntPtr dc, uint flags);
public struct RECT { public int L, T, R, B; }
'@
    Add-Type -AssemblyName System.Drawing
    $game.Refresh()
    if (-not $game.HasExited -and $game.MainWindowHandle -ne 0) {
        $r = New-Object Probar.Shot+RECT; [Probar.Shot]::GetWindowRect($game.MainWindowHandle, [ref]$r) | Out-Null
        $w = $r.R - $r.L; $h = $r.B - $r.T
        $bmp = New-Object System.Drawing.Bitmap $w, $h
        $g = [System.Drawing.Graphics]::FromImage($bmp); $dc = $g.GetHdc()
        [Probar.Shot]::PrintWindow($game.MainWindowHandle, $dc, 2) | Out-Null; $g.ReleaseHdc($dc); $g.Dispose()
        $k = if ($w -gt 700) { 2 } else { 1 }  # una ventana grande, a la mitad
        $small = New-Object System.Drawing.Bitmap $bmp, ([int]($w / $k)), ([int]($h / $k))
        New-Item -ItemType Directory -Force "$root\test-shots" | Out-Null
        $png = "$root\test-shots\$Case.png"; $small.Save($png); $small.Dispose(); $bmp.Dispose()
        Write-Host "   captura: $png"
    }
}
if (-not $Keep -and -not $game.HasExited) { $game | Stop-Process; $game.WaitForExit(5000) | Out-Null }

# lo que importa del log
if ($Log) { Get-Content $out }
elseif ($Case -eq "digilog" -and $ok) { python -I "$root\native\tools\digilog_dump.py" $out }
else {
    Get-Content $out | Select-String 'CRASH|error|fail|can.t|test:|field:|refused' | Select-Object -Last 15 | ForEach-Object { "   $($_.Line)" }
    if (-not $ok) { "   -- las últimas líneas:"; Get-Content $out -Tail 10 | ForEach-Object { "   $_" } }
}
if (-not $ok) { exit 1 }

# Pruebas del cooperativo en local (solo para nosotros): 2 a 4 copias del juego en 127.0.0.1, en test2..test5 junto a
# native\, que se juntan en el mismo mapa y, según el modo, se ven, combaten, hacen una raid o juegan un duelo de
# cartas solas (DW3_COOP_AUTO). Espera a que todas apunten su resultado en coop.log; si los logs se quedan quietos
# demasiado (40 s; 180 s en una raid, cuyos turnos con el jefe atacando a cada uno son largos), guarda capturas de cada ventana en test-shots\ y para. native\build solo se lee: ni su native.ini ni sus
# partidas se tocan.
#   powershell -File native\scripts\coop_test.ps1 -Mode duel
#   powershell -File native\scripts\coop_test.ps1 -Mode raid -Players 3 [-Raid 200]
#   -Mode see | battle | raid | duel; -Keep deja las copias abiertas al terminar.
param([ValidateSet("see", "battle", "raid", "duel")][string]$Mode = "see", [ValidateRange(2, 4)][int]$Players = 2,
      [int]$Raid = 200, [int]$Timeout = 900, [switch]$Keep)
$ErrorActionPreference = "Stop"
$root = Split-Path (Split-Path $PSScriptRoot)  # la carpeta del proyecto: native\, test2..test5
$build = "$root\native\build"
$shots = "$root\test-shots"
if ($Mode -in "battle", "duel") { $Players = 2 }  # el PvP y el duelo son de dos
$dirs = @("$root\test2", "$root\test3", "$root\test4", "$root\test5")[0..($Players - 1)]
$parties = @(0, 1, 2, 0)
$quietMax = if ($Mode -eq "raid") { 180 } else { 40 }
$done = @{ see = 'is here'; battle = 'PvP battle with player \d+: '; raid = 'Raid: (won|lost)'; duel = 'Card duel with player \d+: (won|lost)' }[$Mode]

Add-Type -Name Shot -Namespace CoopTest -MemberDefinition @'
[DllImport("user32.dll")] public static extern bool GetWindowRect(System.IntPtr h, out RECT r);
[DllImport("user32.dll")] public static extern bool PrintWindow(System.IntPtr h, System.IntPtr dc, uint flags);
public struct RECT { public int L, T, R, B; }
'@
Add-Type -AssemblyName System.Drawing
function Capture($dir, $out) {  # la ventana de la copia de esa carpeta, a un PNG
    $p = Get-Process dw2003 -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 -and (Split-Path $_.Path) -eq $dir } | Select-Object -First 1
    if (-not $p) { return }
    $r = New-Object CoopTest.Shot+RECT; [CoopTest.Shot]::GetWindowRect($p.MainWindowHandle, [ref]$r) | Out-Null
    $bmp = New-Object System.Drawing.Bitmap ($r.R - $r.L), ($r.B - $r.T)
    $g = [System.Drawing.Graphics]::FromImage($bmp); $dc = $g.GetHdc()
    [CoopTest.Shot]::PrintWindow($p.MainWindowHandle, $dc, 2) | Out-Null; $g.ReleaseHdc($dc); $g.Dispose()
    $bmp.Save($out); $bmp.Dispose()
}
function Stop-Copies {  # solo las de test2..test5, nunca la de native\build
    Get-Process dw2003 -ErrorAction SilentlyContinue | Where-Object { (Split-Path $_.Path) -like "$root\test*" } | Stop-Process
}

Stop-Copies; Start-Sleep 1
$base = (Get-Content "$build\native.ini" | Where-Object { $_ -notmatch '^\s*(coop|coop_peer|coop_peers|coop_port|scale|fullscreen)\s*=' }) -join "`n"
$auto = @{ see = $null; battle = 'battle'; raid = 'raid'; duel = 'duel' }[$Mode]
for ($i = 0; $i -lt $Players; $i++) {
    $d = $dirs[$i]
    New-Item -ItemType Directory -Force "$d\saves" | Out-Null
    if (-not (Test-Path "$d\data")) { New-Item -ItemType Junction -Path "$d\data" -Target "$build\data" | Out-Null }
    Copy-Item "$build\dw2003.exe", "$build\dw2003.fill", "$build\SDL3.dll" $d -Force
    $peers = (0..($Players - 1) | Where-Object { $_ -ne $i } | ForEach-Object { "127.0.0.1:$(27501 + $_)" }) -join ', '
    Set-Content "$d\native.ini" "$base`ncoop = 1`nscale = 2`nfullscreen = 0`ncoop_port = $(27501 + $i)`ncoop_peers = $peers`n" -NoNewline
    Remove-Item "$d\coop.log" -ErrorAction SilentlyContinue
    if ($auto) { $env:DW3_COOP_AUTO = $auto } else { Remove-Item Env:DW3_COOP_AUTO -ErrorAction SilentlyContinue }
    if ($Mode -eq "raid") { $env:DW3_COOP_RAID = "$Raid" } else { Remove-Item Env:DW3_COOP_RAID -ErrorAction SilentlyContinue }
    Start-Process "$d\dw2003.exe" -ArgumentList "--field 0x230 --progress 10 --party $($parties[$i])" -WorkingDirectory $d
    Start-Sleep -Milliseconds 800
}
Remove-Item Env:DW3_COOP_AUTO, Env:DW3_COOP_RAID -ErrorAction SilentlyContinue

$logs = $dirs | ForEach-Object { "$_\coop.log" }
$end = (Get-Date).AddSeconds($Timeout); $last = ''; $quiet = 0; $stalled = $false
while ((Get-Date) -lt $end) {
    Start-Sleep 5
    $found = @($logs | Where-Object { (Test-Path $_) -and (Select-String -Path $_ -Pattern $done -Quiet) }).Count
    if ($found -ge $Players) { break }
    $now = ($logs | ForEach-Object { if (Test-Path $_) { Get-Content $_ | Select-String -NotMatch 'ping' | Select-Object -Last 1 } }) -join '|'
    if ($now -eq $last) { $quiet += 5 } else { $quiet = 0; $last = $now }
    if ($quiet -ge $quietMax) { $stalled = $true; break }
}
$result = if ($stalled) { "ATASCADA ($quietMax s sin cambios en los logs)" } elseif ((Get-Date) -ge $end) { "SIN TERMINAR ($Timeout s)" } else { "terminada" }
Write-Host "== $Mode con $Players copias: $result" -ForegroundColor $(if ($result -eq "terminada") { "Green" } else { "Red" })
if ($result -ne "terminada") {
    New-Item -ItemType Directory -Force $shots | Out-Null
    for ($i = 0; $i -lt $Players; $i++) { Capture $dirs[$i] "$shots\$Mode-$($i + 2).png" }
    Write-Host "   capturas en $shots"
}
foreach ($l in $logs) {
    $name = Split-Path (Split-Path $l) -Leaf
    if (Test-Path $l) {
        Select-String -Path $l -Pattern "$done|out of step|refused|disconnected|gone" | ForEach-Object { "  ${name}: $($_.Line)" }
    } else {
        "  ${name}: sin coop.log"
    }
}
if (-not $Keep) { Stop-Copies }

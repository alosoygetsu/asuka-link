# Compila Asuka Link con el compilador y SDL3 de la carpeta del proyecto (toolchain\, junto a native\).
# Uso: powershell -File native\scripts\build.ps1 [argumentos de build.py, p. ej. --clean]
$root = Split-Path (Split-Path $PSScriptRoot)  # la carpeta del proyecto: native\, toolchain\, dw3_decomp\
$env:LLVM_MINGW = "$root\toolchain\llvm-mingw-20250114-ucrt-x86_64"
$env:PATH = "$env:LLVM_MINGW\bin;C:\Program Files\Git\usr\bin;$env:PATH"
Push-Location "$root\native"
try { python -I build.py @args; exit $LASTEXITCODE } finally { Pop-Location }

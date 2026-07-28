[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$installData = Join-Path $env:ProgramData "opencpn\plugins\install_data"
$oldNames = @("otcurrent_ltc", "otcurrent_LTC_V.2.1")
$oldFiles = foreach ($name in $oldNames) {
    Join-Path $installData "imports\$name.xml"
    Join-Path $installData "$name.dirs"
    Join-Path $installData "$name.files"
    Join-Path $installData "$name.version"
}

if (Get-Process -Name "opencpn" -ErrorAction SilentlyContinue) {
    throw "OpenCPN läuft noch. Bitte OpenCPN vollständig schließen und das Skript erneut starten."
}

$removed = 0
foreach ($path in $oldFiles) {
    if (Test-Path -LiteralPath $path) {
        Remove-Item -LiteralPath $path -Force
        Write-Host "Entfernt: $path"
        $removed++
    }
}

if ($removed -eq 0) {
    Write-Host "Keine alten otcurrent-LTC-Importmetadaten gefunden."
}

Write-Host ""
Write-Host "Plugin-DLL, Einstellungen und TCD-Auswahl wurden nicht gelöscht."
Write-Host "Jetzt das neu gebaute Paket otcurrent_LTC_V.2.2 über 'Plugin importieren' installieren."

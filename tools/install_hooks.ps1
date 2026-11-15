# Run once after cloning:  powershell -File tools\install_hooks.ps1
git config core.hooksPath .githooks
if ($LASTEXITCODE -eq 0) { Write-Host 'IP guard hooks installed (.githooks).' }

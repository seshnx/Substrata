# Helper script to set up JUCE submodule (optional)

Write-Host "Setting up JUCE..." -ForegroundColor Cyan
Write-Host ""

# Check if JUCE already exists
if (Test-Path "JUCE") {
    Write-Host "JUCE directory already exists. Skipping clone." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "If you want to reinstall, delete the JUCE folder first." -ForegroundColor Yellow
    exit 0
}

# Clone JUCE
Write-Host "Cloning JUCE repository..." -ForegroundColor Green
Write-Host "This may take a few minutes..." -ForegroundColor Gray
git clone --depth 1 https://github.com/juce-framework/JUCE.git JUCE

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "JUCE setup complete!" -ForegroundColor Green
    Write-Host ""
    Write-Host "You can now run build.bat or build.ps1 to compile the plugin." -ForegroundColor Yellow
} else {
    Write-Host ""
    Write-Host "ERROR: Failed to clone JUCE." -ForegroundColor Red
    Write-Host "You may need to install git or clone JUCE manually." -ForegroundColor Yellow
    exit 1
}


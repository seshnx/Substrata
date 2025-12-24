# Substrata Build Script
# PowerShell script to build the SeshNx Substrata plugin

param(
    [string]$Config = "Release",
    [switch]$Clean = $false,
    [switch]$Configure = $false
)

$ErrorActionPreference = "Stop"

# Get the script directory (project root)
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = $ScriptDir
$BuildDir = Join-Path $ProjectDir "build"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "SeshNx Substrata Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Project Directory: $ProjectDir" -ForegroundColor Gray
Write-Host "Build Directory: $BuildDir" -ForegroundColor Gray
Write-Host "Configuration: $Config" -ForegroundColor Gray
Write-Host ""

# Check if JUCE directory exists
if (-not (Test-Path (Join-Path $ProjectDir "JUCE")))
{
    Write-Host "ERROR: JUCE directory not found!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please either:" -ForegroundColor Yellow
    Write-Host "  1. Run: .\setup_juce.ps1" -ForegroundColor Yellow
    Write-Host "  2. Clone JUCE manually:" -ForegroundColor Yellow
    Write-Host "     git clone https://github.com/juce-framework/JUCE.git JUCE" -ForegroundColor Yellow
    Write-Host "  3. Or modify CMakeLists.txt to point to your JUCE installation" -ForegroundColor Yellow
    Write-Host ""
    exit 1
}

# Clean build directory if requested
if ($Clean -and (Test-Path $BuildDir))
{
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Path $BuildDir -Recurse -Force
    Write-Host "Build directory cleaned." -ForegroundColor Green
    Write-Host ""
}

# Create build directory if it doesn't exist
if (-not (Test-Path $BuildDir))
{
    Write-Host "Creating build directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
    Write-Host "Build directory created." -ForegroundColor Green
    Write-Host ""
    $Configure = $true
}

# Change to build directory
Push-Location $BuildDir

try
{
    # Configure with CMake if needed
    if ($Configure -or -not (Test-Path "CMakeCache.txt"))
    {
        Write-Host "Configuring CMake..." -ForegroundColor Yellow
        cmake .. -DCMAKE_BUILD_TYPE=$Config
        
        if ($LASTEXITCODE -ne 0)
        {
            Write-Host "CMake configuration failed!" -ForegroundColor Red
            exit 1
        }
        Write-Host "CMake configuration successful." -ForegroundColor Green
        Write-Host ""
    }
    else
    {
        Write-Host "Using existing CMake configuration." -ForegroundColor Gray
        Write-Host ""
    }
    
    # Build the project
    Write-Host "Building project..." -ForegroundColor Yellow
    Write-Host ""
    
    cmake --build . --config $Config
    
    if ($LASTEXITCODE -ne 0)
    {
        Write-Host ""
        Write-Host "Build failed!" -ForegroundColor Red
        exit 1
    }
    
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "Build completed successfully!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    
    # Show output location
    $VST3Path = Join-Path $BuildDir "Substrata_artefacts\$Config\VST3\Substrata.vst3"
    $AUPath = Join-Path $BuildDir "Substrata_artefacts\$Config\AU\Substrata.component"
    $StandalonePath = Join-Path $BuildDir "Substrata_artefacts\$Config\Standalone\Substrata.exe"
    
    Write-Host "Output files:" -ForegroundColor Cyan
    if (Test-Path $VST3Path)
    {
        Write-Host "  VST3: $VST3Path" -ForegroundColor Gray
    }
    if (Test-Path $AUPath)
    {
        Write-Host "  AU: $AUPath" -ForegroundColor Gray
    }
    if (Test-Path $StandalonePath)
    {
        Write-Host "  Standalone: $StandalonePath" -ForegroundColor Gray
    }
    Write-Host ""
}
catch
{
    Write-Host ""
    Write-Host "Error occurred: $_" -ForegroundColor Red
    exit 1
}
finally
{
    Pop-Location
}

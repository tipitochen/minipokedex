@echo off
color 0A
:: Asegurar que el script trabaje en su propia carpeta
cd /d "%~dp0"

echo ==========================================
echo    VERIFICANDO PYTHON EN EL SISTEMA...
echo ==========================================

python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo.
    echo [X ERROR CRITICO] Python no esta instalado o no se encuentra en el PATH.
    echo Por favor, instala Python desde https://www.python.org/ 
    echo IMPORTANTE: Asegurate de marcar la casilla "Add python.exe to PATH" durante la instalacion.
    echo.
    pause
    exit /b
) else (
    echo [V] Python detectado correctamente.
)

echo.
echo ==========================================
echo    COMPROBANDO DEPENDENCIAS DE PYTHON...
echo ==========================================
echo Instalando solo pyserial (evitando dependencias pesadas)...
python -m pip install --upgrade pip >nul 2>&1
python -m pip install pyserial

echo.
echo ==========================================
echo    COMPROBANDO ARDUINO-CLI...
echo ==========================================

:: Comprobar si arduino-cli existe en la carpeta local
if exist arduino-cli.exe (
    echo [V] 'arduino-cli' ya esta listo en la carpeta local.
    set "CLI=.\arduino-cli.exe"
) else (
    where arduino-cli >nul 2>&1
    if %errorlevel% neq 0 (
        echo [!] No se encontro 'arduino-cli' en el sistema ni localmente.
        echo Intentando instalar con winget...
        winget install ArduinoSA.ArduinoCLI --accept-source-agreements --accept-package-agreements
        echo.
        echo [V] Instalacion finalizada.
        set "CLI=arduino-cli"
    ) else (
        echo [V] 'arduino-cli' ya esta instalado en el sistema.
        set "CLI=arduino-cli"
    )
)

echo.
echo ==========================================
echo    VERIFICANDO NUCLEO ESP32...
echo ==========================================
:: Inicializar configuracion si es la primera vez
%CLI% config init --additional-urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json >nul 2>&1

echo Actualizando indices de placas (esto puede tardar unos segundos)...
%CLI% core update-index

echo.
echo Descargando e instalando el nucleo ESP32 (ESTO PUEDE TARDAR VARIOS MINUTOS DEPENDIENDO DE TU INTERNET)...
echo Por favor, no cierres la ventana.
%CLI% core install esp32:esp32

echo [V] Entorno de compilacion preparado.

echo.
echo ==========================================
echo    INICIANDO PANEL DE LA MINI POKEDEX...
echo ==========================================
python flasheador_pokedex.py

if errorlevel 1 (
    echo.
    echo [!] El programa finalizo con algun error.
)
echo.
pause
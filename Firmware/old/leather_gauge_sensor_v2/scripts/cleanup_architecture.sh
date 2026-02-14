#!/bin/bash
# Script de Limpieza de Arquitectura - LwPKT Refactor
# Fecha: 2026-02-09
# Autor: c-pro Agent

set -e  # Exit on error

PROJECT_ROOT="/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_sensor_v2"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

echo "========================================="
echo "Limpieza de Arquitectura LwPKT"
echo "========================================="
echo ""

# 1. Eliminar archivos duplicados de adapters
echo "[1/4] Eliminando archivos duplicados en adapters/comms_lwpkt..."

cd "$PROJECT_ROOT/leather_gauge_sensor/adapters/comms_lwpkt"

if [ -f "lg_adapter_comm_FIXED.c" ]; then
    echo "  ❌ Eliminando lg_adapter_comm_FIXED.c (duplicado de lg_adapter_comm.c)"
    rm lg_adapter_comm_FIXED.c
fi

if [ -f "lg_adapter_comm_refactored.c" ]; then
    echo "  ❌ Eliminando lg_adapter_comm_refactored.c (duplicado de lg_adapter_comm.c)"
    rm lg_adapter_comm_refactored.c
fi

echo "  ✅ Adapters limpios"
echo ""

# 2. Archivar carpeta modules/ (arquitectura vieja)
echo "[2/4] Archivando carpeta modules/ (arquitectura pre-refactor)..."

cd "$PROJECT_ROOT/leather_gauge_sensor"

if [ -d "modules" ]; then
    ARCHIVE_DIR="_archived_modules_$TIMESTAMP"
    echo "  📦 Creando archivo: $ARCHIVE_DIR"
    mkdir -p "$ARCHIVE_DIR"
    mv modules/* "$ARCHIVE_DIR/" 2>/dev/null || true
    rmdir modules 2>/dev/null || true
    echo "  ✅ Módulos archivados en: $ARCHIVE_DIR"
    echo "  ℹ️  Si build exitoso, eliminar con: rm -rf $ARCHIVE_DIR"
else
    echo "  ℹ️  Carpeta modules/ ya no existe"
fi

echo ""

# 3. Eliminar archivos .bak
echo "[3/4] Eliminando archivos backup (.bak)..."

cd "$PROJECT_ROOT"
BAK_COUNT=$(find . -name "*.bak" -type f | wc -l)

if [ "$BAK_COUNT" -gt 0 ]; then
    find . -name "*.bak" -type f -print -delete
    echo "  ✅ Eliminados $BAK_COUNT archivos .bak"
else
    echo "  ℹ️  No se encontraron archivos .bak"
fi

echo ""

# 4. Limpiar archivos de build antiguos
echo "[4/4] Limpiando archivos de build..."

cd "$PROJECT_ROOT/Debug"

if [ -d "leather_gauge_sensor/modules" ]; then
    echo "  ❌ Eliminando objetos de modules/ obsoletos"
    rm -rf leather_gauge_sensor/modules
    echo "  ✅ Objetos de build antiguos eliminados"
else
    echo "  ℹ️  Sin objetos antiguos en Debug/"
fi

echo ""
echo "========================================="
echo "Limpieza Completada"
echo "========================================="
echo ""
echo "Próximos pasos:"
echo "  1. Verificar compilación:"
echo "     cd Debug && make clean && make all"
echo ""
echo "  2. Si build exitoso, eliminar archivo:"
echo "     rm -rf leather_gauge_sensor/_archived_modules_$TIMESTAMP"
echo ""
echo "  3. Commit cambios:"
echo "     git add -A"
echo "     git commit -m 'refactor: clean architecture, remove duplicates, archive old modules'"
echo ""
echo "Archivos modificados:"
echo "  - Eliminados: lg_adapter_comm_FIXED.c, lg_adapter_comm_refactored.c"
echo "  - Archivados: leather_gauge_sensor/modules/ → _archived_modules_$TIMESTAMP/"
echo "  - Eliminados: *.bak files"
echo "  - Actualizados: lg_core.c (usa wrappers de interfaz)"
echo "  - Actualizados: lg_i_comm.h, lg_i_lwpkt.h (añadidos wrappers)"
echo ""

#!/usr/bin/env python3
"""
Quick Test Script - Leather Gauge Sensor V2
============================================
Script de prueba rápida para verificar comunicación básica con sensores.

ANTES DE EJECUTAR:
1. pip install pyserial
2. Conectar conversor RS-485 USB
3. Editar PORT abajo con tu puerto serial

Uso:
    python quick_test.py
"""

import sys
import time

try:
    from main import LeatherGaugeTester
except ImportError:
    print("❌ Error: No se encontró main.py")
    print("   Asegúrate de ejecutar desde el directorio correcto.")
    sys.exit(1)

# ============================================================================
# CONFIGURACIÓN - EDITA AQUÍ
# ============================================================================

# Puerto serial (cambiar según tu sistema)
PORT = '/dev/ttyUSB0'   # Linux: /dev/ttyUSB0 o /dev/ttyACM0
# PORT = 'COM3'         # Windows: COM3, COM4, etc.
# PORT = '/dev/cu.usbserial-1410'  # Mac

BAUDRATE = 115200
TIMEOUT = 0.5

# Configuración de sensores
NUM_SENSORS = 11         # Número total de sensores en el bus
SENSOR_TO_TEST = 1       # Sensor individual a probar

# ============================================================================
# SCRIPT DE PRUEBA
# ============================================================================

def main():
    print("="*70)
    print("  LEATHER GAUGE SENSOR V2 - QUICK TEST")
    print("="*70)
    print(f"\n📋 Configuración:")
    print(f"   Puerto:  {PORT}")
    print(f"   Baudrate: {BAUDRATE}")
    print(f"   Sensores: {NUM_SENSORS}")
    print(f"   Test individual: Sensor {SENSOR_TO_TEST}")
    print()
    
    # Inicializar tester
    tester = LeatherGaugeTester(
        port=PORT,
        baudrate=BAUDRATE,
        timeout=TIMEOUT
    )
    
    # Conectar
    if not tester.connect():
        print("\n❌ Error: No se pudo conectar al puerto serial")
        print("   Verifica:")
        print("   - Conversor RS-485 está conectado")
        print("   - Puerto es correcto (edita PORT en este script)")
        print("   - Permisos en Linux: sudo usermod -a -G dialout $USER")
        return False
    
    print("\n" + "="*70)
    print("  TEST 1: LECTURA INDIVIDUAL")
    print("="*70)
    
    try:
        values = tester.read_sensor(SENSOR_TO_TEST, timeout=1.0)
        
        if values:
            print(f"\n✅ Sensor {SENSOR_TO_TEST} respondió correctamente!")
            print(f"   Valores: {[f'{v:.2f}' for v in values]}")
            print(f"   Promedio: {sum(values)/len(values):.2f}")
            print(f"   Rango: [{min(values):.2f}, {max(values):.2f}]")
        else:
            print(f"\n❌ Sensor {SENSOR_TO_TEST} no respondió")
            print("   Verifica:")
            print("   - Sensor está alimentado (12-24VDC)")
            print("   - Cables A-B correctamente conectados")
            print(f"   - Dirección del sensor es {SENSOR_TO_TEST}")
            print("   - Terminación de 120Ω en extremos del bus")
            return False
        
        time.sleep(0.5)
        
        print("\n" + "="*70)
        print("  TEST 2: LECTURA EN CASCADA (OPTIMIZADA)")
        print("="*70)
        
        results = tester.read_cascade(num_sensors=NUM_SENSORS, timeout_per_sensor=0.3)
        
        if results:
            print(f"\n✅ Cascada completada: {len(results)}/{NUM_SENSORS} sensores respondieron")
            print("\n📊 Resumen:")
            for addr, vals in sorted(results.items()):
                avg = sum(vals) / len(vals)
                print(f"   Sensor {addr:2d}: Promedio={avg:8.2f}  "
                      f"Min={min(vals):8.2f}  Max={max(vals):8.2f}")
        else:
            print("\n⚠️  No se recibieron respuestas en modo cascada")
            print("   Esto puede indicar problema con el protocolo LwPKT")
            return False
        
        print("\n" + "="*70)
        print("  RESUMEN FINAL")
        print("="*70)
        print(f"\n✅ Todos los tests completados exitosamente!")
        print(f"   - Comunicación RS-485: OK")
        print(f"   - Protocolo LwPKT: OK")
        print(f"   - Sensores detectados: {len(results)}/{NUM_SENSORS}")
        print(f"   - Modo cascada funcional: {'SÍ' if len(results) > 0 else 'NO'}")
        
        if len(results) < NUM_SENSORS:
            print(f"\n⚠️  Nota: Solo {len(results)} de {NUM_SENSORS} sensores respondieron")
            print("   Sensores faltantes:")
            missing = set(range(1, NUM_SENSORS+1)) - set(results.keys())
            for addr in sorted(missing):
                print(f"   - Sensor {addr}")
        
        print("\n💡 Siguiente paso: Ejecuta pruebas completas con:")
        print(f"   python main.py --port {PORT} --mode compare --sensors {NUM_SENSORS}")
        
        return True
    
    except KeyboardInterrupt:
        print("\n\n⚠️  Test interrumpido por usuario")
        return False
    
    except Exception as e:
        print(f"\n❌ Error inesperado: {e}")
        import traceback
        traceback.print_exc()
        return False
    
    finally:
        tester.disconnect()

if __name__ == '__main__':
    print("\n🚀 Iniciando quick test...\n")
    success = main()
    
    if success:
        print("\n🎉 ¡Test exitoso! El sistema está funcionando correctamente.\n")
        sys.exit(0)
    else:
        print("\n❌ Test falló. Revisa los mensajes de error arriba.\n")
        sys.exit(1)

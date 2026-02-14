#!/usr/bin/env python3
"""
Leather Gauge Sensor V2 - Test Script
=====================================
Script para probar sensores de espesor de cuero desde PC usando conversor RS-485 USB.

Hardware requerido:
- Conversor RS-485 a USB (CH340, FTDI, etc.)
- Sensores conectados al bus RS-485

Uso:
    python main.py --port /dev/ttyUSB0 --mode individual
    python main.py --port COM3 --mode cascade --sensors 11
"""

import serial
import time
import struct
import argparse
import sys
from typing import List, Optional, Tuple
from lwpkt import LwPKT
from enum import IntEnum

# ============================================================================
# Comandos del Sensor (desde lg_domain_types.h)
# ============================================================================
class SensorCommand(IntEnum):
    # Read Commands
    CMD_READ_SENSOR = 0x10       # Lectura individual (calibrated)
    CMD_READ_SENSOR_RESP = 0x90  # Respuesta
    CMD_READ_RAW = 0x11          # Lectura raw ADC
    CMD_READ_RAW_RESP = 0x91     # Respuesta raw
    
    # Cascade Commands
    CMD_READ_CASCADE = 0x12      # Lectura en cascada (broadcast)
    CMD_READ_CASCADE_RESP = 0x92 # Respuesta cascada
    
    # Write/Config Commands
    CMD_WRITE_CONFIG = 0x20
    CMD_WRITE_CONFIG_RESP = 0xA0
    CMD_SET_OFFSET = 0x21
    CMD_SET_FILTER = 0x22
    
    # Control Commands
    CMD_CALIBRATE = 0x30
    CMD_CALIBRATE_RESP = 0xB0
    CMD_GET_STATUS = 0x31
    CMD_GET_STATUS_RESP = 0xB1
    
    # Error
    CMD_ERROR = 0xFF

# Códigos de error
class ErrorCode(IntEnum):
    LG_OK = 0
    LG_ERROR = 1
    LG_BUSY = 2
    LG_TIMEOUT = 3
    LG_INVALID_PARAM = 4

# ============================================================================
# Clase Principal: LeatherGaugeTester
# ============================================================================
class LeatherGaugeTester:
    """
    Clase para comunicación con sensores de espesor de cuero vía RS-485.
    """
    
    MASTER_ADDRESS = 0xFF  # Dirección del Master (broadcast capable)
    BROADCAST_ADDRESS = 0xFF
    
    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 0.5):
        """
        Inicializa el tester.
        
        Args:
            port: Puerto serial (ej: '/dev/ttyUSB0' o 'COM3')
            baudrate: Velocidad (default: 115200)
            timeout: Timeout de lectura serial (segundos)
        """
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial = None
        self.lwpkt = LwPKT()
        
        # Configurar LwPKT según firmware (lg_adapter_comm.c)
        self.lwpkt.our_addr = self.MASTER_ADDRESS
        self.lwpkt.opt_addr = True
        self.lwpkt.opt_addr_ext = False      # Direcciones de 1 byte (1-11)
        self.lwpkt.opt_cmd = True
        self.lwpkt.opt_cmd_ext = False       # Comandos de 1 byte
        self.lwpkt.opt_flags = True          # FLAGS habilitado (4 bytes)
        self.lwpkt.opt_crc = True
        self.lwpkt.opt_crc32 = False         # CRC-8 (no CRC-32)
        
        print(f"🔧 Inicializando LeatherGaugeTester")
        print(f"   Puerto: {port}")
        print(f"   Baudrate: {baudrate}")
        print(f"   Timeout: {timeout}s")
    
    def connect(self) -> bool:
        """
        Abre la conexión serial.
        
        Returns:
            True si conectó exitosamente
        """
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=self.timeout
            )
            print(f"✅ Conectado a {self.port}")
            time.sleep(0.1)  # Esperar estabilización
            self.serial.reset_input_buffer()
            self.serial.reset_output_buffer()
            return True
        except serial.SerialException as e:
            print(f"❌ Error al conectar: {e}")
            return False
    
    def disconnect(self):
        """Cierra la conexión serial."""
        if self.serial and self.serial.is_open:
            self.serial.close()
            print("🔌 Desconectado")
    
    def send_packet(self, to_addr: int, cmd: int, data: bytes = None, flags: int = 0):
        """
        Envía un paquete LwPKT al sensor.
        
        Args:
            to_addr: Dirección destino (1-11 o 0xFF para broadcast)
            cmd: Comando a enviar
            data: Payload (opcional)
            flags: Campo FLAGS (para modo cascada)
        """
        if not self.serial or not self.serial.is_open:
            print("❌ Puerto serial no está abierto")
            return
        
        # Generar paquete
        packet = self.lwpkt.generate_packet(
            data=data if data else bytearray(),
            cmd=cmd,
            addr_to=to_addr,
            flags=flags
        )
        
        # Debug: mostrar paquete
        hex_str = ' '.join([f'{b:02X}' for b in packet])
        print(f"📤 TX → Addr:{to_addr:02X} CMD:{cmd:02X} FLAGS:{flags:08X} LEN:{len(data) if data else 0}")
        print(f"   Bytes: {hex_str}")
        
        # Enviar por serial
        self.serial.write(packet)
        self.serial.flush()
    
    def receive_packet(self, timeout: float = None) -> Optional[LwPKT.Packet]:
        """
        Recibe y decodifica un paquete LwPKT.
        
        Args:
            timeout: Timeout en segundos (usa self.timeout si es None)
        
        Returns:
            Packet decodificado o None si timeout/error
        """
        if not self.serial or not self.serial.is_open:
            return None
        
        timeout = timeout if timeout is not None else self.timeout
        start_time = time.time()
        
        while (time.time() - start_time) < timeout:
            # Leer bytes disponibles
            if self.serial.in_waiting > 0:
                data = self.serial.read(self.serial.in_waiting)
                self.lwpkt.write_rx_data(data)
            
            # Procesar paquetes recibidos
            if self.lwpkt.rx_process():
                packet = self.lwpkt.rx_get_packet()
                if packet:
                    # Debug: mostrar paquete recibido
                    hex_str = ' '.join([f'{b:02X}' for b in packet.data[:20]])  # Primeros 20 bytes
                    if len(packet.data) > 20:
                        hex_str += '...'
                    print(f"📥 RX ← From:{packet.pkt_from:02X} CMD:{packet.cmd:02X} FLAGS:{packet.flags:08X} LEN:{packet.len}")
                    print(f"   Data: {hex_str}")
                    return packet
            
            time.sleep(0.01)  # Pequeña pausa para no saturar CPU
        
        return None
    
    # ========================================================================
    # Comandos de Alto Nivel
    # ========================================================================
    
    def read_sensor(self, sensor_addr: int, timeout: float = 0.5) -> Optional[List[float]]:
        """
        Lee valores calibrados de un sensor específico (modo individual).
        
        Args:
            sensor_addr: Dirección del sensor (1-11)
            timeout: Timeout en segundos
        
        Returns:
            Lista de 10 floats con valores calibrados, o None si error
        """
        print(f"\n📊 Leyendo Sensor {sensor_addr} (Individual)...")
        
        # Enviar comando
        self.send_packet(
            to_addr=sensor_addr,
            cmd=SensorCommand.CMD_READ_SENSOR,
            data=None,
            flags=0
        )
        
        # Esperar respuesta
        packet = self.receive_packet(timeout=timeout)
        
        if packet is None:
            print(f"⏱️  Timeout esperando respuesta de Sensor {sensor_addr}")
            return None
        
        # Verificar comando de respuesta
        if packet.cmd != SensorCommand.CMD_READ_SENSOR_RESP:
            print(f"⚠️  Respuesta inesperada: CMD={packet.cmd:02X} (esperado {SensorCommand.CMD_READ_SENSOR_RESP:02X})")
            return None
        
        # Decodificar payload (10 floats = 40 bytes)
        if packet.len != 40:
            print(f"⚠️  Longitud de payload incorrecta: {packet.len} bytes (esperado 40)")
            return None
        
        try:
            # Unpack 10 floats (little-endian)
            values = struct.unpack('<10f', bytes(packet.data))
            print(f"✅ Sensor {sensor_addr}: {[f'{v:.2f}' for v in values]}")
            return list(values)
        except struct.error as e:
            print(f"❌ Error al decodificar datos: {e}")
            return None
    
    def read_raw(self, sensor_addr: int, timeout: float = 0.5) -> Optional[List[int]]:
        """
        Lee valores raw ADC de un sensor.
        
        Args:
            sensor_addr: Dirección del sensor (1-11)
            timeout: Timeout en segundos
        
        Returns:
            Lista de 10 uint16 con valores ADC, o None si error
        """
        print(f"\n🔬 Leyendo Sensor {sensor_addr} (Raw ADC)...")
        
        self.send_packet(
            to_addr=sensor_addr,
            cmd=SensorCommand.CMD_READ_RAW,
            data=None,
            flags=0
        )
        
        packet = self.receive_packet(timeout=timeout)
        
        if packet is None:
            print(f"⏱️  Timeout esperando respuesta de Sensor {sensor_addr}")
            return None
        
        if packet.cmd != SensorCommand.CMD_READ_RAW_RESP:
            print(f"⚠️  Respuesta inesperada: CMD={packet.cmd:02X}")
            return None
        
        # Decodificar payload (10 uint16 = 20 bytes)
        if packet.len != 20:
            print(f"⚠️  Longitud de payload incorrecta: {packet.len} bytes (esperado 20)")
            return None
        
        try:
            values = struct.unpack('<10H', bytes(packet.data))
            print(f"✅ Sensor {sensor_addr} RAW: {list(values)}")
            return list(values)
        except struct.error as e:
            print(f"❌ Error al decodificar datos: {e}")
            return None
    
    def read_cascade(self, num_sensors: int = 11, timeout_per_sensor: float = 0.2) -> dict:
        """
        Lee todos los sensores en modo cascada (optimizado).
        
        Args:
            num_sensors: Número de sensores en el bus (default: 11)
            timeout_per_sensor: Timeout por sensor en segundos
        
        Returns:
            Diccionario {sensor_addr: [valores], ...}
        """
        print(f"\n⚡ Lectura en Cascada ({num_sensors} sensores)...")
        results = {}
        
        # Enviar comando broadcast con FLAGS=1 (inicia en Sensor 1)
        start_time = time.time()
        self.send_packet(
            to_addr=self.BROADCAST_ADDRESS,
            cmd=SensorCommand.CMD_READ_CASCADE,
            data=None,
            flags=1  # Inicia en Sensor 1
        )
        
        # Recibir respuestas secuenciales
        for i in range(1, num_sensors + 1):
            packet = self.receive_packet(timeout=timeout_per_sensor)
            
            if packet is None:
                print(f"⏱️  Timeout esperando Sensor {i}")
                break
            
            if packet.cmd != SensorCommand.CMD_READ_CASCADE_RESP:
                print(f"⚠️  Respuesta inesperada de Sensor {i}: CMD={packet.cmd:02X}")
                continue
            
            # Decodificar datos
            if packet.len == 40:
                try:
                    values = struct.unpack('<10f', bytes(packet.data))
                    sensor_addr = i  # Asumimos orden secuencial
                    results[sensor_addr] = list(values)
                    print(f"✅ Sensor {sensor_addr}: {[f'{v:.2f}' for v in values[:3]]}... FLAGS={packet.flags}")
                except struct.error as e:
                    print(f"❌ Error decodificando Sensor {i}: {e}")
            else:
                print(f"⚠️  Sensor {i}: Payload incorrecto ({packet.len} bytes)")
            
            # Verificar FLAGS para fin de cascada
            if packet.flags == 0:
                print(f"🏁 Fin de cascada detectado (FLAGS=0)")
                break
        
        elapsed = time.time() - start_time
        print(f"\n📊 Cascada completada en {elapsed:.3f}s ({len(results)}/{num_sensors} sensores)")
        return results
    
    def get_status(self, sensor_addr: int, timeout: float = 0.5) -> Optional[int]:
        """
        Lee el estado digital del sensor (threshold bitmask).
        
        Args:
            sensor_addr: Dirección del sensor
            timeout: Timeout en segundos
        
        Returns:
            uint16 con bitmask de estado, o None si error
        """
        print(f"\n🔍 Consultando estado de Sensor {sensor_addr}...")
        
        self.send_packet(
            to_addr=sensor_addr,
            cmd=SensorCommand.CMD_GET_STATUS,
            data=None,
            flags=0
        )
        
        packet = self.receive_packet(timeout=timeout)
        
        if packet is None:
            print(f"⏱️  Timeout")
            return None
        
        if packet.cmd != SensorCommand.CMD_GET_STATUS_RESP:
            print(f"⚠️  Respuesta inesperada: CMD={packet.cmd:02X}")
            return None
        
        if packet.len != 2:
            print(f"⚠️  Longitud incorrecta: {packet.len} bytes")
            return None
        
        try:
            status = struct.unpack('<H', bytes(packet.data))[0]
            print(f"✅ Estado: 0x{status:04X} (bin: {status:016b})")
            return status
        except struct.error as e:
            print(f"❌ Error al decodificar: {e}")
            return None
    
    def set_filter(self, sensor_addr: int, cutoff_freq: float, timeout: float = 0.5) -> bool:
        """
        Configura la frecuencia de corte del filtro Biquad.
        
        Args:
            sensor_addr: Dirección del sensor
            cutoff_freq: Frecuencia de corte en Hz
            timeout: Timeout en segundos
        
        Returns:
            True si ACK recibido
        """
        print(f"\n⚙️  Configurando filtro de Sensor {sensor_addr} → fc={cutoff_freq}Hz...")
        
        # Empaquetar float
        data = struct.pack('<f', cutoff_freq)
        
        self.send_packet(
            to_addr=sensor_addr,
            cmd=SensorCommand.CMD_SET_FILTER,
            data=data,
            flags=0
        )
        
        packet = self.receive_packet(timeout=timeout)
        
        if packet is None:
            print(f"⏱️  Timeout")
            return False
        
        # Verificar ACK (payload vacío = OK)
        if packet.len == 0:
            print(f"✅ Filtro actualizado")
            return True
        else:
            print(f"⚠️  Respuesta inesperada")
            return False

# ============================================================================
# Funciones de Test
# ============================================================================

def test_individual_read(tester: LeatherGaugeTester, sensor_addr: int):
    """Test de lectura individual."""
    print("\n" + "="*60)
    print("TEST: LECTURA INDIVIDUAL")
    print("="*60)
    
    values = tester.read_sensor(sensor_addr)
    if values:
        print(f"\n📈 Valores del Sensor {sensor_addr}:")
        for i, val in enumerate(values):
            print(f"   Canal {i}: {val:8.3f}")

def test_cascade_read(tester: LeatherGaugeTester, num_sensors: int):
    """Test de lectura en cascada."""
    print("\n" + "="*60)
    print("TEST: LECTURA EN CASCADA")
    print("="*60)
    
    results = tester.read_cascade(num_sensors=num_sensors)
    
    if results:
        print(f"\n📈 Resumen de {len(results)} sensores:")
        for addr, values in results.items():
            avg = sum(values) / len(values)
            print(f"   Sensor {addr}: Promedio={avg:8.3f} Min={min(values):8.3f} Max={max(values):8.3f}")

def test_raw_read(tester: LeatherGaugeTester, sensor_addr: int):
    """Test de lectura RAW ADC."""
    print("\n" + "="*60)
    print("TEST: LECTURA RAW ADC")
    print("="*60)
    
    values = tester.read_raw(sensor_addr)
    if values:
        print(f"\n🔬 Valores ADC (12-bit) del Sensor {sensor_addr}:")
        for i, val in enumerate(values):
            voltage = (val / 4095.0) * 3.3  # Conversión a voltaje (3.3V ref)
            print(f"   Canal {i}: {val:4d} ({voltage:.3f}V)")

def test_compare_modes(tester: LeatherGaugeTester, num_sensors: int):
    """Compara latencia entre modo individual y cascada."""
    print("\n" + "="*60)
    print("TEST: COMPARACIÓN DE MODOS")
    print("="*60)
    
    # Test individual
    print("\n🐢 Modo Individual:")
    start = time.time()
    for i in range(1, num_sensors + 1):
        tester.read_sensor(i, timeout=0.3)
    individual_time = time.time() - start
    print(f"   Tiempo total: {individual_time:.3f}s")
    
    time.sleep(0.5)  # Pausa entre tests
    
    # Test cascada
    print("\n⚡ Modo Cascada:")
    start = time.time()
    tester.read_cascade(num_sensors=num_sensors, timeout_per_sensor=0.2)
    cascade_time = time.time() - start
    print(f"   Tiempo total: {cascade_time:.3f}s")
    
    # Comparación
    improvement = ((individual_time - cascade_time) / individual_time) * 100
    print(f"\n📊 Mejora: {improvement:.1f}% más rápido en modo cascada")

# ============================================================================
# Main
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        description='Leather Gauge Sensor Tester - RS-485 Communication',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Ejemplos:
  # Lectura individual del sensor 1
  python main.py --port /dev/ttyUSB0 --mode individual --sensor 1
  
  # Lectura en cascada de 11 sensores
  python main.py --port COM3 --mode cascade --sensors 11
  
  # Comparar modos
  python main.py --port /dev/ttyUSB0 --mode compare --sensors 11
  
  # Lectura RAW ADC
  python main.py --port /dev/ttyUSB0 --mode raw --sensor 3
        """
    )
    
    parser.add_argument('--port', type=str, required=True,
                        help='Puerto serial (ej: /dev/ttyUSB0 o COM3)')
    parser.add_argument('--baudrate', type=int, default=115200,
                        help='Velocidad en baudios (default: 115200)')
    parser.add_argument('--mode', type=str, default='individual',
                        choices=['individual', 'cascade', 'raw', 'compare', 'interactive'],
                        help='Modo de operación')
    parser.add_argument('--sensor', type=int, default=1,
                        help='Dirección del sensor para modo individual/raw (1-11)')
    parser.add_argument('--sensors', type=int, default=11,
                        help='Número total de sensores para modo cascada (default: 11)')
    parser.add_argument('--timeout', type=float, default=0.5,
                        help='Timeout en segundos (default: 0.5)')
    
    args = parser.parse_args()
    
    # Crear tester
    tester = LeatherGaugeTester(
        port=args.port,
        baudrate=args.baudrate,
        timeout=args.timeout
    )
    
    # Conectar
    if not tester.connect():
        sys.exit(1)
    
    try:
        # Ejecutar test según modo
        if args.mode == 'individual':
            test_individual_read(tester, args.sensor)
        
        elif args.mode == 'cascade':
            test_cascade_read(tester, args.sensors)
        
        elif args.mode == 'raw':
            test_raw_read(tester, args.sensor)
        
        elif args.mode == 'compare':
            test_compare_modes(tester, args.sensors)
        
        elif args.mode == 'interactive':
            # Modo interactivo
            print("\n🎮 Modo Interactivo")
            print("Comandos disponibles:")
            print("  r [addr]       - Leer sensor individual")
            print("  c              - Lectura en cascada")
            print("  s [addr]       - Estado del sensor")
            print("  f [addr] [hz]  - Configurar filtro")
            print("  q              - Salir")
            
            while True:
                try:
                    cmd = input("\n> ").strip().split()
                    if not cmd:
                        continue
                    
                    if cmd[0] == 'q':
                        break
                    elif cmd[0] == 'r' and len(cmd) > 1:
                        tester.read_sensor(int(cmd[1]))
                    elif cmd[0] == 'c':
                        tester.read_cascade(args.sensors)
                    elif cmd[0] == 's' and len(cmd) > 1:
                        tester.get_status(int(cmd[1]))
                    elif cmd[0] == 'f' and len(cmd) > 2:
                        tester.set_filter(int(cmd[1]), float(cmd[2]))
                    else:
                        print("❌ Comando no reconocido")
                except (ValueError, IndexError):
                    print("❌ Parámetros inválidos")
                except KeyboardInterrupt:
                    break
    
    except KeyboardInterrupt:
        print("\n\n⚠️  Interrumpido por usuario")
    
    finally:
        tester.disconnect()
        print("\n✅ Prueba finalizada")

if __name__ == '__main__':
    main()

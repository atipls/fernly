import os
import serial as pyserial

class Device:
    serial: pyserial.Serial

    def __init__(self, serialport):
        self.serial = pyserial.Serial(115200, timeout=1)
        self.serial.set_input_flow_control(enable=False)
        
        print(self.serial.name)
        print(self.serial.is_open)

    def __del__(self):
        self.serial.close()

    def _get_integer(self):
        result = self.serial.read(12).decode('utf-8')
        print(result)
        return int(result, 16)
    
    def _send_command(self, command):
        self.serial.write(command.encode('utf-8'))
        self.serial.write(b'\n')

        print("Sent command",self.serial.readline().decode('utf-8'))

    def readb(self, address):
        self._send_command(f"readx 0x{address:08x} 1") 
        return self._get_integer()
    
    def readw(self, address):
        self._send_command(f"readx 0x{address:08x} 2")
        return self._get_integer()
    
    def readl(self, address):
        self._send_command(f"readx 0x{address:08x} 4")
        return self._get_integer()
    
    def writeb(self, address, value):
        self._send_command(f"writex 0x{address:08x} 1 0x{value:08x}")
        assert "Ok" in self.serial.readline().decode('utf-8')

    def writew(self, address, value):
        self._send_command(f"writex 0x{address:08x} 2 0x{value:08x}")

    def writel(self, address, value):
        self._send_command(f"writex 0x{address:08x} 4 0x{value:08x}")



device = Device("/dev/tty.usbmodem113300")

zero = device.readl(0x0)

print(f"Zero: {zero:08x}, {zero}, 0x42465f53")

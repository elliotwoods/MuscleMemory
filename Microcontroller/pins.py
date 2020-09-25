from machine import Pin, I2C, DAC, SPI

spi_baud_rate = 100000

class Pins:
    def __init__(self):
        self.motor_1 = Pin(32)
        self.motor_2 = Pin(33)
        self.motor_3 = Pin(14)
        self.motor_4 = Pin(27)
        self.motor_vref_a = Pin(25)
        self.motor_vref_b = Pin(26)
        
        self.dial_a = Pin(34, Pin.IN)
        self.dial_b = Pin(35, Pin.IN)
        self.dial_push = Pin(39, Pin.IN)
        
        self.oled_reset = Pin(16, Pin.OUT)
        
        self.i2c_scl = Pin(15)
        self.i2c_sda = Pin(4)
        self.i2c = I2C(-1, scl=self.i2c_scl, sda=self.i2c_sda)
        
        self.spi = SPI(2, baudrate=spi_baud_rate, polarity=0, phase=1, bits=8, firstbit=SPI.MSB, sck=Pin(18), mosi=Pin(23), miso=Pin(19))
        self.encoder_cs = Pin(5, Pin.OUT)
        self.encoder_cs.on()
        
pins = Pins()
print('init pins')

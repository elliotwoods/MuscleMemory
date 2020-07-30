from machine import Pin

class Pins:
    def __init__(self):
        self.motor_A = Pin(12)
        self.motor_B = Pin(13)
        self.hall_encoder_A = Pin(14, Pin.IN, Pin.PULL_UP)
        self.hall_encoder_B = Pin(27, Pin.IN, Pin.PULL_UP)
        #self.current_sense = Pin(26, Pin.IN)
        #self.rgb = Pin(25, Pin.OUT)
        
        #self.vcc_sense = Pin(17, Pin.IN)
        self.dial_a = Pin(5, Pin.IN, Pin.PULL_UP)
        self.dial_b = Pin(18, Pin.IN, Pin.PULL_UP)
        self.dial_button = Pin(23, Pin.IN, Pin.PULL_UP)
        #self.can_speed_select = Pin(19, Pin.OUT)
        #self.can_rxd = Pin(22)
        #self.can_txd = Pin(21)        
        
        self.oled_reset = Pin(16, Pin.OUT)
        self.oled_scl = Pin(15)
        self.oled_sda = Pin(4)
        
pins = Pins()
print('init pins')

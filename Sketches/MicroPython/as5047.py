from pins import pins
from time import sleep_us

def calc_parity(value):
    count = value & 1
    while value > 1:
        value = value >> 1
        count += value & 1
        
    return count % 2

def apply_parity(value):
    value &= (1 << 14) - 1
    value |= calc_parity(value) << 14
    return value

class AS5047:
    def __init__(self):
        self.read_request = bytearray(2)
        self.read_request[0] = 0xFF
        self.read_request[1] = 0xFF
        
        self.error = False
        pass
    
    def parse_response(self, response):
        self.error |= (response[0] >> 6) & 1
        value = (int(response[0]) << 8) + int(response[1])
        value &= (1 << 14) - 1
        return value
        
    def get_position(self, window_size=4):
        pins.encoder_cs.off()
        pins.spi.write(self.read_request)
        pins.encoder_cs.on()
        
        sleep_us(1)
        
        response = bytearray(2)
        
        value = 0
        
        for i in range(window_size):
            pins.encoder_cs.off()
            pins.spi.write_readinto(self.read_request, response)
            pins.encoder_cs.on()
            
            value += self.parse_response(response)
        
        value /= window_size
        return int(round(value))

    def read_register(self, register):
        # read request
        request_int = register | (1 << 14)
    
        # parity
        request_int |= calc_parity(request_int) << 15

        # to bytes
        request = bytearray(2)
        request[0] = (request_int >> 8)
        request[1] = request_int & 255
        
        pins.encoder_cs.off()
        pins.spi.write(request)
        pins.encoder_cs.on()
        
        sleep_us(1)
        
        pins.encoder_cs.off()
        response = pins.spi.read(2)
        pins.encoder_cs.on()
        
        return self.parse_response(response)
    
    def check_error(self):
        value = self.read_register(0x0001)
        framing_error = value & 1
        invalid_command = (value >> 1) & 1
        parity_error = (value >> 2) & 1
        
        if framing_error:
            print("Framing error")
        if invalid_command:
            print("Invalid command")
        if parity_error:
            print("Parity error")        
            
as5047 = AS5047()
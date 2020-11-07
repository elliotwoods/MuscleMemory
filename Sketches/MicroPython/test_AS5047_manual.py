from machine import Pin, SPI
from time import sleep_ms, sleep_us

baud_rate = 100000
print("Baud rate {}".format(baud_rate))

spi = SPI(2, baudrate=baud_rate, polarity=0, phase=1, bits=8, firstbit=SPI.MSB, sck=Pin(18), mosi=Pin(23), miso=Pin(19))
csn = Pin(5, Pin.OUT)

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

def read(request_int):
    # read request
    request_int |= 1 << 14
    
    # parity
    request_int |= calc_parity(request_int) << 15

    # to bytes
    request = bytearray(2)
    request[0] = (request_int >> 8)
    request[1] = request_int & 255    
    
    csn.on()

    sleep_us(1)

    csn.off()
    spi.write(request)
    csn.on()

    sleep_ms(1)

    csn.off()
    response = spi.read(2)
    csn.on()
    
    print(response)
    
    error = (response[0] >> 6) & 1
    if error:
        print("Error in response")
    
    value = (int(response[0]) << 8) + int(response[1])
    value &= (1 << 14) - 1
    
    
    return value

def read_value():
    return read(0xFFFF)

def read_error():
    value = read(0x0001)
    framing_error = value & 1
    invalid_command = (value >> 1) & 1
    parity_error = (value >> 1) & 1
    
    if framing_error:
        print("Framing error")
    if invalid_command:
        print("Invalid command")
    if parity_error:
        print("Parity error")
    
for i in range(100):
    print(read_value())

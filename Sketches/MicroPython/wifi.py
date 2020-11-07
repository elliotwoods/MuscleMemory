import network
from time import sleep_ms
from oled import oled

wifi = network.WLAN(network.STA_IF)
wifi.active(True)
print(wifi.scan())
wifi.connect('Kimchi and Chips 2.4Ghz', 'YourWifiPassword')

tries = 0
while not wifi.isconnected():
    if tries > 5:
        raise Exception("Failed to connecto wifi")
    sleep_ms(1000)
    
print(wifi.ifconfig())
oled.display(wifi.ifconfig())
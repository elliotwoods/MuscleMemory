from machine import Pin, PWM, Timer
from math import floor
from pins import pins

def saturate_signed(value):
    return max(min(value, 1.0), -1.0)

def clamp(value, min, max):
    if value < min:
        return min
    if value > max:
        return max
    else:
        return value
    
class Motor:
    def __init__(self, source_voltage):
        self.A = PWM(pins.motor_A, freq=1000, duty=0)
        self.B = PWM(pins.motor_B, freq=1000, duty=0)
        
        # since we're using 12VDC motors on a 24VDC source
        
        if source_voltage is None:
            raise Exception("No source_voltage specified")
        
        self.min_duty = 512
        self.max_duty = 1023 * source_voltage / 12
        
        self.set_torque(0)
        
        self.timer = Timer(3)
        self.timer.init(period=1000, callback=self.check_time)
        
        self.recent_update = False
        
    def set_torque(self, torque):
        torque = saturate_signed(torque)
        
        # we program with 'coast' mode, actually data sheet suggests we should be in 'braking' mode
        # braking mode would mean that we set A and B to high most of the time, and then
        
        if torque == 0:
            self.A.duty(0)
            self.B.duty(0)
        else:
            duty = abs(torque) * self.max_duty

            if duty < self.min_duty:
                freq = 10
            else:
                freq = 1000
            
            duty = int(floor(duty))
            duty = clamp(duty, 0, self.max_duty)
        
            if torque > 0.0:
                self.A.freq(freq)
                self.A.duty(duty)
                self.B.duty(0)
            else:
                self.A.duty(0)
                self.B.freq(freq)
                self.B.duty(duty)
                
        self.recent_update = True
        
    def stop(self):
        self.set_torque(0)
        
    def check_time(self, something):
        # call this periodically
        if not self.recent_update:
            self.stop()
        self.recent_update = False
        
        
motor = Motor()
import quadrature

class Encoder:
    def __init__(self):
        self.encoder = quadrature.Encoder(14, 27)
        self.reset()
    
    def value(self):
        current_value = self.encoder.value()
        
        # we must have wound around the int16 space
        if current_value > self.previous_value + (1 << 15):
            # underflow
            self.overflow -= 1 << 16
        elif current_value < self.previous_value - (1 << 15):
            # overflow
            self.overflow += 1 << 16
        self.previous_value = current_value
        
        return (current_value - self.start_offset + self.overflow) * 4
    
    def reset(self):
        self.start_offset = self.encoder.value()
        self.previous_value = self.start_offset
        self.overflow = 0

encoder = Encoder()

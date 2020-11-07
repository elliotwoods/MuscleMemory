import tensorflow
import time
import math
import gc

step_count = 1000


model = tensorflow.Model()

for iteration in range(10000):
    
    model.load(tensorflow.get_sine_model())

    start = time.ticks_ms()
    for step in range(step_count):
        model.invoke([step * math.pi * 2 / step_count])
    end = time.ticks_ms()
    print("{} iterations complete in {}ms".format(step_count, end - start))
    
    model.unload()
    
    
    
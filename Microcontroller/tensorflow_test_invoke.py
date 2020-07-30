import tensorflow
import time
import math

model = tensorflow.Model()
model.load(tensorflow.get_sine_model())

for iteration in range(80000):
    if iteration % 100 == 0:
        print("Iteration : {}".format(iteration))
    model.invoke([0])
    time.sleep_ms(4)
    
    
    

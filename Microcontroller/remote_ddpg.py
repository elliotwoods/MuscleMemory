import urequests
import gc
from wifi import wifi
from ubinascii import b2a_base64, a2b_base64
import tensorflow as tf
from motor import motor
from encoder import encoder
from oled import oled
from time import sleep_ms
from random import random, randrange
from math import sqrt

base_url = "http://172.30.1.55:8000"
id = b2a_base64(wifi.config('mac'))[:-1].decode('utf-8')

def get(url):
    response = urequests.get(base_url + url).json()
    if not response['success']:
        raise Exception("Error on server : " + response['exception'])
    return response['content']

def post(url, args):
    response = urequests.post(base_url + url, json=args).json()
    if not response['success']:
        raise Exception("Error on server : " + response['exception'])
    return response['content']

def find_max(values):
    max_value = values[0]
    max_index = 0
    
    for i in range(1, len(values)):
        value = values[i]
        if value > max_value:
            max_value = value
            max_index = i
    
    return max_index

class OUActionNoise:
    def __init__(self, mean, std_deviation, theta=0.15, dt=1e-2, x_initial=None):
        self.theta = theta
        self.mean = mean
        self.std_dev = std_deviation
        self.dt = dt
        self.x_initial = x_initial
        self.reset()
        
    def __call__(self):
        # Formula taken from https://www.wikipedia.org/wiki/Ornstein-Uhlenbeck_process.
        x = (
            self.x_prev
            + self.theta * (self.mean - self.x_prev) * self.dt
            + self.std_dev * sqrt(self.dt) * (random() - 0.5)
        )
        # Store x into x_prev
        # Makes next noise dependent on current one
        self.x_prev = x
        return x
    
    def reset(self):
        if self.x_initial is not None:
            self.x_prev = self.x_initial
        else:
            self.x_prev = 0
    
class Controller:
    def __init__(self):
        print("Loading model...")
        response = post("/startSession", {
            "client_id" : id,
            "options" : {
                "state_count" : 2,
                "action_count" : 1,
                "agent" : "DDPG"
            }
        })
        self.model = tf.Model()
        self.document_id = response['document_id']
        
        model_binary = a2b_base64(response['model'])
        self.model.load(model_binary)

        self.target = 0
        
        self.start_episode()
        
    def start_episode(self):
        self.value = encoder.value()
        self.last_value = self.value
        self.states = []
        self.actions = []
        self.rewards = []
        
        self.torque = 0
        self.noise = OUActionNoise(0, 0.5)
        
        self.update_action()
    
    def update_value(self):
        self.last_value = self.value
        self.value = encoder.value()

    def update_action(self):
        #state = [self.value, self.value - self.last_value, self.torque]
        state = [self.value, self.value - self.last_value]
        self.states.append(state)
        
        action = self.model.invoke(state)[0] + self.noise()
        #print(action, self.value)
        
        motor.set_torque(action)
        
        self.actions.append(action)
                     
    def update_reward(self):
        value = self.value
        self.rewards.append(-(value * value))
        
    def update(self):
        self.update_value()
        self.update_reward()
        self.update_action()
    
    def remote_update(self):
        motor.set_torque(0)
        
        response = post("/remoteUpdate", {
            "client_id" : id,
            "states" : self.states,
            "actions" : self.actions,
            "rewards" : self.rewards
        })
        
        model_binary = a2b_base64(response['model'])
        self.model.load(model_binary)
        
        self.start_episode()
        
controller = Controller()

def run():
    for episode in range(1000):
        print("Episode {}".format(episode))
        oled.display([wifi.ifconfig()[0], base_url, "Episode {}".format(episode)])
        
        if not episode is 0:
            controller.remote_update()
        
        if abs(encoder.value()) > 1 << 15:
            encoder.reset()
            
        for t in range(100):
            controller.update()
        sleep_ms(10)

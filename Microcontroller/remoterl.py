import urequests
import gc
from wifi import wifi
from ubinascii import b2a_base64, a2b_base64
import tensorflow as tf
from motor import Motor
from encoder import encoder
from oled import oled
from time import sleep_ms
from random import random, randrange

base_url = "http://172.30.1.55:8000"
id = b2a_base64(wifi.config('mac'))[:-1].decode('utf-8')
motor = Motor(7.7)
motor.min_duty = 1023 / 5

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
    
class Controller:
    def __init__(self, action_count=3):
        print("Loading network...")
        response = post("/startSession", {
            "client_id" : id,
            "options" : {
                "state_dimensions" : 3,
                "action_count" : action_count
            }
        })
        self.model = tf.Model()
        self.action_count = action_count
        self.document_id = response['document_id']
        
        model_binary = a2b_base64(response['model'])
        self.model.load(model_binary)

        self.target = 0
        
        self.epsilon = 1.0
        self.epsilon_min = 0.1
        self.epsilon_multiplier = 0.99
        
        self.start_episode()
        
    def start_episode(self):
        self.value = encoder.value()
        self.last_value = self.value
        self.states = []
        self.actions = []
        self.rewards = []
        
        self.torque = 0
        
        self.update_action()
    
    def update_value(self):
        self.last_value = self.value
        self.value = encoder.value()
        
    def update_action(self):
        state = [self.value, self.value - self.last_value, self.torque]
        self.states.append(state)
        
        on_policy = random() > max(self.epsilon, self.epsilon_min)
        
        if on_policy:
            actions = self.model.invoke(state)
            action = find_max(actions)
            #print("ON  {} -> {} [{}]".format(state, action, actions))
        else:
            action = randrange(self.action_count)
            #print("OFF {} -> {}".format(state, action))
        
        self.torque += (action - 1) / 10
        self.torque = min(self.torque, 1)
        self.torque = max(self.torque, -1)
        
        print(state, action)
        
        motor.set_torque(self.torque)
        #motor.set_torque(action / (self.action_count - 1) * 2 - 1) # map 0..action count to -1..1
        
        self.actions.append(action)
        
        self.epsilon *= self.epsilon_multiplier
                     
    def update_reward(self):
        value = self.value
        self.rewards.append(- (value * value))
        
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
for episode in range(1000):
    print("Episode {}".format(episode))
    
    if not episode is 0:
        controller.remote_update()
    
    for t in range(100):
        controller.update()
        sleep_ms(10)
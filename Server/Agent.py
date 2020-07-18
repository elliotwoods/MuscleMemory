import numpy as np
import tensorflow as tf

from tf_agents.networks import q_network
from tf_agents.agents.dqn import dqn_agent
from tf_agents.specs import TensorSpec, BoundedTensorSpec
from tf_agents.trajectories import time_step
from tf_agents.utils import common

from binascii import b2a_base64

tf.compat.v1.enable_v2_behavior()

default_options = {
	"state_dimensions" : 2,
	"action_count" : 3,
	"fc_layer_params" : [10, 10],
	"learning_rate" : 0.001
}

class Agent:
	def __init__(self, options = {}):
		# apply default options
		options = {**default_options, **options}

		self.observation_spec = TensorSpec((options['state_dimensions'],), np.dtype('float32'), 'observation')
		self.reward_spec = TensorSpec((1,), np.dtype('float32'), 'reward')
		self.action_spec = BoundedTensorSpec((1,), np.dtype('int32'), minimum=0, maximum=options['action_count'] - 1, name='action')
		self.time_step_spec = time_step.time_step_spec(self.observation_spec)

		self.optimizer = tf.compat.v1.train.AdamOptimizer(learning_rate=options['learning_rate'])
		self.q_network = q_network.QNetwork(self.observation_spec, self.action_spec, fc_layer_params=options['fc_layer_params'])
		self.agent = dqn_agent.DqnAgent(self.time_step_spec, self.action_spec
			, q_network = self.q_network
			, optimizer = self.optimizer
			, td_errors_loss_fn = common.element_wise_squared_loss
			, epsilon_greedy = 0.1
			, gamma = 0.9)

		self.agent.initialize()

		# Optimise by using the graph
		self.agent.train = common.function(self.agent.train)

		# Reset the training step
		self.agent.train_step_counter.assign(0)
	
	def get_model_byte_string(self):
		converter = tf.lite.TFLiteConverter.from_keras_model(self.q_network)
		converter.optimizations = [tf.lite.Optimize.DEFAULT]
		quantized_model = converter.convert()
		return quantized_model

	def get_model_string(self):
		binary_string = self.get_model_byte_string()
		string = b2a_base64(binary_string).decode('utf-8')
		return string

agent = Agent({})
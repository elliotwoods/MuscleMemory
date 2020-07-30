import numpy as np
import tensorflow as tf

from binascii import b2a_base64
from pydantic import BaseModel

tf.compat.v1.enable_v2_behavior()

default_options = {
	"state_dimensions" : 2,
	"action_count" : 5,
	"hidden_layers" : [16, 16, 16],
	"learning_rate" : 0.00025,
	"gamma" : 0.99, # discount factor,
	"batch_size" : 64
}

class Episode(BaseModel):
	states: list
	actions: list
	rewards: list
	next_states: list

class History:
	def __init__(self):
		self.episodes = []
		self.states = []
		self.actions = []
		self.rewards = []
		self.next_states = []
	
	def __len__(self):
		return len(self.states)

class Agent:
	def __init__(self, options = {}):
		super(Agent, self).__init__()

		# apply default options
		self.options = {**default_options, **options}

		# create the model
		initializer = tf.keras.initializers.RandomNormal()
		self.model = tf.keras.models.Sequential()
		input_shape = (self.options['state_dimensions'],)

		# add the hidden layers
		for hidden_layer_size in self.options['hidden_layers']:
			if input_shape is not None:
				self.model.add(tf.keras.layers.Dense(hidden_layer_size, activation='relu', input_shape=input_shape, kernel_initializer=initializer))
				input_shape = None
			else:
				self.model.add(tf.keras.layers.Dense(hidden_layer_size, activation='relu', kernel_initializer=initializer))
		
		# add the output layer
		self.model.add(tf.keras.layers.Dense(self.options['action_count'], activation='linear', kernel_initializer=initializer))

		# optimiser and loss function
		self.optimiser = tf.keras.optimizers.Adam(learning_rate=self.options['learning_rate'])
		self.loss_function = tf.keras.losses.MSE

		# initialise the RL parts
		self.history = History()
		self.gamma = self.options['gamma']
		self.batch_size = self.options['batch_size']
		self.replay_batches = 10
		self.target_model = tf.keras.models.clone_model(self.model)

		# test model
		print(self.model(tf.zeros((1, self.options['state_dimensions'])), tf.float32))
		
	
	def get_model_byte_string(self):
		#def representative_dataset():
		#	for i in range(500):
		#		yield([tf.constant([[0, 0]], dtype=np.dtype('float32'))])

		converter = tf.lite.TFLiteConverter.from_keras_model(self.model)
		converter.optimizations = [tf.lite.Optimize.DEFAULT]
		#converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
		#converter.representative_dataset = representative_dataset
		quantized_model = converter.convert()
		return quantized_model

	def get_model_string(self):
		binary_string = self.get_model_byte_string()
		string = b2a_base64(binary_string).decode('utf-8')
		return string
	
	def update(self, states, actions, rewards):
		# trim to lowest count
		# This is because generally we have 1 less reward than state or actions
		# Also we will takes states[i + 1] when training
		count = min(len(states) - 1, len(actions), len(rewards))

		episode = Episode(states=states[0:count]
			, actions=actions[0:count]
			, rewards=rewards[0:count]
			, next_states=states[1:count+1])

		self.history.episodes.append(episode)
		self.history.states += episode.states
		self.history.actions += episode.actions
		self.history.rewards += episode.rewards
		self.history.next_states += episode.next_states

		for i in range(self.replay_batches):
			self.train()
		self.target_model.set_weights(self.model.get_weights())		

	def train(self):
		if len(self.history) < self.batch_size:
			return

		random_indices = np.random.choice(range(len(self.history)), size=self.batch_size)

		states_sample = np.array([self.history.states[i] for i in random_indices])
		actions_sample = np.array([self.history.actions[i] for i in random_indices])
		rewards_sample = np.array([self.history.rewards[i] for i in random_indices])
		next_states_sample = np.array([self.history.next_states[i] for i in random_indices])

		future_rewards = self.target_model.predict(next_states_sample)
		lookahead_q_values = rewards_sample + self.gamma * tf.reduce_max(future_rewards, axis=1)

		# only update the q values which were acted upon
		update_mask = tf.one_hot(actions_sample, self.options['action_count'])

		with tf.GradientTape() as tape:
			q_values = self.model(states_sample)

			q_value_for_action = tf.reduce_sum(tf.multiply(q_values, update_mask), axis=1)

			loss = self.loss_function(lookahead_q_values, q_value_for_action)
		
		gradients = tape.gradient(loss, self.model.trainable_variables)
		self.optimiser.apply_gradients(zip(gradients, self.model.trainable_variables))


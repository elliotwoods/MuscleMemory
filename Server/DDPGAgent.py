import numpy as np
import tensorflow as tf
import rl
from tensorflow.keras import layers, models

from binascii import b2a_base64
from pydantic import BaseModel

from ReplayMemory import ReplayMemory

tf.compat.v1.enable_v2_behavior()

default_options = {
	"state_count" : 2,
	"action_count" : 1,
	"actor_hidden_layers" : [16, 16],
	"critic_state_hidden_layers" : [16, 32],
	"critic_action_hidden_layers" : [32],
	"critic_final_hidden_layers" : [32, 32],
	"learning_rate" : 0.001,
	"gamma" : 0.99, # discount factor,
	"batch_size" : 64,
	"batch_runs" : 16,
	"buffer_size" : 100000,
	"tau" : 1e-3 # target model update coefficient
}

class DDPGAgent:
	def __init__(self, options = {}):
		super(DDPGAgent, self).__init__()

		# apply default options
		self.options = options = {**default_options, **options}

		# create the actor
		x = input_layer = layers.Input(shape=(options['state_count'],))

		for hidden_layer_size in options['actor_hidden_layers']:
			x = layers.Dense(hidden_layer_size, activation="relu")(x)
			#x = layers.BatchNormalization()(x)

		x = layers.Dense(1
			, activation='tanh'
			, kernel_initializer=tf.random_uniform_initializer(minval=-0.003, maxval=0.003))(x)

		self.actor_model = tf.keras.Model(input_layer, x)
		self.actor_model_target = models.clone_model(self.actor_model)

		# create the critic
		x = state_input = layers.Input(shape=(options['state_count'],))
		
		for hidden_layer_size in options['critic_state_hidden_layers']:
			x = layers.Dense(hidden_layer_size, activation="relu")(x)
			x = layers.BatchNormalization()(x)
		
		state_x = x

		x = action_input = layers.Input(shape=(options['action_count'],))
		
		for hidden_layer_size in options['critic_action_hidden_layers']:
			x = layers.Dense(hidden_layer_size, activation="relu")(x)
			x = layers.BatchNormalization()(x)
		
		action_x = x

		x = layers.Concatenate()([state_x, action_x])
		
		for hidden_layer_size in options['critic_final_hidden_layers']:
			x = layers.Dense(hidden_layer_size, activation="relu")(x)
			x = layers.BatchNormalization()(x)
		
		x = layers.Dense(1)(x)

		self.critic_model = tf.keras.Model([state_input, action_input], x)
		self.critic_model_target = models.clone_model(self.critic_model)
		

		# create replay memory
		self.replay_memory = ReplayMemory(options['action_count']
			, options['state_count']
			, options['buffer_size'])

		# create the optimisers
		self.actor_optimizer = tf.keras.optimizers.Adam(learning_rate=options['learning_rate'])
		self.critic_optimizer = tf.keras.optimizers.Adam(learning_rate=options['learning_rate'])
		self.loss_function = tf.keras.losses.MAE


	def get_model_byte_string(self):
		converter = tf.lite.TFLiteConverter.from_keras_model(self.actor_model)
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

		for i in range(count):
			self.replay_memory.record(states[i], actions[i], rewards[i], states[i + 1])
			
		for i in range(self.options['batch_runs']):
			self.train()


	def train(self):
		state_batch, action_batch, reward_batch, next_state_batch = self.replay_memory.get_batch(self.options['batch_size'])

		with tf.GradientTape() as tape:
			next_actions = self.actor_model_target(next_state_batch)
			y = reward_batch + self.options['gamma'] * self.critic_model_target([next_state_batch, next_actions])
			critic_value = self.critic_model([state_batch, action_batch])
			critic_loss = self.loss_function(y, critic_value)

		critic_grad = tape.gradient(critic_loss, self.critic_model.trainable_variables)
		self.critic_optimizer.apply_gradients(
			zip(critic_grad, self.critic_model.trainable_variables)
		)

		with tf.GradientTape() as tape:
			actions = self.actor_model(state_batch)
			critic_value = self.critic_model([state_batch, actions])
			# Used `-value` as we want to maximize the value given
			# by the critic for our actions
			actor_loss = -tf.math.reduce_mean(critic_value)
		
		actor_grad = tape.gradient(actor_loss, self.actor_model.trainable_variables)
		self.actor_optimizer.apply_gradients(
			zip(actor_grad, self.actor_model.trainable_variables)
		)

		self.update_target_critic()


	# This update target parameters slowly
	# Based on rate `tau`, which is much less than one.
	def update_target_critic(self):
		tau = self.options['tau']
		new_weights = []
		target_variables = self.critic_model_target.weights
		for i, variable in enumerate(self.critic_model.weights):
			new_weights.append(variable * tau + target_variables[i] * (1 - tau))

		self.critic_model_target.set_weights(new_weights)

		new_weights = []
		target_variables = self.actor_model_target.weights
		for i, variable in enumerate(self.actor_model.weights):
			new_weights.append(variable * tau + target_variables[i] * (1 - tau))

		self.actor_model_target.set_weights(new_weights)

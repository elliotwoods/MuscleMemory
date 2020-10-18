import numpy as np
import tensorflow as tf

from tensorflow.keras import layers, models
import tensorboard

import base64
import datetime

from ReplayMemory import ReplayMemory
from multiprocessing import Lock

tf.compat.v1.enable_v2_behavior()

# Enable tensorboard debugging - currenty doesn't seem to work
#tf.debugging.experimental.enable_dump_debug_info("logs_debug")

default_options = {
	"state_count" : 6,
	"action_count" : 1,
	"actor_hidden_layers" : [16, 16],
	"critic_state_hidden_layers" : [64, 64],
	"critic_action_hidden_layers" : [64, 64],
	"critic_final_hidden_layers" : [64, 64],
	"learning_rate" : 0.001,
	"gamma" : 0.99, # discount factor,
	"batch_size" : 64,
	"buffer_size" : 100000,
	"tau" : 1e-1 # target model update coefficient. We choose a large value because we don't train for each sample arriving
}

# This should be split out into a seperate class (so should many things here)
class RuntimeParameters:
	def __init__(self):
		self.is_training = True
		self.noise_amplitude = 1.0
		self.add_proportional = -0.1
		self.add_constant = 0


class DDPGAgent:
	def __init__(self, client_id, options = {}):
		super(DDPGAgent, self).__init__()

		self.client_id = client_id

		# apply default options
		self.options = options = {**default_options, **options}

		self.init_actor(options)
		self.init_critic(options)

		# create replay memory
		self.replay_memory = ReplayMemory(options['action_count']
			, options['state_count']
			, options['buffer_size'])
		self.replay_memory_lock = Lock()

		# create the optimisers
		self.actor_optimizer = tf.keras.optimizers.Adam(learning_rate=options['learning_rate'])
		self.critic_optimizer = tf.keras.optimizers.Adam(learning_rate=options['learning_rate'])
		self.loss_function = tf.keras.losses.MAE

		# create the logger
		self.log_dir="logs/{}".format(datetime.datetime.now().strftime("%Y-%m-%d/%H.%M"))
		self.tensorboard = tf.summary.create_file_writer(self.log_dir)
		self.episode = 0
		self.time_step = 0

		# create the runtime parameters
		self.runtime_parameters = RuntimeParameters()

		# this parameter is populated when a new network is available
		#  and cleared to None whenever the client has the most recent copy
		self.fresh_model = self.get_model_base64()

		# Used for tensorboard
		self.training_index = 0

	def init_actor(self, options):
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
	
	def init_critic(self, options):
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
		
		x = layers.Dense(1, activation='linear')(x)

		self.critic_model = tf.keras.Model([state_input, action_input], x)
		self.critic_model_target = models.clone_model(self.critic_model)


	def get_model_byte_string(self):
		converter = tf.lite.TFLiteConverter.from_keras_model(self.actor_model)
		converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS
			, tf.lite.OpsSet.SELECT_TF_OPS]
		converter.optimizations = [tf.lite.Optimize.DEFAULT]
		#converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
		#converter.representative_dataset = representative_dataset
		quantized_model = converter.convert()
		return quantized_model


	def get_model_base64(self):
		binary_string = self.get_model_byte_string()
		string = base64.b64encode(binary_string)
		return string

	def get_fresh_model(self):
		fresh_model = self.fresh_model
		self.fresh_model = None
		return fresh_model

	def update_runtime_parameters(self):
		if self.replay_memory.buffer_counter < 10000:
			self.runtime_parameters.is_training = True
			self.runtime_parameters.noise_amplitude = 1 / 4
			self.runtime_parameters.add_proportional = -0.1
		elif self.replay_memory.buffer_counter < 20000:
			self.runtime_parameters.is_training = True
			self.runtime_parameters.noise_amplitude = 1 / 8
			self.runtime_parameters.add_proportional = -0.05
		elif self.replay_memory.buffer_counter < 30000:
			self.runtime_parameters.is_training = True
			self.runtime_parameters.add_proportional = -0.025
			self.runtime_parameters.noise_amplitude = 1 / 16
		elif self.replay_memory.buffer_counter < 40000:
			self.runtime_parameters.is_training = True
			self.runtime_parameters.noise_amplitude = 1 / 32
			self.runtime_parameters.add_proportional = -0.0125
		else:
			self.runtime_parameters.is_training = True
			self.runtime_parameters.noise_amplitude = 0
			self.runtime_parameters.add_proportional = 0
	

	# def update(self, states, actions, rewards):
	# 	# trim to lowest count
	# 	# This is because generally we have 1 less reward than state or actions
	# 	# Also we will takes states[i + 1] when training
	# 	count = min(len(states) - 1, len(actions), len(rewards))

	# 	for i in range(count):
	# 		self.replay_memory.record(states[i], actions[i], rewards[i], states[i + 1])
	# 		critic_loss, actor_loss = self.train()
		
	# 		with self.tensorboard.as_default():
	# 			record_index = self.time_step
	# 			tf.summary.scalar('critic_loss', critic_loss, step=record_index)
	# 			tf.summary.scalar('actor_loss', actor_loss, step=record_index)
	# 			tf.summary.scalar('mean_reward', np.mean(np.array(rewards)), step=record_index)
	# 			tf.summary.scalar('state', states[i][0], step=record_index)
	# 			tf.summary.scalar('action', actions[i], step=record_index)
	# 			tf.summary.scalar('reward', rewards[i], step=record_index)
			
	# 		print("Agent [{}] time step {}".format(self.client_id, self.time_step))

	# 		self.time_step += 1
	# 	self.episode += 1

	def train_with_batch(self, batch):
		with tf.GradientTape() as tape:
			next_actions = self.actor_model_target(batch.next_states)
			y = batch.rewards + self.options['gamma'] * self.critic_model_target([batch.next_states, next_actions])
			critic_value = self.critic_model([batch.states, batch.actions])
			critic_loss = self.loss_function(y, critic_value)
		
		critic_grad = tape.gradient(critic_loss, self.critic_model.trainable_variables)
		self.critic_optimizer.apply_gradients(
			zip(critic_grad, self.critic_model.trainable_variables)
		)

		with tf.GradientTape() as tape:
			actions = self.actor_model(batch.states)
			critic_value = self.critic_model([batch.states, actions])
			# Used `-value` as we want to maximize the value given
			# by the critic for our actions
			actor_loss = -tf.math.reduce_mean(critic_value)
		
		actor_grad = tape.gradient(actor_loss, self.actor_model.trainable_variables)
		self.actor_optimizer.apply_gradients(
			zip(actor_grad, self.actor_model.trainable_variables)
		)

		self.update_target_critic()

		critic_loss_mean = tf.reduce_mean(critic_loss)
		actor_loss_mean = tf.reduce_mean(actor_loss)

		with self.tensorboard.as_default():
			record_index = self.training_index
			tf.summary.scalar('critic_loss', critic_loss_mean, step=record_index)
			tf.summary.scalar('actor_loss', actor_loss_mean, step=record_index)

		self.training_index += 1

		return critic_loss_mean, actor_loss_mean

	def train(self):
		batch = self.replay_memory.get_batch(self.options['batch_size'])
		actor_loss, critic_loss = self.train_with_batch(batch)
		return actor_loss, critic_loss

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

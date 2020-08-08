
import numpy as np
import tensorflow as tf


def filter_trajectory(s, a, r, s_):
	max_velocity = 2000
	if abs(s_[0] - s[0]) > max_velocity or abs(s[1]) > max_velocity or abs(s_[1]) > max_velocity:
		raise Exception("Invalid trajectory", s, a, r, s_)

#adapted from https://keras.io/examples/rl/ddpg_pendulum/
class ReplayMemory:
	def __init__(self, action_count, state_count, buffer_capacity=100000):

		# Number of "experiences" to store at max
		self.buffer_capacity = buffer_capacity

		# Its tells us num of times record() was called.
		self.buffer_counter = 0

		# Instead of list of tuples as the exp.replay concept go
		# We use different np.arrays for each tuple element
		self.state_buffer = np.zeros((self.buffer_capacity, state_count), dtype=np.float32)
		self.action_buffer = np.zeros((self.buffer_capacity, action_count), dtype=np.float32)
		self.reward_buffer = np.zeros((self.buffer_capacity, 1), dtype=np.float32)
		self.next_state_buffer = np.zeros((self.buffer_capacity, state_count), dtype=np.float32)

	# Takes (s,a,r,s') obervation tuple as input
	def record(self, state, action, reward, next_state):
		filter_trajectory(state, action, reward, next_state)

		# Set index to zero if buffer_capacity is exceeded,
		# replacing old records
		index = self.buffer_counter % self.buffer_capacity

		self.state_buffer[index] = state
		self.action_buffer[index] = action
		self.reward_buffer[index] = reward
		self.next_state_buffer[index] = next_state

		self.buffer_counter += 1

	# We compute the loss and update parameters
	def get_batch(self, batch_size):
		# Get sampling range
		record_range = min(self.buffer_counter, self.buffer_capacity)

		# Randomly sample indices
		batch_indices = np.random.choice(record_range, batch_size)

		# HACK - only take the first samples repeatedly
		#batch_indices = np.arange(batch_size)

		# Convert to tensors
		state_batch = tf.convert_to_tensor(self.state_buffer[batch_indices])
		action_batch = tf.convert_to_tensor(self.action_buffer[batch_indices])
		reward_batch = tf.convert_to_tensor(self.reward_buffer[batch_indices])
		next_state_batch = tf.convert_to_tensor(self.next_state_buffer[batch_indices])

		return state_batch, action_batch, reward_batch, next_state_batch

	def save(self, filename):
		with open(filename, 'wb') as file:
			np.savez(file
				, state_buffer=self.state_buffer
				, action_buffer=self.action_buffer
				, reward_buffer=self.reward_buffer
				, next_state_buffer=self.next_state_buffer
				, buffer_counter=self.buffer_counter)
	
	def load(self, filename):
		with open(filename, 'rb') as file:
			load_data = np.load(filename)
		self.state_buffer = load_data['state_buffer']
		self.action_buffer = load_data['action_buffer']
		self.reward_buffer = load_data['reward_buffer']
		self.next_state_buffer = load_data['next_state_buffer']
		self.buffer_counter = load_data['buffer_counter'].item()
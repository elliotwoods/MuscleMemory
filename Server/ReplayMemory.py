
import numpy as np
import tensorflow as tf
import base64
import struct
import array

def filter_trajectory(s, a, r, s_):
	max_velocity = 2000
	if abs(s_[0] - s[0]) > max_velocity or abs(s[1]) > max_velocity or abs(s_[1]) > max_velocity:
		raise Exception("Invalid trajectory", s, a, r, s_)

class Batch:
	def __init__(self):
		self.states = None
		self.actions = None
		self.rewards = None
		self.next_states = None
	
	def __len__(self):
		return len(self.states)

#adapted from https://keras.io/examples/rl/ddpg_pendulum/
class ReplayMemory:
	def __init__(self, action_count, state_count, buffer_capacity=100000):

		# Number of "experiences" to store at max
		self.buffer_capacity = buffer_capacity

		# Its tells us num of times record() was called.
		self.buffer_counter = 0
		self.state_count = state_count
		self.action_count = action_count

		# Instead of list of tuples as the exp.replay concept go
		# We use different np.arrays for each tuple element
		self.state_buffer = np.zeros((self.buffer_capacity, state_count), dtype=np.float32)
		self.action_buffer = np.zeros((self.buffer_capacity, action_count), dtype=np.float32)
		self.reward_buffer = np.zeros((self.buffer_capacity, 1), dtype=np.float32)
		self.next_state_buffer = np.zeros((self.buffer_capacity, state_count), dtype=np.float32)
	
	def __len__(self):
		return min(self.buffer_counter, self.buffer_capacity)

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

	# Add a set of trajectories directly from the microcontroller (sent to us as a base64 encoded array of binary structs)
	def add_trajectories_base64(self, trajectories_base64):
		# base64 to binary
		binary = base64.b64decode(trajectories_base64)

		# binary to python objects
		struct_format = '< {0}f {1}f f {0}f'.format(self.state_count, self.action_count)
		trajectory_added_count = 0
		for trajectory_flat in struct.iter_unpack(struct_format, binary):
			state = np.array(trajectory_flat[0:self.state_count])
			action = np.array(trajectory_flat[self.state_count])
			reward = np.array(trajectory_flat[self.state_count + 1])
			next_state = np.array(trajectory_flat[self.state_count + 2:])
			self.record(state, action, reward, next_state)
			trajectory_added_count += 1
		print("Added {0} trajectories".format(trajectory_added_count))

	# We compute the loss and update parameters
	def get_batch(self, batch_size):
		# Get sampling range
		record_range = len(self)

		# Randomly sample indices
		batch_indices = np.random.choice(record_range, batch_size)

		# HACK - only take the first samples repeatedly
		#batch_indices = np.arange(batch_size)

		# Convert to tensors
		batch = Batch()
		batch.states = tf.convert_to_tensor(self.state_buffer[batch_indices])
		batch.actions = tf.convert_to_tensor(self.action_buffer[batch_indices])
		batch.rewards = tf.convert_to_tensor(self.reward_buffer[batch_indices])
		batch.next_states = tf.convert_to_tensor(self.next_state_buffer[batch_indices])

		return batch

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
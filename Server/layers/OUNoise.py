import tensorflow as tf
from tensorflow.python.ops import array_ops
from tensorflow.python.ops import math_ops
import numpy as np
from math import sqrt

class OUNoise(tf.keras.layers.Layer):
	def __init__(self, mean, std_deviation, min=-1, max=1, theta=0.15, dt=1e-2, x_initial=None):
		super(OUNoise, self).__init__()

		self.mean = mean
		self.std_deviation = std_deviation
		self.theta = theta
		self.dt = dt

		self.min = min
		self.max = max

		self.x_initial = x_initial


	def build(self, input_shape):
		self.reset(input_shape)
		
	def call(self, input):
		# Formula taken from https://www.wikipedia.org/wiki/Ornstein-Uhlenbeck_process.
		noise = tf.keras.backend.random_normal(
			shape = array_ops.shape(input)
			, mean=self.noise_previous
			, stddev = self.std_deviation
			, dtype = input.dtype) * sqrt(self.dt)

		noise += self.theta * (self.mean - self.noise_previous) * self.dt

		x = input + noise
		return tf.clip_by_value(x, self.min, self.max)
	
	def reset(self, input_shape):
		if self.x_initial is None:
			self.noise_previous = tf.zeros(input_shape[-1])
		elif type(self.x_initial) is float:
			self.noise_previous = tf.constant(self.x_initial, shape=(input_shape[-1]))
		elif type(self.x_initial) is np.ndarray:
			self.noise_previous = tf.convert_to_tensor(self.x_initial)
		elif type(self.x_initial) is tf.Tensor:
			self.noise_previous = self.x_initial
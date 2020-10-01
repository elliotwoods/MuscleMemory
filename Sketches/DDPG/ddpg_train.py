#%% imports
import sys, os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..', 'Server'))

from DDPGAgent import DDPGAgent
from ReplayMemory import ReplayMemory

import tensorflow as tf
from tensorflow.keras import layers, models

from matplotlib import pyplot as plt
import numpy as np

import datetime

# %% Load replay memory
replay_memory = ReplayMemory(1, 2)
replay_memory.load('replay.npz')

# %% Visualise a batch
s, a, r, s_ = replay_memory.get_batch(64)
plt.plot(s[:,0], s_[:,0], '.')
plt.xlabel('state')
plt.ylabel('nextstate')
plt.title('position')
plt.show()

plt.plot(s[:,1], s_[:,1], '.')
plt.xlabel('state')
plt.ylabel('nextstate')
plt.title('velocity')
plt.show()

# %%
plt.plot(s[:,0], s_[:,0] - s[:,0], '.')
plt.xlabel('state')
plt.ylabel('change')
plt.title('position')
plt.show()


plt.plot(s[:,1], s_[:,1] - s[:,1], '.')
plt.xlabel('state')
plt.ylabel('change')
plt.title('velocity')
plt.show()

# %% Setup training
state_input = layers.Input(shape=(2,))
action_input = layers.Input(shape=(1,))
x = layers.Concatenate()([state_input, action_input])
#x = layers.Dense(16, activation='relu')(x)
#x = layers.BatchNormalization()(x)
#x = layers.Dense(16, activation='relu')(x)
#x = layers.BatchNormalization()(x)
x = layers.Dense(1, activation='linear')(x)
model = tf.keras.Model([state_input, action_input], x)
loss_function = tf.keras.losses.MAE
optimizer = tf.keras.optimizers.Adam(learning_rate=0.001)

# %% Train
log_dir="logs/{}".format(datetime.datetime.now().strftime("%Y-%m-%d/%H.%M"))
tensorboard = tf.summary.create_file_writer(log_dir)

for epoch in range(100000):
	s, a, r, s_ = replay_memory.get_batch(128)
	delta = s_[:,0] - s[:,0]
	with tf.GradientTape() as tape:
		prediction = model([s, a])
		loss = loss_function(prediction[:,0], delta)
	grad = tape.gradient(loss, model.trainable_variables)
	optimizer.apply_gradients(zip(grad, model.trainable_variables))

	mean_loss = tf.reduce_mean(loss)
	mean_delta = tf.reduce_mean(delta)

	with tensorboard.as_default():
		tf.summary.scalar('loss', mean_loss, step=epoch)
		tf.summary.scalar('delta', mean_delta, step=epoch)
	
	if epoch % 100 is 0:
		print("Epoch {} loss {}".format(epoch, mean_loss))

	if mean_loss > 200:
	  	print("Loss is high")
	  	break

# %% 

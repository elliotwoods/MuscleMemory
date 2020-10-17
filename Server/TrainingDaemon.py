# This process runs constantly and constantly trains networks.
# When a new network is training,  it is serialised and put into a
# 	variable which is then sent to the client.
# Note : this is not compatible with the 'remoteUpdate' pattern

import time

def train(agent):
	agent.train()

def run_training_daemon(client_agents, client_agents_lock):
	print("Starting Training Daemon")
	while True:
		# Take a copy since we will iterate, and the list can change in the other process
		if client_agents_lock.acquire():
			client_agents_copy = client_agents.copy()
			client_agents_lock.release()

			for agent_id in client_agents_copy:
				agent = client_agents[agent_id]
				if agent.runtime_parameters.is_training:
					if agent.replay_memory_lock.acquire():
						if len(agent.replay_memory) > 0:
							batch = agent.replay_memory.get_batch(agent.options['batch_size'])
							agent.replay_memory_lock.release()

							if len(batch) > 0:
								print("Training {}".format(agent_id))
								agent.train_with_batch(batch)
								agent.fresh_model = agent.get_model_base64()
						else:
							agent.replay_memory_lock.release()
		time.sleep(1.0)


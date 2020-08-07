from DQNAgent import DQNAgent
from DDPGAgent import DDPGAgent

client_agents = {}

def get_agent(client_id):
	if not client_id in client_agents:
		raise Exception("No agent found for client {}".format(client_id))
	return client_agents[client_id]

def create_agent(client_id, options):
	global client_agents
	agent = DDPGAgent(client_id, options)
	client_agents[client_id] = agent
	return agent

def save_memory():
	for agent_name in client_agents:
		client_agents[agent_name].replay_memory.save(agent_name + ".npz")
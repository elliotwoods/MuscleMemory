from DQNAgent import DQNAgent
from DDPGAgent import DDPGAgent
from multiprocessing import Lock, Manager

client_agents = {}
client_agents_lock = Lock()

def get_agent(client_id):
	client_agents_lock.acquire()
	if not client_id in client_agents:
		client_agents_lock.release()
		raise Exception("No agent found for client {}".format(client_id))
	client_agent = client_agents[client_id]
	client_agents_lock.release()
	return client_agent

def create_agent(client_id, options):
	client_agents_lock.acquire()
	global client_agents
	agent = DDPGAgent(client_id, options)
	client_agents[client_id] = agent
	client_agents_lock.release()
	return agent

def save_memory():
	for agent_name in client_agents:
		client_agents[agent_name].replay_memory.save(agent_name + ".npz")
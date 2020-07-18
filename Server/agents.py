from Agent import Agent

agents = {}

def get_agent(client_id):
	if not client_id in agents:
		raise Exception("No agent found for client {}".format(client_id))
	return agents[client_id]

def create_agent(client_id, options):
	global agents
	agent = Agent(options)
	agents[client_id] = agent
	return agent
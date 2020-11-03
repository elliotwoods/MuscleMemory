
from TrainingDaemon import run_training_daemon
from threading import Thread
import agents

training_daemon = None

def ckeck_training_deamon():
	# This needs to be started from the web server process (not main)
	global training_daemon
	if training_daemon is None:
		training_daemon = Thread(target=run_training_daemon
			, args=(agents.client_agents, agents.client_agents_lock)
			, daemon=True, name="Training")
		training_daemon.start()
	
def simple_api(endpoint):
	def action():
		try:
			result = endpoint()
			return {
				"success" : True,
				"content" : result
			}
		except Exception as e:
			return {
				"success" : False,
				"exception" : str(e)
			}
	return action
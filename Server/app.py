import agents

from typing import Optional
from pydantic import BaseModel
from fastapi import FastAPI
import db
from datetime import datetime
import base64

from TrainingDaemon import run_training_daemon
from threading import Thread

app = FastAPI()
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
	ckeck_training_deamon()
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

@app.get("/testError")
@simple_api
def test_error():
	raise(Exception("Test exception"))

@app.get("/testSuccess")
@simple_api
def test_success():
	return True

class StartSessionRequest(BaseModel):
	client_id: str
	options: Optional[dict] = {}
	recycle_agent: Optional[bool] = True


@app.post("/startSession")
def new_session(request: StartSessionRequest):
	def action():	
		query = db.Query()

		# remove existing sessions
		db.sessions.remove(query.client_id == request.client_id)

		# remove existing samples
		db.samples.remove(query.client_id == request.client_id)

		# record the session started
		document_id = db.sessions.insert({
			'client_id' : request.client_id,
			'start_time' : datetime.now()
		})

		# create the agent
		agent = agents.create_agent(request.client_id, request.options, request.recycle_agent)

		return {
			"client_id" : request.client_id,
			"document_id" : document_id,
			"model" : agent.get_model_base64(),
			"runtime_parameters" : agent.runtime_parameters.__dict__
		}
	result = simple_api(action)()
	return result

# Old API : Don't use this any more
# 
# class RemoteUpdateRequest(BaseModel):
# 	client_id: str
# 	states: list
# 	actions: list
# 	rewards: list
# 
# @app.post("/remoteUpdate")
# def remoteUpdate(request: RemoteUpdateRequest):
# 	def action():
# 		agent = agents.get_agent(request.client_id)
		
# 		agent.update(request.states, request.actions, request.rewards)

# 		return {
# 			"model" : agent.get_model_string()
# 		}
# 	result = simple_api(action)()
# 	return result

class TransmitTrajectoriesRequest(BaseModel):
	client_id: str
	trajectories: str

@app.post("/transmitTrajectories")
def transmitTrajectories(request: TransmitTrajectoriesRequest):
	def action():
		agent = agents.get_agent(request.client_id)
		if agent.replay_memory_lock.acquire():
			agent.replay_memory.add_trajectories_base64(request.trajectories)
			agent.replay_memory_lock.release()
		
		fresh_model = agent.get_fresh_model()
		if fresh_model is not None:
			agent.update_runtime_parameters()
			return {
				"model" : fresh_model,
				"runtime_parameters" : agent.runtime_parameters.__dict__
			}
		else:
			return {

			}
	result = simple_api(action)()
	return result

@app.get("/saveMemory")
@simple_api
def saveMemory():
	agents.save_memory()
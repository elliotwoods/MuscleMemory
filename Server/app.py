import agents

from typing import Optional
from pydantic import BaseModel
from fastapi import FastAPI
import db
from datetime import datetime

app = FastAPI()

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
		agent = agents.create_agent(request.client_id, request.options)

		return {
			"client_id" : request.client_id,
			"document_id" : document_id,
			"model" : agent.get_model_string(),
			"is_training" : True
		}
	result = simple_api(action)()
	return result
	
class RemoteUpdateRequest(BaseModel):
	client_id: str
	states: list
	actions: list
	rewards: list

@app.post("/remoteUpdate")
def remoteUpdate(request: RemoteUpdateRequest):
	def action():
		agent = agents.get_agent(request.client_id)
		
		agent.update(request.states, request.actions, request.rewards)

		return {
			"model" : agent.get_model_string()
		}
	result = simple_api(action)()
	return result

class TransmitTrajectoriesRequest(BaseModel):
	client_id: str
	trajectories: str

@app.post("/transmitTrajectories")
def transmitTrajectories(request: TransmitTrajectoriesRequest):
	def action():
		print(request)
		return {

		}
	result = simple_api(action)()
	return result

@app.get("/saveMemory")
@simple_api
def saveMemory():
	agents.save_memory()
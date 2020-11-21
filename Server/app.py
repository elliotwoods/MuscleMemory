import agents

from typing import Optional
from pydantic import BaseModel
from fastapi import FastAPI
from fastapi.responses import RedirectResponse
from fastapi.staticfiles import StaticFiles

import db
from datetime import datetime
import base64

from routers import samplingSession, client, interface
client.interface = interface
interface.client = client

from utils import *

from pythonosc.udp_client import SimpleUDPClient
osc_client = SimpleUDPClient("127.0.0.1", 3355)

app = FastAPI()

app.mount("/static", StaticFiles(directory="static"), name="static")

app.include_router(
	samplingSession.router
	, prefix="/samplingSession"
	, tags=["samplingSession"]
)

app.include_router(
	client.router
	, prefix="/client"
	, tags=["client"]
)

app.include_router(
	interface.router
	, prefix="/interface"
	, tags=["interface"]
)

@app.get("/remote/move/{selection}/{movement}")
def getMovement(selection, movement):
	osc_client.send_message("/movement", [selection, int(movement)])
	return True

@app.get("/")
def root():
	return RedirectResponse(url="/static/index.html")

@app.get("/remote")
def test_error():
	return RedirectResponse(url="/static/remote/index.html")


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
		ckeck_training_deamon()

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


class TransmitTrajectoriesRequest(BaseModel):
	client_id: str
	trajectories: str

@app.post("/transmitTrajectories")
def transmitTrajectories(request: TransmitTrajectoriesRequest):
	def action():
		ckeck_training_deamon()

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

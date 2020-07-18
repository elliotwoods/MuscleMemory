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
	options: dict


@app.post("/startSession")
def new_session(args: dict):
	def action():
		if not 'client_id' in args:
			raise Exception('Missing client_id')
		if not 'options' in args:
			args['options'] = {}
		
		query = db.Query()

		# remove existing sessions
		db.sessions.remove(query.client_id == args['client_id'])

		# remove existing samples
		db.samples.remove(query.client_id == args['client_id'])

		# record the session started
		document_id = db.sessions.insert({
			'client_id' : args['client_id'],
			'start_time' : datetime.now()
		})

		# create the agent
		agent = agents.create_agent(args['client_id'], args['options'])

		return {
			"client_id" : args['client_id'],
			"document_id" : document_id,
			"model" : agent.get_model_string()
		}
	result = simple_api(action)()
	return result
	
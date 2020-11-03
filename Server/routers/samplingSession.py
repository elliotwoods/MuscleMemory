from fastapi import APIRouter
from utils import *
from pydantic import BaseModel
router = APIRouter()

class RunRequest(BaseModel):
	client_id: str
	registers = {}
	duration: float = 10.0

@router.post("/run")
async def initSamplingSession(request: RunRequest):
	def action():
		return {}
	
	return simple_api(action)()


class TransmitRequest(BaseModel):
	client_id: str

@router.post("/transmit")
async def transmitSamples(request: TransmitRequest):
	def action():
		return {}
	return simple_api(action)()

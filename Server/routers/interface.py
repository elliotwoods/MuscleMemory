from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from utils import * 
from pydantic import BaseModel
import msgpack
import json

client = None
router = APIRouter()

class Connection:
	def __init__(self, websocket : WebSocket):
		self.websocket : WebSocket = websocket
	
	async def send(self, message):
		await self.websocket.send_text(json.dumps(message))

	async def request_client_register_info(self):
		# Some may not have info data, if so, let's make a request to be handled later
		for client_connection in client.manager.active_connections:
			await client_connection.send({
				"request_register_info" : None
			})
	
	async def route_interface_to_client(self, request):
		for hardware_request in request:
			client_connection = client.manager.get_connection_by_hardware_id(hardware_request['hardware_id'])
			await client_connection.send(hardware_request['content'])


class ConnectionManager:
	def __init__(self):
		self.active_connections: List[Connection] = []

	async def on_connect(self, websocket: WebSocket):
		await websocket.accept()
		connection = Connection(websocket)
		self.active_connections.append(connection)
		await connection.request_client_register_info()
		return connection

	def on_disconnect(self, connection: Connection):
		self.active_connections.remove(connection)
	
	async def route_client_to_interface(self, device_connection, data: object):
		for connection in self.active_connections:
			await connection.send([{
				"hardware_id" : device_connection.hardware_id, 
				"content" : data
			}])
	
manager = ConnectionManager()

@router.websocket("/")
async def ws(websocket : WebSocket):
	print("Interface connected")
	connection = await manager.on_connect(websocket)
	try:
		while True:
			try:
				request = await websocket.receive_text()
				await connection.route_interface_to_client(json.loads(request));
			except WebSocketDisconnect:
				raise
			except Exception as e:
				print("Exception in interface websocket : {}".format(e))

	except WebSocketDisconnect:
		manager.on_disconnect(connection)
		print("Interface disconnected")


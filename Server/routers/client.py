from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from utils import * 
from pydantic import BaseModel
import msgpack

interface = None
router = APIRouter()

class Connection:
	def __init__(self, hardware_id : str, websocket : WebSocket):
		self.hardware_id : str = hardware_id
		self.websocket : WebSocket = websocket
		self.register_info = {}
		self.register_values = {}
		self.connection_manager = None
	
	async def on_data(self, data):
		if type(data) is bytes:
			data = msgpack.unpackb(data, raw=False)
		
		# bounce all data to interface (will be in json format)
		await self.notify_listeners(data)
	
	async def notify_listeners(self, data: object):
		if self.connection_manager is not None:
			await self.connection_manager.on_change(self, data)
	
	async def send(self, request_content):
		await self.websocket.send_bytes(msgpack.packb(request_content))


class ConnectionManager:
	def __init__(self):
		self.active_connections: List[Connection] = []

	async def on_connect(self, hardware_id: str, websocket: WebSocket):
		await websocket.accept()
		connection = Connection(hardware_id, 	websocket)
		connection.connection_manager = self
		self.active_connections.append(connection)
		return connection

	def on_disconnect(self, connection: Connection):
		self.active_connections.remove(connection)

	async def on_change(self, connection: Connection, data):
		await interface.manager.route_client_to_interface(connection, data)

	def get_connection_by_hardware_id(self, hardware_id):
		for connection in self.active_connections:
			if connection.hardware_id == hardware_id:
				return connection
		raise Exception("Cannot find hardware ID {}".format(hardware_id))

manager = ConnectionManager()

@router.websocket("/{hardware_id}")
async def ws(websocket : WebSocket, hardware_id: str):
	print("Incoming websocket connection")
	connection = await manager.on_connect(hardware_id, websocket)
	try:
		print("Client (HW ID : {}) connected".format(hardware_id))
		while True:
			try:
				request_binary = await websocket.receive_bytes()
				await connection.on_data(request_binary)
			except WebSocketDisconnect:
				raise
			except Exception as e:
				print("Exception in websocket (HW ID : {}) : {}".format(hardware_id, e))

	except:
		manager.on_disconnect(connection)
		print("Client ({}) disconnected".format(hardware_id))

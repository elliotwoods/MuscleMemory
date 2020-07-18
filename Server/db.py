from tinydb import TinyDB, Query
from tinydb.storages import JSONStorage
from tinydb.middlewares import CachingMiddleware

sessions = TinyDB('sessions.json', storage=CachingMiddleware(JSONStorage))
samples = TinyDB('samples.json', storage=CachingMiddleware(JSONStorage))

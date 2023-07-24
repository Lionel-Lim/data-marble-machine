import json
import firebase_admin
from firebase_admin import credentials, firestore

json_data = "code/dbTransfer/energycelab-default-rtdb-EM-export.json"

# Init Firebase
cred = credentials.Certificate(
    "code/dbTransfer/energycelab-firebase-adminsdk-gekvr-9b574d3de2.json"
)
firebase_admin.initialize_app(cred)
db = firestore.client()

data = json.loads(json_data)

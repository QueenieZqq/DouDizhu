import sqlite3
import requests
import datetime
import hashlib
import random
import numpy as np

visits_db = "__HOME__/week04/FTL.db"

def request_handler(request):
    if request['method'] == 'GET':
        if request['values']['action']=="games": #getting all the ids of the games that already finishes 
            conn = sqlite3.connect(visits_db)
            c = conn.cursor()
            entries = c.execute('''SELECT * FROM games WHERE status = ?;''', ('Finished',)).fetchall()
            conn.commit()
            conn.close()
            return entries 

        if request['values']['action']=="status":
            game_ID = request['values']['game_id']
            conn = sqlite3.connect(visits_db)
            c = conn.cursor()
            entries =  c.execute('''SELECT * from games WHERE game_id = ?;''', (game_ID,)).fetchall()
            conn.commit()
            conn.close()
            landlord = entries[0][5]
            peasant1 = entries[0][6]
            peasant2 = entries[0][7]
            return "{},{},{}".format(landlord, peasant1, peasant2)

        if request['values']['action']=="entries":
            game_ID = request['values']['game_id']
            conn = sqlite3.connect(visits_db)
            c = conn.cursor()
            entries =  c.execute('''SELECT * from replay WHERE game_id = ? ORDER BY timing ASC;''', (game_ID,)).fetchall()
            conn.commit()
            conn.close()
            return entries

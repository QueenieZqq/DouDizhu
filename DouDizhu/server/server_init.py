import sqlite3
import requests
visits_db = "__HOME__/week04/FTL.db"

def request_handler(request):
    if request['method'] == 'POST':
        action = request['values']['action']
        if action == "init":
            #init user and games db
            conn = sqlite3.connect(visits_db)
            c = conn.cursor()
            c.execute('''CREATE TABLE IF NOT EXISTS users (username text, password text, game_id int, cards_in_hand text, action text, scoring int, timing timestamp);''')
            c.execute('''CREATE TABLE IF NOT EXISTS games (game_id int, cards_last_played text, last_player text, current_player text, status text, landlord text, peasant1 text, peasant2 text);''')
            c.execute('''CREATE TABLE IF NOT EXISTS replay (game_id int, timing timestamp, cards_last_played text, last_player text, current_player text, cards_landlord text, cards_peasant1 text, cards_peasant2 text);''')

            #close connection
            conn.commit()
            conn.close()
            return "init done."
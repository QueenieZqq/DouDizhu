import sqlite3
import requests
import datetime
import hashlib
import random
import numpy as np

visits_db = "__HOME__/week04/FTL.db"
MAX_GAME_ID = 10000

def encrypt_string(hash_string):
    sha_signature = hashlib.sha256(hash_string.encode()).hexdigest()
    return sha_signature

RANK = ('3', '4', '5', '6', '7', '8', '9', 'X', 'J', 'Q', 'K', 'A', '2', 'o', 'O')
SUIT_RANK = ('C', 'D', 'H', 'S', 'o', 'O')

def shuffleDeliverCard():
    deck = []
    for i in RANK[:-2]:
        for j in SUIT_RANK[:-2]:
            deck.append(i + j)
    deck.append('oo')
    deck.append('OO')
    random.shuffle(deck)
    cards_before_landlord = [deck[:17], deck[17: 34], deck[34: 51]]
    best_hand_player = -1
    max_count = -1
    for i in range(3):
        count = 0
        for card in cards_before_landlord[i]:
            if card[0] == '2' or card[0] == 'o' or card[0] == 'O':
                count = count + 1
        if count > max_count:
            max_count = count
            best_hand_player = i

    cards_to_return = {0: cards_before_landlord[(best_hand_player + 1) % 3],
                        1: cards_before_landlord[(best_hand_player + 2) % 3],
                        2: cards_before_landlord[best_hand_player] + deck[51:]}

    # sort cards before returning
    def value(card):
        return RANK.index(card[0]) * 10 + SUIT_RANK.index(card[1])

    for i in cards_to_return:
        sorted_cards = sorted(cards_to_return[i], key = lambda x: value(x))
        cards = ''
        for j in range(len(sorted_cards)):
            cards += (',' + sorted_cards[j])
        cards_to_return[i] = cards[1:]
    return cards_to_return

def request_handler(request):
    if request['method'] == 'POST':
        if request['values']['action']=="DB_users":
            conn = sqlite3.connect(visits_db)
            c = conn.cursor()
            entries = c.execute('''SELECT * FROM users;''').fetchall()
            conn.commit()
            conn.close()
            return entries
        if request['values']['action']=="DB_games":
            conn = sqlite3.connect(visits_db)
            c = conn.cursor()
            entries = c.execute('''SELECT * FROM games;''').fetchall()
            conn.commit()
            conn.close()
            return entries
        if request['values']['action']=="DB_replay":
            conn = sqlite3.connect(visits_db)
            c = conn.cursor()
            entries = c.execute('''SELECT * FROM replay;''').fetchall()
            conn.commit()
            conn.close()
            return entries            
        if request['values']['action']=="create_user":
            #example url: http://608dev-2.net/sandbox/sc/team018/user_handler.py?action=create_user&username=sfang&password=1811
            username = request['values']['username'].upper()
            password = encrypt_string(request['values']['password'])
            conn = sqlite3.connect(visits_db)
            c = conn.cursor()
            entries = c.execute('''SELECT username FROM users WHERE username = ? ;''',(username,)).fetchall()
            if len(entries)!=0:
                return "ERROR: Username already taken"
            c.execute('''INSERT into users VALUES (?,?,?,?,?,?,?);''', (username, password, 0, None, None, 0, datetime.datetime.now()))
            conn.commit()
            conn.close()
            return "SUCCESS: Welcome, " + username

        if request['values']['action']=="log_in":
            #example url: http://608dev-2.net/sandbox/sc/team018/user_handler.py?action=log_in&username=sfang&password=1811
            username = request['values']['username'].upper()
            password = encrypt_string(request['values']['password'])
            conn = sqlite3.connect(visits_db)
            c = conn.cursor()
            entries = c.execute('''SELECT password FROM users WHERE username = ? ;''',(username,)).fetchall()
            if len(entries)==0:
                return "ERROR: no such user"
            if password != entries[0][0]:
                return "ERROR: invalid credentials"
            return "SUCCESS: Welcome, " + username

        if request['values']['action']=='cancel_play':
            username = request['values']['username'].upper()
            conn = sqlite3.connect(visits_db)
            c = conn.cursor()
            c.execute('''UPDATE users SET action = ? WHERE username = ?;''',(None,username))
            conn.commit()
            conn.close()
            return "SUCCESS"

        if request['values']['action']=="play":
            username = request['values']['username'].upper()
            conn = sqlite3.connect(visits_db)
            c = conn.cursor()

            #if current user is already in game
            # return c.execute('''SELECT * FROM users''').fetchall()
            status = c.execute('''SELECT game_id FROM users WHERE username =? AND action=?;''',(username,'playing')).fetchall()
            if len(status)>0:
                conn.commit()
                conn.close()
                return "game_id: {}".format(str(status[0]), )


            #update action
            c.execute('''UPDATE users SET action = ? WHERE username = ?;''',('waiting',username))
            #update id
            people_waiting = c.execute('''SELECT username FROM users WHERE action=?;''',('waiting',)).fetchall()

            if len(people_waiting)>=3:
                #choose a valid game_ID
                flag = True
                game_ID = random.randint(1, MAX_GAME_ID)
                while flag:
                    game_ID = random.randint(1, MAX_GAME_ID)
                    flag = False
                    duplicate = c.execute('''SELECT status from games WHERE game_id = ?;''', (game_ID,)).fetchall()
                    if len(duplicate)>0:
                        if duplicate == "in progress":
                            flag = True
                        else:
                            c.execute('''DELETE * from games WHERE game_id = ?''',(game_ID,))

                #shuffling card
                shuffled_cards = shuffleDeliverCard()
                for i in range(3):
                    c.execute('''UPDATE users SET game_id = ? WHERE username = ?;''',(game_ID, people_waiting[i][0]))
                    c.execute('''UPDATE users SET action = ?  WHERE username = ?;''',("playing", people_waiting[i][0]))
                    c.execute('''UPDATE users SET cards_in_hand = ? WHERE username = ?;''',(shuffled_cards[i], people_waiting[i][0]))

                c.execute('''INSERT into games VALUES (?,?,?,?,?,?,?,?);''',(game_ID, "", "", people_waiting[2][0], "in-progress", people_waiting[2][0], people_waiting[0][0], people_waiting[1][0]))
	            #game_id	timing	cards_last_played	last_player	current_player	cards_landlord	cards_peasant1	cards_peasant2																			
                c.execute('''INSERT into replay VALUES (?,?,?,?,?,?,?,?);''',(game_ID, datetime.datetime.now(), "", "", people_waiting[2][0], shuffled_cards[2], shuffled_cards[0], shuffled_cards[1]))                
                conn.commit()
                conn.close()

                return "game_id: {}".format(game_ID, )

            conn.commit()
            conn.close()
            return "Waiting"
                
        if request['values']['action']=="end":
            username = request['values']['username'].upper()
            conn = sqlite3.connect(visits_db)
            c = conn.cursor()
            c.execute('''UPDATE users SET game_id = ? WHERE username = ?;''',(None,username))
            c.execute('''UPDATE users SET action=? WHERE username=?;''',(None,username))
            conn.commit()
            conn.close()
            
            return "current game ended!"

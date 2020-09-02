import sqlite3
import requests
import datetime
import random
from collections import Counter


visits_db = "__HOME__/week04/FTL.db"

def cards_to_number(cards):
    return int((len(cards) + 1)/3)

RANK = ('3', '4', '5', '6', '7', '8', '9', 'X', 'J', 'Q', 'K', 'A', '2', 'o', 'O')

def get_multiplicity(cards_played):
        cards_multiplicity = [0] * len(RANK) #multiplicity of cards played
        for card in cards_played:
                cards_multiplicity[RANK.index(card)] += 1
        return cards_multiplicity

#given a list cards_played (e.g. ['3','4','5','6','7']), return category of hand, or None if illegal.
def is_legal_hand(cards_played):
        cards_played = [i[0] for i in cards_played]
        cards_multiplicity = get_multiplicity(cards_played)

        def check_chain(m, restricted = True):

                if restricted:
                        if '2' in cards_played or 'o' in cards_played or 'O' in cards_played:
                                return False

                ones = False
                count = 0

                for i in range(len(cards_multiplicity)):
                        if cards_multiplicity[i] == 0:
                                if ones == True:
                                        ones = False
                        elif cards_multiplicity[i] == m:
                                if i >= RANK.index('2'):
                                        return False
                                if ones == False:
                                        if count == 0:
                                                ones = True
                                                count += 1
                                        else:
                                                return False
                                else:
                                        count += 1
                        else:
                                if restricted:
                                        return False
                if m == 1 and count >= 5:
                        return True
                elif m == 2 and count >= 3:
                        return True
                elif m == 3 and count >=2:
                        return True
                else:
                        return False

        if check_chain(1):
                return 'solo chain'
        if check_chain(2):
                return 'pair chain'
        if check_chain(3):
                return 'trio chain'

        if len(cards_played) == 1:
                return 'solo'

        if len(cards_played) == 2:
                if 'o' in cards_played and 'O' in cards_played:
                        return 'bomb'
                elif 2 in cards_multiplicity:
                        return 'pair'
                return None

        if len(cards_played) == 3:
                if 3 in cards_multiplicity:
                        return 'trio'
                return None

        if len(cards_played) == 4:
                if 4 in cards_multiplicity:
                        return 'bomb'
                else:
                        if 1 in cards_multiplicity and 3 in cards_multiplicity:
                                return 'trio and solo'
                        else:
                                return None

        if len(cards_played) == 5:
                if 3 in cards_multiplicity and 2 in cards_multiplicity:
                        return 'trio and pair'
                return None

        if len(cards_played) == 6:
                if 4 in cards_multiplicity:
                        return 'quad and double solo'
                return None

        if len(cards_played) == 8:
                if check_chain(3, False):
                        return 'double trio and solo'
                if 4 in cards_multiplicity and sum([i == 2 for i in cards_multiplicity]) == 2:
                        return 'quad and double pair'
                return None

        if len(cards_played) == 10:
                if check_chain(3, False):
                        if sum([i == 2 for i in cards_multiplicity]) == 2:
                                return 'double trio and pair'
                return None

        if len(cards_played) == 12:
                if check_chain(3, False):
                        if sum([i == 3 for i in cards_multiplicity]) == 3:
                                return 'triple trio and solo'
                return None

        if len(cards_played) == 15:
                if check_chain(3, False):
                        if sum([i == 3 for i in cards_multiplicity]) == 3 and sum([i == 2 for i in cards_multiplicity]) == 3:
                                return 'triple trio and pair'
                return None

        if len(cards_played) == 16:
                if check_chain(3, False):
                        if sum([i == 3 for i in cards_multiplicity]) == 4:
                                return 'quadriple trio and solo'
                return None

        if len(cards_played) == 20:
                if check_chain(3, False):
                        if sum([i == 3 for i in cards_multiplicity]) == 5:
                                return 'quintuple trio and solo'
                        if sum([i == 3 for i in cards_multiplicity]) == 4 and sum([i == 2 for i in cards_multiplicity]) == 4:
                                return 'quadriple trio and pair'
                return None
        return None


#given two lists prev_hand and current_hand return if current_hand can be played after prev_hand

def is_greater_than(prev_hand, current_hand):

        prev_hand_type = is_legal_hand(prev_hand)
        current_hand_type = is_legal_hand(current_hand)
        prev_hand = [i[0] for i in prev_hand]
        current_hand = [i[0] for i in current_hand]
        prev_mult = get_multiplicity(prev_hand)
        current_mult = get_multiplicity(current_hand)

        if current_hand_type == 'bomb':
                if prev_hand_type == 'bomb':
                        if len(current_hand) == 2:
                                return True
                        elif len(prev_hand) == 2:
                                return False
                        else:
                                prev_mult.index(4) < current_mult.index(4)
                else:
                        return True

        if current_hand_type != prev_hand_type:
                return False

        if len(prev_hand) != len(current_hand):
                        return False

        if current_hand_type in {'solo', 'solo chain'}:
                return prev_mult.index(1) < current_mult.index(1)

        if current_hand_type in {'pair', 'pair chain'}:
                return prev_mult.index(2) < current_mult.index(2)

        if current_hand_type in {'trio', 'trio and solo', 'trio and pair', 'trio chain', 'double trio and solo',
                'double trio and pair', 'triple trio and solo', 'triple trio and pair', 'quadriple trio and solo',
                'quadriple trio and pair', 'quintuple trio and solo'}:
                return prev_mult.index(3) < current_mult.index(3)

        if current_hand_type in {'quad and double solo', 'quad and double pair'}:
                return prev_mult.index(4) < current_mult.index(4)

#handling all the post request
def request_handler(request):
    game_ID = request['values']['game_ID']
    username = request['values']['username'].upper()

    if request['method'] == "GET": #getting the current progress of the game
        conn = sqlite3.connect(visits_db)
        c = conn.cursor()

        cards_in_hand = c.execute('''SELECT cards_in_hand FROM users WHERE username = ? ;''',(username,)).fetchall()[0][0]
        cards_last_played = c.execute('''SELECT cards_last_played FROM games WHERE game_id = ? ;''',(game_ID,)).fetchall()[0][0]
        last_player = c.execute('''SELECT last_player FROM games WHERE game_id = ? ;''',(game_ID,)).fetchall()[0][0]
        current_player = c.execute('''SELECT current_player FROM games WHERE game_id = ? ;''',(game_ID,)).fetchall()[0][0]
        landlord = c.execute('''SELECT landlord FROM games WHERE game_id = ? ;''',(game_ID,)).fetchall()[0][0]
        num_cards_landlord = cards_to_number(c.execute('''SELECT cards_in_hand FROM users WHERE username = ? ;''',(landlord,)).fetchall()[0][0])
        peasant1 = c.execute('''SELECT peasant1 FROM games WHERE game_id = ? ;''',(game_ID,)).fetchall()[0][0]
        num_cards_peasant1 = cards_to_number(c.execute('''SELECT cards_in_hand FROM users WHERE username = ? ;''',(peasant1,)).fetchall()[0][0])
        peasant2 = c.execute('''SELECT peasant2 FROM games WHERE game_id = ? ;''',(game_ID,)).fetchall()[0][0]
        num_cards_peasant2 = cards_to_number(c.execute('''SELECT cards_in_hand FROM users WHERE username = ? ;''',(peasant2,)).fetchall()[0][0])

        conn.commit()
        conn.close()
        # return current_player
        return "{}%{}%{}%{}%{}%{}%{}%{}%{}%{}".format(cards_in_hand, cards_last_played, last_player,
            landlord, peasant1, peasant2, current_player,
            int(num_cards_landlord), int(num_cards_peasant1), int(num_cards_peasant2))

    if request['method'] == 'POST':
        conn = sqlite3.connect(visits_db)
        c = conn.cursor()

        hand = request['values']['cards']
        current_player = c.execute('''SELECT current_player FROM games WHERE game_id = ? ;''',(game_ID,)).fetchall()[0][0]
        cards_last_played = c.execute('''SELECT cards_last_played FROM games WHERE game_id = ? ;''',(game_ID,)).fetchall()[0][0]
        last_player = c.execute('''SELECT last_player FROM games WHERE game_id = ? ;''',(game_ID,)).fetchall()[0][0]

        if username!= current_player:
            return "Not your turn"

        if hand!="" and is_legal_hand(hand.split(",")) == None:
            return "Not legal hand"

        if hand!="" and cards_last_played!="" and not(is_greater_than(cards_last_played.split(","), hand.split(","))) and current_player!=last_player:
            return "Not valid"

        if hand == "": # user choose to skip this round
            #find next player to put into current player landlord->peasant1->pesant2->landlord
            landlord = c.execute('''SELECT landlord FROM games WHERE game_id = ? ;''',(game_ID,)).fetchall()[0][0]
            peasant1 = c.execute('''SELECT peasant1 FROM games WHERE game_id = ? ;''',(game_ID,)).fetchall()[0][0]
            peasant2 = c.execute('''SELECT peasant2 FROM games WHERE game_id = ? ;''',(game_ID,)).fetchall()[0][0]
            next_mapping = {landlord:peasant1, peasant1:peasant2, peasant2:landlord }
            c.execute('''UPDATE games SET current_player = ? WHERE game_id = ?;''',(next_mapping[current_player], game_ID))

        else:
            #last_player = current_player
            c.execute('''UPDATE games SET last_player = ? WHERE game_id = ?;''',(current_player,game_ID))

            #find next player to put into current player landlord->peasant1->pesant2->landlord
            landlord = c.execute('''SELECT landlord FROM games WHERE game_id = ? ;''',(game_ID,)).fetchall()[0][0]
            peasant1 = c.execute('''SELECT peasant1 FROM games WHERE game_id = ? ;''',(game_ID,)).fetchall()[0][0]
            peasant2 = c.execute('''SELECT peasant2 FROM games WHERE game_id = ? ;''',(game_ID,)).fetchall()[0][0]
            next_mapping = {landlord:peasant1, peasant1:peasant2, peasant2:landlord }
            c.execute('''UPDATE games SET current_player = ? WHERE game_id = ?;''',(next_mapping[current_player], game_ID))

            #update cards_last_played = hand
            c.execute('''UPDATE games SET cards_last_played = ? WHERE game_id = ?;''',(hand,game_ID))

            #change cards current player has
            holding_cards_before = c.execute('''SELECT cards_in_hand FROM users WHERE username = ?;''',(username,)).fetchall()[0][0]
            holding_cards_before = holding_cards_before.split(",")
            hands_ridden = hand.split(",")
            holding_cards = []
            for cards in holding_cards_before:
                if cards not in hands_ridden:
                    holding_cards.append(cards)
            holding_cards = ','.join(holding_cards)
            c.execute('''UPDATE users SET cards_in_hand = ? WHERE username = ?;''',(holding_cards, username,))

        #change status if game is terminated
        landlord = c.execute('''SELECT landlord FROM games WHERE game_id = ? ;''',(game_ID,)).fetchall()[0][0]
        cards_landlord = c.execute('''SELECT cards_in_hand FROM users WHERE username = ? ;''',(landlord,)).fetchall()[0][0]
        num_cards_landlord = cards_to_number(cards_landlord)
        
        peasant1 = c.execute('''SELECT peasant1 FROM games WHERE game_id = ? ;''',(game_ID,)).fetchall()[0][0]
        cards_peasant1 = c.execute('''SELECT cards_in_hand FROM users WHERE username = ? ;''',(peasant1,)).fetchall()[0][0]
        num_cards_peasant1 = cards_to_number(cards_peasant1)
        
        peasant2 = c.execute('''SELECT peasant2 FROM games WHERE game_id = ? ;''',(game_ID,)).fetchall()[0][0]
        cards_peasant2 = c.execute('''SELECT cards_in_hand FROM users WHERE username = ? ;''',(peasant2,)).fetchall()[0][0]
        num_cards_peasant2 = cards_to_number(cards_peasant2)
        
        #game_id	timing	cards_last_played	last_player	current_player	cards_landlord	cards_peasant1	cards_peasant2																			
        c.execute('''INSERT into replay VALUES (?,?,?,?,?,?,?,?);''',(game_ID, datetime.datetime.now(), hand, current_player, next_mapping[current_player], cards_landlord, cards_peasant1, cards_peasant2))                

        if num_cards_landlord==0 or num_cards_peasant1==0 or num_cards_peasant2==0:
            c.execute('''UPDATE games SET status = ? WHERE game_id = ?;''',("Finished", game_ID))
            #set user's game ID to be 0 again
            c.execute('''UPDATE users SET game_id = ?, action = ? WHERE username = ?;''',(0, None, landlord))
            c.execute('''UPDATE users SET game_id = ?, action = ? WHERE username = ?;''',(0, None, peasant1))
            c.execute('''UPDATE users SET game_id = ?, action = ? WHERE username = ?;''',(0, None, peasant2))


        conn.commit()
        conn.close()

        if num_cards_landlord < 1:
            return "Winner is the landlord!"
        elif num_cards_peasant1 < 1 or num_cards_peasant2 < 1:
            return "Winners are the peasants!"
        else:
            return "Winner is unknown."
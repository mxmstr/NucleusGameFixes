from Command import Command

class CMD_GET_RANKING(Command):

    def execute(self, data):

        data = {'crypto_type': 'COMPOUND', 'flowid': None, 'msgid': 'CMD_GET_RANKING', 'ranking_list': [{'disp_rank': 1, 'fob_grade': 11, 'is_grade_top': 1, 'league_grade': 28, 'player_info': {'npid': {'handler': {'data': '', 'dummy': [0, 0, 0], 'term': 0}, 'opt': [0, 0, 0, 0, 0, 0, 0, 0], 'reserved': [0, 0, 0, 0, 0, 0, 0, 0]}, 'player_id': 973669, 'player_name': '76561198021929084_player01', 'ugc': 1, 'xuid': 76561198021929084}, 'rank': 1, 'score': 2147483647}], 'ranking_num': 1, 'result': 'NOERR', 'rqid': 0, 'update_date': 1727194226, 'xuid': None}

        return self._receiver.action(data, self.__class__.__name__)

"""
{'compress': False, 'data': {'event_id': 0, 'get_type': 'BEST', 'index': 0, 'is_new': 0, 'msgid': 'CMD_GET_RANKING', 'num': 1, 'rqid': 0, 'type': 'SNEAK'}, 'original_size': 111, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
{'compress': True, 'data': {'crypto_type': 'COMPOUND', 'flowid': None, 'msgid': 'CMD_GET_RANKING', 'ranking_list': [{'disp_rank': 1, 'fob_grade': 11, 'is_grade_top': 1, 'league_grade': 28, 'player_info': {'npid': {'handler': {'data': '', 'dummy': [0, 0, 0], 'term': 0}, 'opt': [0, 0, 0, 0, 0, 0, 0, 0], 'reserved': [0, 0, 0, 0, 0, 0, 0, 0]}, 'player_id': 973669, 'player_name': '76561198021929084_player01', 'ugc': 1, 'xuid': 76561198021929084}, 'rank': 1, 'score': 2147483647}], 'ranking_num': 1, 'result': 'NOERR', 'rqid': 0, 'update_date': 1727194226, 'xuid': None}, 'original_size': 476, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
"""
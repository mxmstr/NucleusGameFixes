from Command import Command

class CMD_OPEN_WORMHOLE(Command):

    def execute(self, data):

        data = {'crypto_type': 'COMPOUND', 'flowid': None, 'is_new_open': 1, 'msgid': 'CMD_OPEN_WORMHOLE', 'player_id': 13550, 'result': 'NOERR', 'rqid': 0, 'to_player_id': 595034, 'xuid': None}

        return self._receiver.action(data, self.__class__.__name__)

"""
{'compress': False, 'data': {'flag': 'BLACK', 'is_open': 1, 'msgid': 'CMD_OPEN_WORMHOLE', 'player_id': 13550, 'retaliate_score': 252, 'rqid': 0, 'to_player_id': 595034}, 'original_size': 127, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
{'compress': False, 'data': {'crypto_type': 'COMPOUND', 'flowid': None, 'is_new_open': 1, 'msgid': 'CMD_OPEN_WORMHOLE', 'player_id': 13550, 'result': 'NOERR', 'rqid': 0, 'to_player_id': 595034, 'xuid': None}, 'original_size': 162, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
"""
from Command import Command

class CMD_GET_FOB_STATUS(Command):

    def execute(self, data):

        data = {'crypto_type': 'COMPOUND', 'flowid': None, 'fob_index': -1, 'is_rescue': 0, 'is_reward': 0, 'kill_count': 0, 'msgid': 'CMD_GET_FOB_STATUS', 'record': {'defense': {'lose': 4, 'win': 1}, 'insurance': 0, 'score': 34, 'shield_date': 0, 'sneak': {'lose': 19, 'win': 1}}, 'result': 'NOERR', 'rqid': 0, 'sneak_mode': -1, 'xuid': None}

        return self._receiver.action(data, self.__class__.__name__)

"""
{'compress': False, 'data': {'msgid': 'CMD_GET_FOB_STATUS', 'rqid': 0}, 'original_size': 39, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
{'compress': True, 'data': {'crypto_type': 'COMPOUND', 'flowid': None, 'fob_index': -1, 'is_rescue': 0, 'is_reward': 0, 'kill_count': 0, 'msgid': 'CMD_GET_FOB_STATUS', 'record': {'defense': {'lose': 4, 'win': 1}, 'insurance': 0, 'score': 34, 'shield_date': 0, 'sneak': {'lose': 19, 'win': 1}}, 'result': 'NOERR', 'rqid': 0, 'sneak_mode': -1, 'xuid': None}, 'original_size': 290, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
"""
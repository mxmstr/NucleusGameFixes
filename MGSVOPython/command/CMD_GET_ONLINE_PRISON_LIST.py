from Command import Command

class CMD_GET_ONLINE_PRISON_LIST(Command):

    def execute(self, data):

        data = {'crypto_type': 'COMPOUND', 'flowid': None, 'msgid': 'CMD_GET_ONLINE_PRISON_LIST', 'prison_soldier_param': [], 'rescue_list': [], 'rescue_num': 0, 'result': 'NOERR', 'rqid': 0, 'soldier_num': 0, 'total_num': 0, 'xuid': None}

        return self._receiver.action(data, self.__class__.__name__)

"""
{'compress': False, 'data': {'is_persuade': 1, 'msgid': 'CMD_GET_ONLINE_PRISON_LIST', 'num': 100, 'offset': 0, 'rqid': 0}, 'original_size': 84, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
{'compress': False, 'data': {'crypto_type': 'COMPOUND', 'flowid': None, 'msgid': 'CMD_GET_ONLINE_PRISON_LIST', 'prison_soldier_param': [], 'rescue_list': [], 'rescue_num': 0, 'result': 'NOERR', 'rqid': 0, 'soldier_num': 0, 'total_num': 0, 'xuid': None}, 'original_size': 203, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
"""
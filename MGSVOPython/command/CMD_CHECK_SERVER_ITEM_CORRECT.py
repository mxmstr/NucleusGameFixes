from Command import Command

class CMD_CHECK_SERVER_ITEM_CORRECT(Command):

    def execute(self, data):

        data = {'check_result': 1, 'crypto_type': 'COMPOUND', 'flowid': None, 'msgid': 'CMD_CHECK_SERVER_ITEM_CORRECT', 'result': 'NOERR', 'rqid': 0, 'xuid': None}

        return self._receiver.action(data, self.__class__.__name__)

"""
{'compress': False, 'data': {'item_list': [19372, 3030, 1096, 17032], 'item_list_num': 4, 'msgid': 'CMD_CHECK_SERVER_ITEM_CORRECT', 'rqid': 0}, 'original_size': 104, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
{'compress': False, 'data': {'check_result': 1, 'crypto_type': 'COMPOUND', 'flowid': None, 'msgid': 'CMD_CHECK_SERVER_ITEM_CORRECT', 'result': 'NOERR', 'rqid': 0, 'xuid': None}, 'original_size': 135, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
"""
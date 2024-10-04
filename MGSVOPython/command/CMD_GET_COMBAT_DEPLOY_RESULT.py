from Command import Command

class CMD_GET_COMBAT_DEPLOY_RESULT(Command):

    def execute(self, data):

        data = {'crypto_type': 'COMPOUND', 'flowid': None, 'msgid': 'CMD_GET_COMBAT_DEPLOY_RESULT', 'result': 'NOERR', 'result_list': [], 'result_num': 0, 'rqid': 0, 'xuid': None}

        return self._receiver.action(data, self.__class__.__name__)

"""
{'compress': False, 'data': {'msgid': 'CMD_GET_COMBAT_DEPLOY_RESULT', 'rqid': 0, 'version': 12}, 'original_size': 62, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
{'compress': False, 'data': {'crypto_type': 'COMPOUND', 'flowid': None, 'msgid': 'CMD_GET_COMBAT_DEPLOY_RESULT', 'result': 'NOERR', 'result_list': [], 'result_num': 0, 'rqid': 0, 'xuid': None}, 'original_size': 149, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
"""
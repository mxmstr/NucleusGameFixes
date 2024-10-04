from Command import Command

class CMD_GET_LOGIN_PARAM(Command):

    def execute(self, data):
        return self._receiver.action(data, self.__class__.__name__)

"""
{'compress': False, 'data': {'msgid': 'CMD_GET_LOGIN_PARAM', 'rqid': 0, 'server_item_platform_info': {'platform_base_rank': 0}}, 'original_size': 93, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}

"""
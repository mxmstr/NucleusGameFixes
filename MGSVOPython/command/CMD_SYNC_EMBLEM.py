from Command import Command

class CMD_SYNC_EMBLEM(Command):

    def execute(self, data):
        
        data = {'crypto_type': 'COMPOUND', 'flowid': None, 'msgid': 'CMD_SYNC_EMBLEM', 'result': 'NOERR', 'rqid': 0, 'xuid': None}

        return self._receiver.action(data, self.__class__.__name__)


"""
{'compress': False, 'data': {'emblem': {'parts': [{'base_color': 1842204, 'frame_color': 11842740, 'position_x': 20, 'position_y': 20, 'rotate': 0, 'scale': 10, 'texture_tag': 1241886423}, {'base_color': 1842204, 'frame_color': 11842740, 'position_x': 30, 'position_y': 30, 'rotate': 0, 'scale': 8, 'texture_tag': 1066752497}, {'base_color': 1842204, 'frame_color': 11842740, 'position_x': -92, 'position_y': -80, 'rotate': 0, 'scale': 30, 'texture_tag': 473894566}, {'base_color': 1842204, 'frame_color': 11842740, 'position_x': -64, 'position_y': -80, 'rotate': 0, 'scale': 30, 'texture_tag': 473894566}]}, 'msgid': 'CMD_SYNC_EMBLEM', 'rqid': 0}, 'original_size': 558, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
{'compress': False, 'data': {'crypto_type': 'COMPOUND', 'flowid': None, 'msgid': 'CMD_SYNC_EMBLEM', 'result': 'NOERR', 'rqid': 0, 'xuid': None}, 'original_size': 104, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
"""
from Command import Command

class CMD_SYNC_LOADOUT(Command):

    def execute(self, data):

        data = {'crypto_type': 'COMPOUND', 'flowid': None, 'msgid': 'CMD_SYNC_LOADOUT', 'result': 'NOERR', 'rqid': 0, 'xuid': None}

        return self._receiver.action(data, self.__class__.__name__)

"""
{'compress': False, 'data': {'loadout': {'hand': {'id': 520, 'level_list': [4, 4, 4, 4, 0, 0, 3, 3, 0, 0, 1, 1]}, 'item_level_list': [0, 0, 0, 0, 0, 0, 0, 0], 'item_list': [0, 0, 0, 0, 0, 0, 0, 0], 'primary1': {'chimera_desc': {'color': 0, 'paint': 0, 'parts_list': [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]}, 'chimera_slot': 0, 'id': 0, 'is_chimera': 0}, 'primary2': {'chimera_desc': {'color': 0, 'paint': 0, 'parts_list': [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]}, 'chimera_slot': 0, 'id': 0, 'is_chimera': 0}, 'secondary': {'chimera_desc': {'color': 0, 'paint': 0, 'parts_list': [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]}, 'chimera_slot': 0, 'id': 1066, 'is_chimera': 0}, 'suit': {'camo': 14, 'face': 0, 'level': 7, 'parts': 25}, 'support_list': [0, 0, 0, 0, 0, 0, 0, 0]}, 'msgid': 'CMD_SYNC_LOADOUT', 'rqid': 0}, 'original_size': 642, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
{'compress': False, 'data': {'crypto_type': 'COMPOUND', 'flowid': None, 'msgid': 'CMD_SYNC_LOADOUT', 'result': 'NOERR', 'rqid': 0, 'xuid': None}, 'original_size': 105, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
"""
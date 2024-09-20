from Command import Command
import json

class CMD_SET_MGO_CHARACTER_AND_LOADOUT2(Command):

    def __init__(self, receiver):
        super(CMD_SET_MGO_CHARACTER_AND_LOADOUT2, self).__init__(receiver)
        self._receiver.encrypt = True
        self._receiver.compress = False

    def get_data(self, data):
        
        with open('user_character.json', 'w') as f:
            f.write(json.dumps(data['character']))

        with open('user_loadout.json', 'w') as f:
            f.write(json.dumps(data['loadout']))

        data = {
            'crypto_type': 'COMPOUND',
            'flowid': None,
            'msgid': 'CMD_SET_MGO_CHARACTER_AND_LOADOUT2',
            'result': 'NOERR',
            'rqid': 0,
            'xuid': None
        }
        return data

    def execute(self, data):
        data = self.get_data(data)
        return self._receiver.action(data, self.__class__.__name__)

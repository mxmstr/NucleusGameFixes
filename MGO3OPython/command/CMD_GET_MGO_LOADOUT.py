from Command import Command
import json

class CMD_GET_MGO_LOADOUT(Command):

    def __init__(self, receiver):
        super(CMD_GET_MGO_LOADOUT, self).__init__(receiver)
        self._receiver.encrypt = True
        self._receiver.compress = True

    def get_data(self):

        loadout_json = ''
        
        try:
            with open('user_loadout.json', 'r') as f:
                loadout_json = json.loads(f.read())
        except FileNotFoundError:
            with open('loadout_loadout.json', 'r') as f:
                loadout_json = json.loads(f.read())
                with open('user_loadout.json', 'w') as f:
                    f.write(json.dumps(loadout_json))

        data = {
            "loadout": loadout_json,
            'crypto_type': 'COMPOUND',
            'flowid': None,
            'msgid': 'CMD_GET_MGO_LOADOUT', 
            'result': 'NOERR', 
            'rqid': 0, 
            'xuid': None
        }
        return data

    def execute(self, data):
        data = self.get_data()
        return self._receiver.action(data, self.__class__.__name__)

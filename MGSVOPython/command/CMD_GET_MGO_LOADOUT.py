from Command import Command
import os, json, configparser

config = configparser.ConfigParser()
config.read('MGSVOffline.ini')
my_steam_id = config.get('Settings', 'my_steam_id')
settings_path = os.path.join(os.environ['USERPROFILE'], 'Documents', 'MGSVOffline', my_steam_id)
user_loadout_path = os.path.join(settings_path, 'user_loadout.json')

class CMD_GET_MGO_LOADOUT(Command):

    def __init__(self, receiver):
        super(CMD_GET_MGO_LOADOUT, self).__init__(receiver)
        self._receiver.encrypt = True
        self._receiver.compress = True

    def get_data(self):
        
        loadout_json = {}    

        try:
            with open(user_loadout_path, 'r') as f:
                loadout_json = json.loads(f.read())
        except FileNotFoundError:
            with open('default_loadout.json', 'r') as f:
                loadout_json = json.loads(f.read())
                with open(user_loadout_path, 'w') as f:
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

from Command import Command
import os, json, configparser

config = configparser.ConfigParser()
config.read('MGSVOffline.ini')
my_steam_id = config.get('Settings', 'my_steam_id')
settings_path = os.path.join(os.environ['USERPROFILE'], 'Documents', 'MGSVOffline', my_steam_id)
user_character_path = os.path.join(settings_path, 'user_character.json')
user_loadout_path = os.path.join(settings_path, 'user_loadout.json')

class CMD_DELETE_MGO_CHARACTER(Command):

    def __init__(self, receiver):
        super(CMD_DELETE_MGO_CHARACTER, self).__init__(receiver)
        self._receiver.encrypt = True
        self._receiver.compress = False

    def get_data(self, data):
        
        character_index = data['characterIndex']
        character_json = {}

        with open(user_character_path, 'r') as f:
            character_json = json.loads(f.read())
            del character_json['character_list'][character_index]

        with open(user_character_path, 'w') as f:
            f.write(json.dumps(character_json))
        
        loadout_json = {}
        
        with open(user_loadout_path, 'r') as f:
            loadout_json = json.loads(f.read())
            del loadout_json['character_list'][character_index]
        
        with open(user_loadout_path, 'w') as f:
            f.write(json.dumps(loadout_json))
        
        data = {
            'crypto_type': 'COMPOUND',
            'flowid': None,
            'msgid': 'CMD_DELETE_MGO_CHARACTER',
            'result': 'NOERR',
            'rqid': 0,
            'xuid': None
        }
        return data

    def execute(self, data):
        data = self.get_data(data)
        return self._receiver.action(data, self.__class__.__name__)

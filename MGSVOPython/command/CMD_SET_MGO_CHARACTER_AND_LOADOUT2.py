from Command import Command
import os, json, configparser

config = configparser.ConfigParser()
config.read('MGSVOffline.ini')
my_steam_id = config.get('Settings', 'my_steam_id')
settings_path = os.path.join(os.environ['USERPROFILE'], 'Documents', 'MGSVOffline', my_steam_id)
user_character_path = os.path.join(settings_path, 'user_character.json')
user_loadout_path = os.path.join(settings_path, 'user_loadout.json')

class CMD_SET_MGO_CHARACTER_AND_LOADOUT2(Command):

    def __init__(self, receiver):
        super(CMD_SET_MGO_CHARACTER_AND_LOADOUT2, self).__init__(receiver)
        self._receiver.encrypt = True
        self._receiver.compress = False

    def get_data(self, data):
        
        with open(user_character_path, 'w') as f:
            f.write(json.dumps(data['character']))
        
        with open(user_loadout_path, 'w') as f:
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

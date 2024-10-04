from Command import Command
import os, json, configparser

config = configparser.ConfigParser()
config.read('MGSVOffline.ini')
my_steam_id = config.get('Settings', 'my_steam_id')
settings_path = os.path.join(os.environ['USERPROFILE'], 'Documents', 'MGSVOffline', my_steam_id)
user_character_path = os.path.join(settings_path, 'user_character.json')

class CMD_GET_MGO_CHARACTER2(Command):

    def __init__(self, receiver):
        super(CMD_GET_MGO_CHARACTER2, self).__init__(receiver)
        self._receiver.encrypt = True
        self._receiver.compress = True

    def get_data(self):
        
        character_json = {}

        try:
            with open(user_character_path, 'r') as f:
                character_json = json.loads(f.read())
        except FileNotFoundError:
            with open('default_character.json', 'r') as f:
                character_json = json.loads(f.read())
                with open(user_character_path, 'w') as f:
                    f.write(json.dumps(character_json))

        data = {
            "character": character_json,
            "crypto_type": "COMPOUND",
            "flowid": None,
            "msgid": "CMD_GET_MGO_CHARACTER2",
            "result": "NOERR",
            "rqid": 0,
            "xuid": None,
        }
        return data

    def execute(self, data):
        data = self.get_data()
        return self._receiver.action(data, self.__class__.__name__)

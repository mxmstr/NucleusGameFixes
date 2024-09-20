from Command import Command

class CMD_DELETE_MGO_CHARACTER(Command):

    def __init__(self, receiver):
        super(CMD_DELETE_MGO_CHARACTER, self).__init__(receiver)
        self._receiver.encrypt = True
        self._receiver.compress = False

    def get_data(self, data):
        
        character_index = data['characterIndex']
        character_json = ''

        with open('user_character.json', 'r') as f:
		    character_json = json.loads(f.read())

        del character_json['character_list'][character_index]

        with open('user_character.json', 'w') as f:
            f.write(json.dumps(character_json))

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

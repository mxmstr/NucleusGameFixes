from Command import Command

class CMD_SET_MGO_CHARACTER_AND_LOADOUT2(Command):

    def __init__(self, receiver):
        super(CMD_SET_MGO_CHARACTER_AND_LOADOUT2, self).__init__(receiver)
        self._receiver.encrypt = True
        self._receiver.compress = False
        
        self._database = Database()
        self._database.connect()

    def get_data(self, data):

        session_key = self._receiver.session_key
        player_id = self._database.get_player_by_session_id(session_key)['id']

        self._database.update_player_character_json(player_id, data['character'])
        self._database.update_player_loadout_json(player_id, data['loadout'])

        # for character_index, character in enumerate(data['character']['character_list']):

        #     self._database.update_player_character(player_id, character, character_index)

        #     loadout_list = data['loadout']['character_list'][character_index]['loadout_list']

        #     for loadout_index, loadout in enumerate(loadout_list):
        #         self._database.update_player_character_loadout(character['character_id'], character_index, loadout, loadout_index)


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

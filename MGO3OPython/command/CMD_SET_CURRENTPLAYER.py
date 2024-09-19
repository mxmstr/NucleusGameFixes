from Command import Command

class CMD_SET_CURRENTPLAYER(Command):
    
    def __init__(self, receiver):
        super(CMD_SET_CURRENTPLAYER, self).__init__(receiver)
        self._receiver.encrypt = True
        self._receiver.compress = False
        
    def get_data(self):
        data = {
            'crypto_type': 'COMPOUND', 
            'flag': 0, 
            'flowid': None, 
            'msgid': 'CMD_SET_CURRENTPLAYER', 
            'player_id': 595034, 
            'result': 'NOERR', 
            'rqid': 0, 
            'xuid': None
        }
        return data

    def execute(self, data):
        data = self.get_data()
        return self._receiver.action(data, self.__class__.__name__)

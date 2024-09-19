from Command import Command

class CMD_SET_MGO_CHARACTER2(Command):

    def __init__(self, receiver):
        super(CMD_SET_MGO_CHARACTER2, self).__init__(receiver)
        self._receiver.encrypt = True
        self._receiver.compress = False
        
        self._database = Database()
        self._database.connect()

    def get_data(self):
        data = {
            'crypto_type': 'COMPOUND',
            'flowid': None,
            'msgid': 'CMD_SET_MGO_CHARACTER2',
            'result': 'NOERR',
            'rqid': 0,
            'xuid': None
        }
        return data

    def execute(self, data):
        data = self.get_data()
        return self._receiver.action(data, self.__class__.__name__)

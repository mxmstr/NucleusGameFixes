from Command import Command

class CMD_GET_SVRLIST(Command):
    def __init__(self, receiver):
        super(CMD_GET_SVRLIST, self).__init__(receiver)
        self._receiver.encrypt = False
        self._receiver.compress = False

    def get_svrlist(self):
        data = {
            'crypto_type': 'COMMON', 
            'flowid': None, 
            'msgid': 'CMD_GET_SVRLIST', 
            'result': 'NOERR', 
            'rqid': 0, 
            'server_num': 0, 
            'svrlist': [], 
            'xuid': None
        }
        return data

    def execute(self, data):
        data = self.get_svrlist()
        return self._receiver.action(data, self.__class__.__name__)

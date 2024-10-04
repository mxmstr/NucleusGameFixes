from Command import Command

class CMD_UPDATE_SESSION(Command):
    
    def __init__(self, receiver):
        super(CMD_UPDATE_SESSION, self).__init__(receiver)
        self._receiver.encrypt = True
        self._receiver.compress = False
        
    def get_data(self):
        data = {
            'crypto_type': 'COMPOUND', 
            'flowid': None, 
            'fob_index': -1, 
            'msgid': 'CMD_UPDATE_SESSION', 
            'result': 'NOERR', 
            'rqid': 0, 
            'sneak_mode': -1, 
            'xuid': None
        }
        return data

    def execute(self, data):
        data = self.get_data()
        return self._receiver.action(data, self.__class__.__name__)

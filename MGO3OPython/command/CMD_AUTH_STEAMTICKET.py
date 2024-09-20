from Command import Command
from settings import *
import hashlib
import logging
logger = logging.getLogger(LOGGER_NAME)


class CMD_AUTH_STEAMTICKET(Command):

    def __init__(self, receiver):
        super(CMD_AUTH_STEAMTICKET, self).__init__(receiver)
        self._receiver.encrypt = False
        self._receiver.compress = False

    def get_account(self, data):
        data = {
            'account_id': '00000000000000000', 
            'crypto_type': 'COMMON', 
            'currency': 'USD', 
            'flowid': None, 
            'loginid_password': 'password', 
            'msgid': 'CMD_AUTH_STEAMTICKET', 
            'result': 'NOERR', 
            'rqid': 0, 
            'smart_device_id': 'sMaRtDeViCeId', 
            'xuid': None
        }
        return data

    def execute(self, data):
        data = self.get_account(data)
        return self._receiver.action(data, self.__class__.__name__)


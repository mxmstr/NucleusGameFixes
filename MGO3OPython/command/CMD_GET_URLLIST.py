from Command import Command
import os

class CMD_GET_URLLIST(Command):

    def __init__(self, receiver):
        super(CMD_GET_URLLIST, self).__init__(receiver)
        self._receiver.encrypt = False
        self._receiver.compress = True

    def get_url_list(self):

        base_url = '127.0.0.1'

        data = {
            "crypto_type": "COMMON",
            "flowid": None,
            "msgid": "CMD_GET_URLLIST",
            "result": "NOERR",
            "rqid": 0,
            "url_list": [
                {
                    "type": "GATE",
                    "url": "http://%s/mgostm/gate" % base_url,
                    "version": 0
                },
                {
                    "type": "WEB",
                    "url": "http://%s/mgostm/main" % base_url,
                    "version": 0
                },
                {
                    "type": "HEATMAP",
                    "url": "http://%s/tppstmweb/heatmap" % base_url,
                    "version": 0
                },
                {
                    "type": "DEVICE",
                    "url": "http://%s/tppstm/main" % base_url,
                    "version": 0
                },
                {
                    "type": "EULA",
                    "url": "http://%s/tppstmweb/eula/eula.var" % base_url,
                    "version": 0
                },
                {
                    "type": "EULA_COIN",
                    "url": "http://%s/tppstmweb/coin/coin.var" % base_url,
                    "version": 0
                },
                {
                    "type": "POLICY_GDPR",
                    "url": "http://%s/tppstmweb/gdpr/privacy.var" % base_url,
                    "version": 0
                },
                {
                    "type": "POLICY_JP",
                    "url": "http://%s/tppstmweb/privacy_jp/privacy.var" % base_url,
                    "version": 0
                },
                {
                    "type": "POLICY_ELSE",
                    "url": "http://%s/tppstmweb/privacy/privacy.var" % base_url,
                    "version": 0
                },
                {
                    "type": "LEGAL",
                    "url": "http://%s/games/mgsvtpp/" % base_url,
                    "version": 0
                },
                {
                    "type": "PERMISSION",
                    "url": "http://%s/" % base_url,
                    "version": 0
                },
                {
                    "type": "POLICY_CCPA",
                    "url": "http://%s/tppstmweb/privacy_ccpa/privacy.var" % base_url,
                    "version": 0
                },
                {
                    "type": "EULA_TEXT",
                    "url": "http://%s/games/mgsvtpp/terms/" % base_url,
                    "version": 0
                },
                {
                    "type": "EULA_COIN_TEXT",
                    "url": "http://%s/games/mgsvtpp/terms/currency/" % base_url,
                    "version": 0
                },
                {
                    "type": "POLICY_GDPR_TEXT",
                    "url": "http://%s/games/mgsvtpp/" % base_url,
                    "version": 0
                },
                {
                    "type": "POLICY_JP_TEXT",
                    "url": "http://%s/games/privacy/view/" % base_url,
                    "version": 0
                },
                {
                    "type": "POLICY_ELSE_TEXT",
                    "url": "http://%s/games/privacy/view/" % base_url,
                    "version": 0
                },
                {
                    "type": "POLICY_CCPA_TEXT",
                    "url": "http://%s/games/mgsvtpp/ppa4ca/" % base_url,
                    "version": 0
                }
            ],
            "url_num": 18,
            "xuid": None
        }
        return data

    def execute(self, data):
        data = self.get_url_list()
        return self._receiver.action(data, self.__class__.__name__)
    
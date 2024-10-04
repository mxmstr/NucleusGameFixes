from Command import Command

class CMD_SEND_BOOT(Command):

    def execute(self, data):
        
        data = {'crypto_type': 'COMPOUND', 'flag': 0, 'flowid': None, 'msgid': 'CMD_SEND_BOOT', 'result': 'NOERR', 'rqid': 0, 'xuid': None}

        return self._receiver.action(data, self.__class__.__name__)


"""
{'compress': False, 'data': {'is_goty': 0, 'is_mbcoin_dlc': 0, 'msgid': 'CMD_SEND_BOOT', 'play_time': 263048, 'rqid': 0, 'send_record': {'config': [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 49, 1, 0, 1, 0, 0, 0, 5, 0], 'develop_count': [69, 64, 29, 52, 46, 25, 32, 29, 0, 0, 51, 30, 0, 0, 11, 50, 27, 53, 0, 16, 11, 5, 5, 39, 0, 0, 0, 0, 19, 0, 0, 0, 0, 0, 12, 0], 'score': [203157, 157776, 142072, 173673, 177001, 231637, 216126, 228925, 146497, 176516, 273349, 185753, 204277, 180479, 159068, 164638, 152906, 189815, 145281, 204500, 162949, 188676, 72099, 215507, 191664, 157077, 197056, 198660, 199126, 164038, 195397, 193275, 186621, 181483, 185943, 198149, 168365, 173874, 202161, 0, 245426, 0, 233223, 0, 207927, 269480, 0, 235448, 177304, 0, 0, 0, 198936, 0, 206350, 0, 160222, 0, 259478, 163432, 0, 189151, 0, 0], 'score_limit': [0, 0, 0, 0, 177739, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 87642, 0, 0, 0, 0, 145892, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 171890, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]}}, 'original_size': 839, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
{'compress': False, 'data': {'crypto_type': 'COMPOUND', 'flag': 0, 'flowid': None, 'msgid': 'CMD_SEND_BOOT', 'result': 'NOERR', 'rqid': 0, 'xuid': None}, 'original_size': 111, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
"""
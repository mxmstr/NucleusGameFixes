from Command import Command

class CMD_SEND_SNEAK_RESULT(Command):

    def execute(self, data):

        return self._receiver.action(data, self.__class__.__name__)

"""
{'compress': True, 'data': {'crypto_type': 'COMPOUND', 'event_point': 0, 'flowid': None, 'is_security_challenge': 0, 'is_wormhole_open': 0, 'msgid': 'CMD_SEND_SNEAK_RESULT', 'result': 'NOERR', 'result_reward': {'african_peach': 0, 'biotic': 0, 'black_carrot': 0, 'common': 0, 'digitalis_l': 0, 'digitalis_p': 0, 'fuel': 0, 'gmp': 0, 'golden_crescent': 0, 'haoma': 0, 'is_before': 0, 'key_item': 0, 'main_type': 0, 'minor': 0, 'param_id': 0, 'precious': 0, 'rate': 0, 'section': 0, 'staff_count': 0, 'staff_rank': [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], 'staff_type': 0, 'tarragon': 0, 'wormwood': 0}, 'rqid': 0, 'sneak_point': 0, 'xuid': None}, 'original_size': 641, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
"""
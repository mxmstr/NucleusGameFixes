from Command import Command

class CMD_GET_CHALLENGE_TASK_TARGET_VALUES(Command):

    def execute(self, data):
        
        data = {'crypto_type': 'COMPOUND', 'espionage_rating_grade': 4, 'flowid': None, 'fob_defense_success_count': 1, 'fob_deploy_to_supporters_emergency_count': 0, 'fob_sneak_count': 20, 'fob_sneak_success_count': 1, 'fob_supporting_user_count': 0, 'msgid': 'CMD_GET_CHALLENGE_TASK_TARGET_VALUES', 'pf_rating_defense_force': 367919, 'pf_rating_defense_life': 509929, 'pf_rating_offence_force': 285912, 'pf_rating_offence_life': 610538, 'pf_rating_rank': 28, 'result': 'NOERR', 'rqid': 0, 'total_development_grade': 1895, 'total_fob_security_level': 343, 'xuid': None}

        return self._receiver.action(data, self.__class__.__name__)

"""
{'compress': False, 'data': {'msgid': 'CMD_GET_CHALLENGE_TASK_TARGET_VALUES', 'rqid': 0}, 'original_size': 57, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
{'compress': True, 'data': {'crypto_type': 'COMPOUND', 'espionage_rating_grade': 4, 'flowid': None, 'fob_defense_success_count': 1, 'fob_deploy_to_supporters_emergency_count': 0, 'fob_sneak_count': 20, 'fob_sneak_success_count': 1, 'fob_supporting_user_count': 0, 'msgid': 'CMD_GET_CHALLENGE_TASK_TARGET_VALUES', 'pf_rating_defense_force': 367919, 'pf_rating_defense_life': 509929, 'pf_rating_offence_force': 285912, 'pf_rating_offence_life': 610538, 'pf_rating_rank': 28, 'result': 'NOERR', 'rqid': 0, 'total_development_grade': 1895, 'total_fob_security_level': 343, 'xuid': None}, 'original_size': 518, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
"""
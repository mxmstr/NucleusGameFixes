from Command import Command

class CMD_MGO_MISSION_RESULT(Command):
    
    def __init__(self, receiver):
        super(CMD_MGO_MISSION_RESULT, self).__init__(receiver)
        self._receiver.encrypt = True
        self._receiver.compress = True
        
    def get_data(self):
        data = {
            'character_index': 0, 
            'code': 0, 
            'crypto_type': 'COMPOUND', 
            'current_gp': 999999, 
            'current_xp': 999999, 
            'earned_gp': 0, 
            'earned_xp': 0, 
            'flowid': None, 
            'msgid': 'CMD_MGO_MISSION_RESULT', 
            'rank_param': {
                'current_rank_xp': 999999, 
                'earned_rank_xp': 0, 
                'rank_xp_list': [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
            }, 
            'result': 'NOERR', 
            'rqid': 0, 
            'survival_params': {
                'current_survival_wins': 0, 
                'earned_survival_gp': 0, 
                'reward_category': 'NotImplement', 
                'reward_id_a': 0, 
                'reward_id_b': 0, 
                'reward_id_c': 0, 
                'survival_update_key': 0
            },
            'ucd': '595034-0',
            'xuid': None
        }
        return data

    def execute(self, data):
        data = self.get_data()
        return self._receiver.action(data, self.__class__.__name__)

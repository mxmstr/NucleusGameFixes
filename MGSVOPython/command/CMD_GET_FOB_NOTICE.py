from Command import Command

class CMD_GET_FOB_NOTICE(Command):

    def execute(self, data):

        data = {'active_event_server_text': 'mb_fob_event_name_03', 'campaign_param_list': [], 'common_server_text': 'NotImplement', 'common_server_text_title': 'NotImplement', 'crypto_type': 'COMPOUND', 'daily': 0, 'event_delete_date': 0, 'event_end_date': 1728363600, 'exists_event_point_combat_deploy': 0, 'flag': 1501, 'flowid': None, 'league_update': {'get_point': 59050, 'grade': 28, 'now_rank': 3626, 'point': 9333959, 'prev_grade': 28, 'prev_rank': 4763, 'score': 40512}, 'mb_coin': 1435, 'msgid': 'CMD_GET_FOB_NOTICE', 'pf_current_season': 950, 'pf_finish_num': 13, 'pf_finish_num_max': 120, 'point_exchange_event_server_text': 'NotImplement', 'record': {'defense': {'lose': 4, 'win': 1}, 'insurance': 0, 'score': 0, 'shield_date': 0, 'sneak': {'lose': 0, 'win': 0}}, 'result': 'NOERR', 'rqid': 0, 'short_pf_current_season': 2624, 'short_pf_finish_num': 12, 'short_pf_finish_num_max': 56, 'sneak_update': {'get_point': 0, 'grade': 4, 'now_rank': 609955, 'point': 0, 'prev_grade': 4, 'prev_rank': 609856, 'score': 34}, 'xuid': None}

        return self._receiver.action(data, self.__class__.__name__)

"""
{'compress': False, 'data': {'msgid': 'CMD_GET_FOB_NOTICE', 'rqid': 0}, 'original_size': 39, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
{'compress': True, 'data': {'active_event_server_text': 'mb_fob_event_name_03', 'campaign_param_list': [], 'common_server_text': 'NotImplement', 'common_server_text_title': 'NotImplement', 'crypto_type': 'COMPOUND', 'daily': 0, 'event_delete_date': 0, 'event_end_date': 1728363600, 'exists_event_point_combat_deploy': 0, 'flag': 1501, 'flowid': None, 'league_update': {'get_point': 59050, 'grade': 28, 'now_rank': 3626, 'point': 9333959, 'prev_grade': 28, 'prev_rank': 4763, 'score': 40512}, 'mb_coin': 1435, 'msgid': 'CMD_GET_FOB_NOTICE', 'pf_current_season': 950, 'pf_finish_num': 13, 'pf_finish_num_max': 120, 'point_exchange_event_server_text': 'NotImplement', 'record': {'defense': {'lose': 4, 'win': 1}, 'insurance': 0, 'score': 0, 'shield_date': 0, 'sneak': {'lose': 0, 'win': 0}}, 'result': 'NOERR', 'rqid': 0, 'short_pf_current_season': 2624, 'short_pf_finish_num': 12, 'short_pf_finish_num_max': 56, 'sneak_update': {'get_point': 0, 'grade': 4, 'now_rank': 609955, 'point': 0, 'prev_grade': 4, 'prev_rank': 609856, 'score': 34}, 'xuid': None}, 'original_size': 933, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
"""
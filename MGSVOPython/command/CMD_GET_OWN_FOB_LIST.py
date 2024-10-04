from Command import Command

class CMD_GET_OWN_FOB_LIST(Command):

    def execute(self, data):

        data = {'crypto_type': 'COMPOUND', 'enable_security_challenge': 0, 'flowid': None, 'fob': [{'area_id': 53, 'cluster_param': [], 'construct_param': 41067, 'fob_index': 0, 'mother_base_id': 318711, 'platform_count': 0, 'price': 1000, 'security_rank': 0}, {'area_id': 0, 'cluster_param': [], 'construct_param': 0, 'fob_index': 0, 'mother_base_id': 0, 'platform_count': 0, 'price': 0, 'security_rank': 0}, {'area_id': 0, 'cluster_param': [], 'construct_param': 0, 'fob_index': 0, 'mother_base_id': 0, 'platform_count': 0, 'price': 0, 'security_rank': 0}, {'area_id': 0, 'cluster_param': [], 'construct_param': 0, 'fob_index': 0, 'mother_base_id': 0, 'platform_count': 0, 'price': 0, 'security_rank': 0}], 'msgid': 'CMD_GET_OWN_FOB_LIST', 'result': 'NOERR', 'rqid': 0, 'xuid': None}

        return self._receiver.action(data, self.__class__.__name__)

"""
{'compress': False, 'data': {'msgid': 'CMD_GET_OWN_FOB_LIST', 'rqid': 0}, 'original_size': 41, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
{'compress': True, 'data': {'crypto_type': 'COMPOUND', 'enable_security_challenge': 0, 'flowid': None, 'fob': [{'area_id': 53, 'cluster_param': [], 'construct_param': 41067, 'fob_index': 0, 'mother_base_id': 318711, 'platform_count': 0, 'price': 1000, 'security_rank': 0}, {'area_id': 0, 'cluster_param': [], 'construct_param': 0, 'fob_index': 0, 'mother_base_id': 0, 'platform_count': 0, 'price': 0, 'security_rank': 0}, {'area_id': 0, 'cluster_param': [], 'construct_param': 0, 'fob_index': 0, 'mother_base_id': 0, 'platform_count': 0, 'price': 0, 'security_rank': 0}, {'area_id': 0, 'cluster_param': [], 'construct_param': 0, 'fob_index': 0, 'mother_base_id': 0, 'platform_count': 0, 'price': 0, 'security_rank': 0}], 'msgid': 'CMD_GET_OWN_FOB_LIST', 'result': 'NOERR', 'rqid': 0, 'xuid': None}, 'original_size': 692, 'session_crypto': True, 'session_key': '86a19a7a7ffa42e98d3c0af60be961a5'}
"""
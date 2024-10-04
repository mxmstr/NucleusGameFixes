from Command import Command

class CMD_GET_MGO_PURCHASABLE_GEAR_COLOR(Command):

    def __init__(self, receiver):
        super(CMD_GET_MGO_PURCHASABLE_GEAR_COLOR, self).__init__(receiver)
        self._receiver.encrypt = True
        self._receiver.compress = False

    def get_data(self):
        data = {
            'crypto_type': 'COMPOUND',
            'flowid': None,
            'msgid': 'CMD_GET_MGO_PURCHASABLE_GEAR_COLOR',
            "purchasable_gear_color": {
                "already_released": 1,
                "gear_id": 3347441038,
                "purchasable_color_list": [
                    {
                        "already_purchased": 1,
                        "color": 378514155,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 373432697,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 3020208773,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 76167981,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 12356394,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 3652746413,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 3642541877,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 42204107,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 288703677,
                        "level": 0,
                        "point": 0,
                        "prestige": 0,
                        "purchase_type": 0,
                    },
                    {
                        "already_purchased": 1,
                        "color": 3753018657,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 3202192720,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 596127025,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 3075741992,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 2281905530,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 2358646918,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 1320004083,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 441935374,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 3102859567,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 1334550402,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 877211317,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 75594939,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 2510955615,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 1563954279,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 3609649821,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 2421028971,
                        "level": 0,
                        "point": 50,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 2377407379,
                        "level": 0,
                        "point": 5000,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 274590504,
                        "level": 0,
                        "point": 300,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 3874100232,
                        "level": 0,
                        "point": 300,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 844354489,
                        "level": 0,
                        "point": 300,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 603682560,
                        "level": 0,
                        "point": 1000,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 3358777140,
                        "level": 0,
                        "point": 300,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 1319182002,
                        "level": 0,
                        "point": 300,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 82930585,
                        "level": 0,
                        "point": 300,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 1253000828,
                        "level": 0,
                        "point": 300,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 1517145530,
                        "level": 0,
                        "point": 300,
                        "prestige": 0,
                        "purchase_type": 2,
                    },
                    {
                        "already_purchased": 1,
                        "color": 3308198696,
                        "level": 0,
                        "point": 500,
                        "prestige": 0,
                        "purchase_type": 3,
                    },
                    {
                        "already_purchased": 1,
                        "color": 1602343766,
                        "level": 0,
                        "point": 300,
                        "prestige": 0,
                        "purchase_type": 3,
                    },
                    {
                        "already_purchased": 1,
                        "color": 211193641,
                        "level": 0,
                        "point": 0,
                        "prestige": 0,
                        "purchase_type": 4,
                    },
                    {
                        "already_purchased": 1,
                        "color": 2981525399,
                        "level": 0,
                        "point": 0,
                        "prestige": 0,
                        "purchase_type": 4,
                    },
                    {
                        "already_purchased": 1,
                        "color": 956441261,
                        "level": 0,
                        "point": 0,
                        "prestige": 0,
                        "purchase_type": 4,
                    },
                    {
                        "already_purchased": 1,
                        "color": 1223860679,
                        "level": 0,
                        "point": 0,
                        "prestige": 0,
                        "purchase_type": 4,
                    },
                    {
                        "already_purchased": 1,
                        "color": 2072697952,
                        "level": 0,
                        "point": 0,
                        "prestige": 0,
                        "purchase_type": 4,
                    },
                    {
                        "already_purchased": 1,
                        "color": 1538492005,
                        "level": 0,
                        "point": 0,
                        "prestige": 0,
                        "purchase_type": 4,
                    },
                    {
                        "already_purchased": 1,
                        "color": 75500924,
                        "level": 0,
                        "point": 0,
                        "prestige": 0,
                        "purchase_type": 4,
                    },
                    {
                        "already_purchased": 1,
                        "color": 2775162649,
                        "level": 0,
                        "point": 0,
                        "prestige": 0,
                        "purchase_type": 4,
                    },
                    {
                        "already_purchased": 1,
                        "color": 1129643036,
                        "level": 0,
                        "point": 0,
                        "prestige": 0,
                        "purchase_type": 4,
                    },
                    {
                        "already_purchased": 1,
                        "color": 403463967,
                        "level": 0,
                        "point": 0,
                        "prestige": 0,
                        "purchase_type": 4,
                    },
                    {
                        "already_purchased": 1,
                        "color": 743569537,
                        "level": 0,
                        "point": 0,
                        "prestige": 0,
                        "purchase_type": 4,
                    },
                    {
                        "already_purchased": 1,
                        "color": 1483600195,
                        "level": 0,
                        "point": 0,
                        "prestige": 0,
                        "purchase_type": 4,
                    },
                ],
                "release_date": 0,
            },
            'result': 'NOERR',
            'rqid': 0,
            'xuid': None
        }
        return data

    def execute(self, data):
        data = self.get_data()
        return self._receiver.action(data, self.__class__.__name__)


"""
{'compress': False, 'data': {'gear_id': 878119308, 'msgid': 'CMD_GET_MGO_PURCHASABLE_GEAR_COLOR', 'rqid': 0}, 'original_size': 75, 'session_crypto': True, 'session_key': '11111111111111111111111111111111'}

{'compress': False, 'data': {'gear_id_list': {'gear_id_list': [3347441038, 1044513810, 3478078378, 4273678466, 13258465, 2487588223, 1363005292, 4236819450, 1177875415, 2579147060, 878119308, 2919407593, 1071960682, 775154908, 1041509373, 4126523844, 319804376, 1601475495, 662270850, 3609704195, 2986082315, 3685115142, 2904393478, 4084124012, 2593011832, 1031643753, 4243121754, 3316788240, 1227294830, 2983599233, 3172129675, 1082868864, 391035613, 3875105802, 2211090473, 2294721085, 317452302, 2280419734, 3802731151, 887697884, 3265718520, 1998788085, 2281828283, 3929682215, 1244045621, 2575864910, 3491263666, 3286648596, 2602268177, 1034459920, 3420616329, 2417898025, 343198811]}, 'msgid': 'CMD_GET_MGO_PURCHASABLE_GEAR', 'rqid': 0}, 'original_size': 656, 'session_crypto': True, 'session_key': '3b12e881e25f4a1bb5697a9cf4357532'}
{'compress': True, 'data': {'crypto_type': 'COMPOUND', 'flowid': None, 'msgid': 'CMD_GET_MGO_PURCHASABLE_GEAR', 'purchasable_gear_list': {'purchasable_gear_list': [{'already_purchased': 0, 'already_released': 1, 'default_color': 288703677, 'gear_id': 3347441038, 'point': 0, 'prestige': 0, 'purchase_type': 0}, {'already_purchased': 0, 'already_released': 1, 'default_color': 2421028971, 'gear_id': 1044513810, 'point': 0, 'prestige': 0, 'purchase_type': 0}, {'already_purchased': 0, 'already_released': 1, 'default_color': 373432697, 'gear_id': 3478078378, 'point': 0, 'prestige': 0, 'purchase_type': 0}, {'already_purchased': 0, 'already_released': 1, 'default_color': 3202192720, 'gear_id': 4273678466, 'point': 300, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 373432697, 'gear_id': 13258465, 'point': 300, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 2421028971, 'gear_id': 2487588223, 'point': 3000, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 2421028971, 'gear_id': 1363005292, 'point': 5000, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 352863748, 'gear_id': 4236819450, 'point': 0, 'prestige': 0, 'purchase_type': 0}, {'already_purchased': 0, 'already_released': 1, 'default_color': 75594939, 'gear_id': 1177875415, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 2421028971, 'gear_id': 2579147060, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 1563954279, 'gear_id': 878119308, 'point': 10000, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 373432697, 'gear_id': 2919407593, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 1563954279, 'gear_id': 1071960682, 'point': 3000, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 327502434, 'gear_id': 775154908, 'point': 1000, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 327502434, 'gear_id': 1041509373, 'point': 300, 'prestige': 0, 'purchase_type': 3}, {'already_purchased': 0, 'already_released': 1, 'default_color': 327502434, 'gear_id': 4126523844, 'point': 300, 'prestige': 0, 'purchase_type': 3}, {'already_purchased': 0, 'already_released': 1, 'default_color': 327502434, 'gear_id': 319804376, 'point': 300, 'prestige': 0, 'purchase_type': 3}, {'already_purchased': 0, 'already_released': 1, 'default_color': 327502434, 'gear_id': 1601475495, 'point': 300, 'prestige': 0, 'purchase_type': 3}, {'already_purchased': 0, 'already_released': 1, 'default_color': 327502434, 'gear_id': 662270850, 'point': 300, 'prestige': 0, 'purchase_type': 3}, {'already_purchased': 0, 'already_released': 1, 'default_color': 327502434, 'gear_id': 3609704195, 'point': 300, 'prestige': 0, 'purchase_type': 3}, {'already_purchased': 0, 'already_released': 1, 'default_color': 327502434, 'gear_id': 2986082315, 'point': 3000, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 327502434, 'gear_id': 3685115142, 'point': 3000, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 327502434, 'gear_id': 2904393478, 'point': 1000, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 0, 'default_color': 327502434, 'gear_id': 4084124012, 'point': 0, 'prestige': 0, 'purchase_type': 0}, {'already_purchased': 0, 'already_released': 1, 'default_color': 327502434, 'gear_id': 2593011832, 'point': 300, 'prestige': 0, 'purchase_type': 3}, {'already_purchased': 0, 'already_released': 1, 'default_color': 327502434, 'gear_id': 1031643753, 'point': 300, 'prestige': 0, 'purchase_type': 3}, {'already_purchased': 0, 'already_released': 0, 'default_color': 327502434, 'gear_id': 4243121754, 'point': 1000000000, 'prestige': 0, 'purchase_type': 3}, {'already_purchased': 0, 'already_released': 0, 'default_color': 327502434, 'gear_id': 3316788240, 'point': 1000000000, 'prestige': 0, 'purchase_type': 3}, {'already_purchased': 0, 'already_released': 0, 'default_color': 42204107, 'gear_id': 1227294830, 'point': 5000, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 0, 'default_color': 2510955615, 'gear_id': 2983599233, 'point': 10000, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 3642541877, 'gear_id': 3172129675, 'point': 500, 'prestige': 0, 'purchase_type': 3}, {'already_purchased': 0, 'already_released': 1, 'default_color': 441935374, 'gear_id': 1082868864, 'point': 500, 'prestige': 0, 'purchase_type': 3}, {'already_purchased': 0, 'already_released': 1, 'default_color': 2421028971, 'gear_id': 391035613, 'point': 0, 'prestige': 0, 'purchase_type': 0}, {'already_purchased': 0, 'already_released': 1, 'default_color': 12356394, 'gear_id': 3875105802, 'point': 0, 'prestige': 0, 'purchase_type': 0}, {'already_purchased': 0, 'already_released': 1, 'default_color': 373432697, 'gear_id': 2211090473, 'point': 0, 'prestige': 0, 'purchase_type': 0}, {'already_purchased': 0, 'already_released': 1, 'default_color': 373432697, 'gear_id': 2294721085, 'point': 500, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 373432697, 'gear_id': 317452302, 'point': 500, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 373432697, 'gear_id': 2280419734, 'point': 500, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 2421028971, 'gear_id': 3802731151, 'point': 500, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 3020208773, 'gear_id': 887697884, 'point': 5000, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 2421028971, 'gear_id': 3265718520, 'point': 0, 'prestige': 0, 'purchase_type': 0}, {'already_purchased': 0, 'already_released': 1, 'default_color': 76167981, 'gear_id': 1998788085, 'point': 0, 'prestige': 0, 'purchase_type': 0}, {'already_purchased': 0, 'already_released': 1, 'default_color': 2358646918, 'gear_id': 2281828283, 'point': 1000, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 352863748, 'gear_id': 3929682215, 'point': 0, 'prestige': 0, 'purchase_type': 0}, {'already_purchased': 0, 'already_released': 1, 'default_color': 75594939, 'gear_id': 1244045621, 'point': 300, 'prestige': 0, 'purchase_type': 3}, {'already_purchased': 0, 'already_released': 1, 'default_color': 76167981, 'gear_id': 2575864910, 'point': 500, 'prestige': 0, 'purchase_type': 3}, {'already_purchased': 0, 'already_released': 1, 'default_color': 76167981, 'gear_id': 3491263666, 'point': 5000, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 327502434, 'gear_id': 3286648596, 'point': 500, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 327502434, 'gear_id': 2602268177, 'point': 10000, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 327502434, 'gear_id': 1034459920, 'point': 10000, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 76167981, 'gear_id': 3420616329, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 2421028971, 'gear_id': 2417898025, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'already_released': 1, 'default_color': 2421028971, 'gear_id': 343198811, 'point': 300, 'prestige': 0, 'purchase_type': 2}]}, 'result': 'NOERR', 'rqid': 0, 'xuid': None}, 'original_size': 7328, 'session_crypto': True, 'session_key': '3b12e881e25f4a1bb5697a9cf4357532'}
{'compress': False, 'data': {'gear_id': 3347441038, 'msgid': 'CMD_GET_MGO_PURCHASABLE_GEAR_COLOR', 'rqid': 0}, 'original_size': 76, 'session_crypto': True, 'session_key': '3b12e881e25f4a1bb5697a9cf4357532'}
{'compress': True, 'data': {'crypto_type': 'COMPOUND', 'flowid': None, 'msgid': 'CMD_GET_MGO_PURCHASABLE_GEAR_COLOR', 'purchasable_gear_color': {'already_released': 1, 'gear_id': 3347441038, 'purchasable_color_list': [{'already_purchased': 0, 'color': 378514155, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 373432697, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3020208773, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 76167981, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 12356394, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3652746413, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3642541877, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 42204107, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 288703677, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 0}, {'already_purchased': 0, 'color': 3753018657, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3202192720, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 596127025, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3075741992, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 2281905530, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 2358646918, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 1320004083, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 441935374, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3102859567, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 1334550402, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 877211317, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 75594939, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 2510955615, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 1563954279, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3609649821, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 2421028971, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 2377407379, 'level': 0, 'point': 5000, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 274590504, 'level': 0, 'point': 300, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3874100232, 'level': 0, 'point': 300, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 844354489, 'level': 0, 'point': 300, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 603682560, 'level': 0, 'point': 1000, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3358777140, 'level': 0, 'point': 300, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 1319182002, 'level': 0, 'point': 300, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 82930585, 'level': 0, 'point': 300, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 1253000828, 'level': 0, 'point': 300, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 1517145530, 'level': 0, 'point': 300, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3308198696, 'level': 0, 'point': 500, 'prestige': 0, 'purchase_type': 3}, {'already_purchased': 0, 'color': 1602343766, 'level': 0, 'point': 300, 'prestige': 0, 'purchase_type': 3}, {'already_purchased': 0, 'color': 211193641, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 2981525399, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 956441261, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 1223860679, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 2072697952, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 1538492005, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 75500924, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 2775162649, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 1129643036, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 403463967, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 743569537, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 1483600195, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}], 'release_date': 0}, 'result': 'NOERR', 'rqid': 0, 'xuid': None}, 'original_size': 4867, 'session_crypto': True, 'session_key': '3b12e881e25f4a1bb5697a9cf4357532'}
{'compress': False, 'data': {'gear_id': 3347441038, 'msgid': 'CMD_GET_MGO_PURCHASABLE_GEAR_COLOR', 'rqid': 0}, 'original_size': 76, 'session_crypto': True, 'session_key': '3b12e881e25f4a1bb5697a9cf4357532'}
{'compress': True, 'data': {'crypto_type': 'COMPOUND', 'flowid': None, 'msgid': 'CMD_GET_MGO_PURCHASABLE_GEAR_COLOR', 'purchasable_gear_color': {'already_released': 1, 'gear_id': 3347441038, 'purchasable_color_list': [{'already_purchased': 0, 'color': 378514155, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 373432697, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3020208773, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 76167981, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 12356394, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3652746413, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3642541877, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 42204107, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 288703677, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 0}, {'already_purchased': 0, 'color': 3753018657, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3202192720, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 596127025, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3075741992, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 2281905530, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 2358646918, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 1320004083, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 441935374, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3102859567, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 1334550402, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 877211317, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 75594939, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 2510955615, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 1563954279, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3609649821, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 2421028971, 'level': 0, 'point': 50, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 2377407379, 'level': 0, 'point': 5000, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 274590504, 'level': 0, 'point': 300, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3874100232, 'level': 0, 'point': 300, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 844354489, 'level': 0, 'point': 300, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 603682560, 'level': 0, 'point': 1000, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3358777140, 'level': 0, 'point': 300, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 1319182002, 'level': 0, 'point': 300, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 82930585, 'level': 0, 'point': 300, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 1253000828, 'level': 0, 'point': 300, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 1517145530, 'level': 0, 'point': 300, 'prestige': 0, 'purchase_type': 2}, {'already_purchased': 0, 'color': 3308198696, 'level': 0, 'point': 500, 'prestige': 0, 'purchase_type': 3}, {'already_purchased': 0, 'color': 1602343766, 'level': 0, 'point': 300, 'prestige': 0, 'purchase_type': 3}, {'already_purchased': 0, 'color': 211193641, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 2981525399, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 956441261, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 1223860679, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 2072697952, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 1538492005, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 75500924, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 2775162649, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 1129643036, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 403463967, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 743569537, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}, {'already_purchased': 0, 'color': 1483600195, 'level': 0, 'point': 0, 'prestige': 0, 'purchase_type': 4}], 'release_date': 0}, 'result': 'NOERR', 'rqid': 0, 'xuid': None}, 'original_size': 4867, 'session_crypto': True, 'session_key': '3b12e881e25f4a1bb5697a9cf4357532'}

"""
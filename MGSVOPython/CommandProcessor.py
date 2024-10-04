import os, sys, time, configparser

config = configparser.ConfigParser()
config.read('MGSVOffline.ini')
my_steam_id = config.get('Settings', 'my_steam_id')
settings_path = os.path.join(os.environ['USERPROFILE'], 'Documents', 'MGSVOffline', my_steam_id)

if not os.path.exists(settings_path): os.makedirs(settings_path)

from Invoker import Invoker
from Receiver import Receiver
from Encoder import Encoder
from Decoder import Decoder

encoder = Encoder()
decoder = Decoder()

request = ''
with open('request.txt', 'r') as f:
	request = f.read()

#print('Request: {} \n'.format(request))

# Use the passed request value
decoded_request = decoder.decode(request)

if decoded_request['session_crypto']:

    crypto_key = 'MyCoolCryptoKeyAAAAAAA=='    
    encoder.__init_session_blowfish__(crypto_key)
    decoder.__init_session_blowfish__(crypto_key)
    decoded_request = decoder.decode(request)

#print('Decoded request: {} \n'.format(decoded_request))

command_name = decoded_request['data']['msgid']
command_data = decoded_request['data']

#print('Got a command: {} \n'.format(command_name))

mod = __import__('command.{}'.format(command_name), fromlist=[command_name])
command = getattr(mod, command_name)
receiver = Receiver(decoded_request['session_key'])
my_command = command(receiver)
invoker = Invoker()
invoker.store_command(my_command)
invoker.store_data(command_data)
execution_result = invoker.execute_commands()

#print('Execution result: {} \n'.format(str(execution_result)))

# there is only one command per request 
result = encoder.encode(execution_result[command_name])

#print('Encoded result: {} \n'.format(result))

print(result)

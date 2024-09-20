import os, sys, time

# Get the current working directory
current_directory = os.getcwd()

# Print the current working directory
#print("Current working directory:", current_directory)

from Invoker import Invoker
from Receiver import Receiver
from Encoder import Encoder
from Decoder import Decoder

encoder = Encoder()
decoder = Decoder()

#request = "YnHdLj/1b4RBZXynl0xG1B0domEayp/1lLk99kX4wjo7NblxIkhv1ByeVvdNenjEJTavALlsfZfSgBpLKuCMvHkaMHdNOW9g+4ytGq/cFcXOpW6W3rjoDzBVAFXLVj+HRATx/hb68EX3+00fDqDfc0/wdXEaV+G7h5Zc4M2QoF5juLcqskL1iLDYQlLVsTH5VCgC7mK204ygBrK6BopI6RZN6pX+6R+lfT/01GExQVs=";

# get first command line argument and assign to request
request = sys.argv[1]

#print('Request: {} \n'.format(request))

# log request to a text file
#with open('C:\\Program Files (x86)\\Steam\\steamapps\\common\\MGS_TPP\\request.txt', 'w') as f:
#	f.write('asdf ' + request)
# replace all %2B with + in request
#request = request.replace('%2B', '+')

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
# remove newlines and returns
#result = result.replace('\r\n\r\n', '\r\n')
#result = result.replace('\r\r', '\r')

print(result)
#print('httpMsg=' + result)
#
## sleep for 60 seconds
##time.sleep(60)
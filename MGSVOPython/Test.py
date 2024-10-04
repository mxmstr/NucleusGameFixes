import os, sys, time, json

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

user_character_path = os.path.join(os.environ['USERPROFILE'], 'Documents', 'MGSVOffline', 'user_character.json')

if not os.path.exists(user_character_path):
	os.makedirs(user_character_path)

with open('request.txt', 'r') as f:

	request = f.read()

	# remove line breaks
	request = request.replace('\n', '')
	# remove httpMsg= from request
	request = request.replace('httpMsg=', '')
	
	#print('Request: {} \n'.format(request))
	
	try:
	    decoded_request = decoder.decode(request)
	
	    if decoded_request['session_crypto']:
	
	        crypto_key = 'MyCoolCryptoKeyAAAAAAA=='    
	        encoder.__init_session_blowfish__(crypto_key)
	        decoder.__init_session_blowfish__(crypto_key)
	        decoded_request = decoder.decode(request)
	
	    print('Decoded request: {} \n'.format(decoded_request))
	except:
	    pass#output += 'Error: ' + str(request) + '\n'


## open msgs.xml and iterate each <message/> element
#import xml.etree.ElementTree as ET
#tree = ET.parse('msgs.xml')
#root = tree.getroot()
#
#with open('output.txt', 'w') as f:
#	
#    for message in root.findall('message'):
#
#        request = message.text
#
#        # remove line breaks
#        request = request.replace('\n', '')
#        # remove httpMsg= from request
#        request = request.replace('httpMsg=', '')
#
#        #print('Request: {} \n'.format(request))
#
#        try:
#            decoded_request = decoder.decode(request)
#
#            if decoded_request['session_crypto']:
#        
#                crypto_key = 'Q9Tv79OFHREyBIekAOwmNg=='    
#                encoder.__init_session_blowfish__(crypto_key)
#                decoder.__init_session_blowfish__(crypto_key)
#                decoded_request = decoder.decode(request)
#
#            f.write(str(decoded_request) + '\n')
#            #print('Decoded request: {} \n'.format(decoded_request))
#        except:
#            pass#output += 'Error: ' + str(request) + '\n'


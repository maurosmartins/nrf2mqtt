import json
import time
import queue
import signal
import spidev
import datetime
import threading
import RPi.GPIO as GPIO
from lib_nrf24 import NRF24
import paho.mqtt.client as mqtt

# Global variables
mqtt_broker_address = "192.168.20.120"
mqtt_topic = "nrf/out/#"
nodeID = 0


def signal_handler(signal, frame): 
	print('Exiting\n') 
	global finishprogram
	finishprogram = True
     
# Callback when a new message is received
def on_message(client, userdata, msg):
    payload = msg.payload.decode('utf-8')
    try:
        data = json.loads(payload)
        with queue_lock:
            if(msg.topic == 'nrf/out'):
                data["addr"] = 0xFF
            else:
                data["addr"] = int( (msg.topic.split('/'))[2] )
            message_queue.put(data)
    except json.JSONDecodeError as e:
        print(f"Error decoding JSON: {e}")

def nrfSender():
    broacastMessagesCounter = 0
    directMessagesCounter = 0
    
    while finishprogram == False:
        with queue_lock:
            if not message_queue.empty():
                data = message_queue.get()
                radio.stopListening();
                messageLength = len(data["message"])
                radio.setPayloadSize( messageLength )
                
                if( data["addr"] == 0xFF):                                  #Broadcast Message
                    print(f'Broadcast M.#{broacastMessagesCounter} - {data["message"]}')
                    broacastMessagesCounter+=1
                    TX_ADDR = [0xe7, 0xe7, 0xe7, 0xe7, 0xFF]
                    radio.openWritingPipe(TX_ADDR)
                    #u8TXAddr,u8OriginAddr,u8MsgType,u8PayloadLength, DATA0, DATA1 ... DATA27
                    txbuffer = bytes([0xFF, nodeID, 0, messageLength])
                    txbuffer+= bytes(data["message"], 'UTF-8')
                    radio.vNRFSendPayloadNoAck(txbuffer)
                    
                else:                                                       #Direct Message   
                    #print("Direct")
                    print(f'Direct M.#{directMessagesCounter} - {data["addr"]}: {data["message"]}')
                    directMessagesCounter+=1
                    TXRX_ADDR = [0xe7, 0xe7, 0xe7, 0xe7, data["addr"]]
                    radio.openWritingPipe(TXRX_ADDR)
                    radio.openWritingPipe(TXRX_ADDR) 
                    radio.openReadingPipe(0, TXRX_ADDR)
                    radio.openReadingPipe(0, TXRX_ADDR)

                    radio.print_address_register("TX_ADDR", NRF24.TX_ADDR)
                    radio.print_address_register("RX_ADDR_P0", NRF24.RX_ADDR_P0, 2)     

                    radio.flush_tx()
                    txbuffer = bytes([data["addr"], nodeID, 0, messageLength])
                    txbuffer+= bytes(data["message"], 'UTF-8')
                    radio.setAutoAck(True)
                    radio.vNRFSendPayload(txbuffer)
                    time.sleep(20 / 1000)
                    if radio.bNRFWasAckReceived():
                        print("ACK Received");
                    else:
                        print("ACK Error")
                        
                    radio.startListening()

        time.sleep(100 / 1000)

if __name__ == '__main__':     

    finishprogram = False 
    queue_lock = threading.Lock()
    message_queue = queue.Queue()
    #broadcastQ = queue.Queue() 
    #directQ = queue.Queue() 

    it = 0

    GPIO.setmode(GPIO.BCM)
    GPIO.setwarnings(False)

    print("initializing Radio")
    radio = NRF24(GPIO, spidev.SpiDev())
    #begin(self, csn_pin, ce_pin=0)
    radio.begin(0, 4)

    radio.write_register(NRF24.CONFIG,     0x7A) # MASK_RX_DR MASK_TX_DS MASK_MAX_RT EN_CRC CRCO PWR_UP PRIM_RX (0=TX, 1=RX)
    radio.write_register(NRF24.EN_AA,      0x07) # Reserved ENAA_P5 ENAA_P4 ENAA_P3 ENAA_P2 ENAA_P1 ENAA_P0
    radio.write_register(NRF24.EN_RXADDR,  0x07) # Reserved ERX_P5 ERX_P4 ERX_P3 ERX_P2 ERX_P1 ERX_P0
    radio.write_register(NRF24.SETUP_AW,   0x03) # Reserved AW -> 5bytes
    radio.write_register(NRF24.SETUP_RETR, 0x00) # ARD ARC -> 4000us delay between retries, 15 retries
    radio.write_register(NRF24.RF_CH,      0x01) # RF_CH -> 0x01
    radio.write_register(NRF24.RF_SETUP,   0x26) # CONT_WAVE Reserved RF_DR_LOW PLL_LOCK RF_DR_HIGH RF_PWR Obsolete -> 250kbps, 0dbm
    radio.write_register(NRF24.DYNPD,      0x07) # Reserved DPL_P5 DPL_P4 DPL_P3 DPL_P2 DPL_P1 DPL_P0
    radio.write_register(NRF24.FEATURE,    0x07) # Reserved EN_DPL EN_ACK_PAYd EN_DYN_ACK

    TX_ADDR = [0xe7, 0xe7, 0xe7, 0xe7, 0x01]
    RXADDRP0 = [0xe7, 0xe7, 0xe7, 0xe7, 0x01]

    #TX_ADDR = [0xe7, 0xe7, 0xe7, 0xe7, 0xFF]
    #RXADDRP0 = [0xe7, 0xe7, 0xe7, 0xe7, 0xFF]

    RXADDRP1 = [0xe7, 0xe7, 0xe7, 0xe7, nodeID]
    RXADDRP2 = [0xFF]
#   RXADDRP3 = [0xc4]
#   RXADDRP4 = [0xc5]
#   RXADDRP5 = [0xc6]

    radio.openWritingPipe(TX_ADDR)

    radio.openReadingPipe(0, RXADDRP0)
    radio.openReadingPipe(1, RXADDRP1)
    radio.openReadingPipe(2, RXADDRP2)
    #radio.openReadingPipe(3, RXADDRP3)
    #radio.openReadingPipe(4, RXADDRP4)
    radio.enableAckPayload()

    print("")
    radio.printDetails()
    print("")

    radio.flush_rx()
    radio.startListening()

    print("initializing MQTT Client")
    # MQTT setup
    #client = mqtt.Client()
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
    client.on_message = on_message

    # Connect to the MQTT broker
    client.username_pw_set("username", "password")
    client.connect(mqtt_broker_address, 1883, 60)

    # Subscribe to the MQTT topic
    client.subscribe(mqtt_topic)

    # Start the MQTT loop in a separate thread
    mqtt_thread = threading.Thread(target=client.loop_forever)
    mqtt_thread.daemon = True
    mqtt_thread.start()

    print("initializing Signal")
    signal.signal(signal.SIGINT, signal_handler)

    print("initializing Sender Thread")
    t = threading.Thread(name='RxComm', target=nrfSender)
    t.daemon = True
    #t.setDaemon(True)
    t.start()

    print("Listning.....")
    while finishprogram == False:
        if radio.bNRFIsDataReady():
            now = datetime.datetime.now()
            payload_length = radio.getDynamicPayloadSize()
            print( "Payload Length " + str(payload_length) )
            payload = radio.read_payload(payload_length)
            print(type(payload[0]))
            print(f"Origin {payload[1]}")
            print( "it {} @{} - ".format(it, now.strftime("%Y.%m.%d %H:%M:%S")) + radio.ConvertFromIntListToString(payload) )
            it = it+1
        time.sleep(100 / 1000)

	    
    

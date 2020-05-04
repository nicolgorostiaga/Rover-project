#Author: Nicol Anokhin and Jeniffer Ordonez
#date: 5-04-2020
#Brief: This python script converts the commands from the app to #equivalent letters and stores each letter in a file.

import paho.mqtt.client as mqtt

clientName = "rover"
serverAddress = "Rover.local"
mqttClient = mqtt.Client(clientName)

def connectionStatus(client, userdata, flags, rc):
    mqttClient.subscribe("rover/move")

def messageDecoder(client, userdata, msg):
    message = msg.payload.decode(encoding='UTF-8')
    
    if message == "forward":
       file1 = open("passchar.txt","w")
       file1.seek(0)
       file1.truncate()
       file1.write("w")
       file1.close()
    elif message == "picture":
       file1 = open("passchar.txt","w")
       file1.seek(0)
       file1.truncate()
       file1.write("c")
       file1.close()
    elif message == "backward":
       file1 = open("passchar.txt","w")
       file1.seek(0)
       file1.truncate()
       file1.write("s")
       file1.close()
    elif message == "left":
       file1 = open("passchar.txt","w")
       file1.seek(0)
       file1.truncate()
       file1.write("a")
       file1.close()
    elif message == "right":
       file1 = open("passchar.txt","w")
       file1.seek(0)
       file1.truncate()
       file1.write("d")
       file1.close()
    else:
       print("?!? Unknown message?!?")

# Set up calling functions to mqttClient
mqttClient.on_connect = connectionStatus
mqttClient.on_message = messageDecoder

# Connect to the MQTT server & loop forever.
# CTRL-C will stop the program from running.
mqttClient.connect(serverAddress)
mqttClient.loop_forever()

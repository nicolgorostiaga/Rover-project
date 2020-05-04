#Author: Nicol Anokhin and Jeniffer Ordonez
#date: 5-04-2020
#This python script uses Adafruit read function to read the #sensor values. It stores each sensor value separately and sends #it to the iphone app with the publish function.

import paho.mqtt.client as mqtt
import sys
import Adafruit_DHT
import time

clientName = "rover"
serverAddress = "Rover.local"
mqttClient = mqtt.Client(clientName)
def connectionStatus(client, userdata, flags, rc):
    mqttClient.subscribe("rover/sensor")

def messageDecoder(client, userdata, msg):
    message = msg.payload.decode(encoding='UTF-8')
    #Get values from the sensor
    humidity, temperature = Adafruit_DHT.read_retry(11,4)
    #Send temperature value to app
    mqttClient.publish("rover/temperature", int(temperature))
    #print('Temperature =', int(temperature))
    
    #Send Humidity value to app
    mqttClient.publish("rover/humidity", int(humidity))
    #print("Humidity = ", int(humidity))

# Set up calling functions to mqttClient
mqttClient.on_connect = connectionStatus
mqttClient.on_message = messageDecoder

# Connect to the MQTT server & loop forever.
# CTRL-C will stop the program from running.
mqttClient.connect(serverAddress)
mqttClient.loop_forever()

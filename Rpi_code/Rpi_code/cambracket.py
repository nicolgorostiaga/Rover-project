#Author: Nicol Anokhin and Jeniffer Ordonez
#date: 5-04-2020
#This python script accepts commands from the iphone app and #moves the camera from the center position.
#The quit command is use to reposition the camera bracket at #the origin.

import paho.mqtt.client as mqtt
import curses
import os
import time
import RPi.GPIO as GPIO                                                                           

# map arrow keys to special values
#screen.keypad(True)
clientName = "rover"
serverAddress = "Rover.local"
mqttClient = mqtt.Client(clientName)

GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

pan = 27
tilt = 17

GPIO.setup(pan,GPIO.OUT)
GPIO.setup(tilt,GPIO.OUT)


# print doesn't work with curses, use addstr instead
def setServoAngle(servo, angle):
    pwm = GPIO.PWM(servo, 50)
    pwm.start(8)
    dutyCycle = angle / 18. + 3.
    pwm.ChangeDutyCycle(dutyCycle)
    time.sleep(0.3)
    pwm.stop()

setServoAngle(pan, 90)
setServoAngle(tilt, 90)

def connectionStatus(client, userdata, flags, rc):
    mqttClient.subscribe("rover/bracket")

def messageDecoder(client, userdata, msg):
    message = msg.payload.decode(encoding='UTF-8') 
    angle1 = 90
    angle2 = 90

    if message == "quit":
         setServoAngle(pan, 90)
         setServoAngle(tilt, 90)
         #print("Reset bracket")
    elif message == "rightbrac":
         angle1 -= 15
         setServoAngle(pan,angle1)
         #print("Move bracket right")
    elif message == "leftbrac":
         angle1 += 15
         setServoAngle(pan,angle1)
         #print("Move bracket left")
    elif message == "upbrac":
         angle2 -= 15
         setServoAngle(tilt,angle2)
         #print("Move bracket up")
    elif message == "downbrac":
         angle2 += 15
         setServoAngle(tilt,angle2)
         #print("Move bracket down")
    else:
         GPIO.cleanup()

# Set up calling functions to mqttClient
mqttClient.on_connect = connectionStatus
mqttClient.on_message = messageDecoder

# Connect to the MQTT server & loop forever.
# CTRL-C will stop the program from running.
mqttClient.connect(serverAddress)
mqttClient.loop_forever()

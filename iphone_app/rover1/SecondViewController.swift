//
//  SecondViewController.swift
//  rover1
//
//  Created by Rover on 2/2/20.
//  Copyright © 2020 Rover. All rights reserved.
//

import UIKit
import CocoaMQTT
import YouTubePlayer_Swift

class SecondViewController: UIViewController, UITextFieldDelegate{
    
    
    var stop = "stop"
    var picture = "picture"
    var direction: [Int: String] = [0: "forward",
                                    1: "backward",
                                    2: "left",
                                    3: "right"]

    let mqttClient = CocoaMQTT(clientID: "RoverApp", host: "Rover.local", port: 1883)
    
    
    @IBOutlet weak var videoView: YouTubePlayerView!
    @IBOutlet weak var ConnectionButton: UIButton!
    @IBOutlet weak var DisconnectButton: UIButton!
    @IBOutlet weak var PictureButton: UIButton!
    @IBOutlet weak var StreamingButton: UIButton!
    @IBOutlet weak var StopButton: UIButton!
    @IBOutlet weak var TempData: UILabel!
    @IBOutlet weak var HumidityData: UILabel!
    @IBOutlet weak var DangerData: UILabel!
    
    override func viewDidLoad() {
        super.viewDidLoad()
       
        mqttClient.delegate = self
        
        TempData.text = ""
        HumidityData.text = ""
        DangerData.text = ""
        
        StreamingButton.backgroundColor = UIColor.lightGray
        StreamingButton.layer.cornerRadius = 7
        PictureButton.backgroundColor = UIColor.lightGray
        PictureButton.layer.cornerRadius = 7
        StopButton.backgroundColor = UIColor.lightGray
        StopButton.layer.cornerRadius = 7
        DisconnectButton.backgroundColor = UIColor.lightGray
        DisconnectButton.layer.cornerRadius = 7
        ConnectionButton.backgroundColor = UIColor.lightGray
        ConnectionButton.layer.cornerRadius = 7
        
    }
    
    override func didReceiveMemoryWarning() {
        super.viewDidLoad()
    }
    
    @IBAction func connectionBtnPressed(_ sender: UIButton) {
        print("Connection Establish")
        mqttClient.connect()
    }
    
    @IBAction func disconnectButton(_ sender: UIButton) {
        print("Connecion Disconnect")
        print("Sending message: \("disconnect")")
        mqttClient.publish("rover/move", withString:"disconnect")
    }
    @IBAction func buttonDown(_ sender: UIButton) {
        print("Sending message: \(direction[sender.tag]!)")
        mqttClient.publish("rover/move", withString: direction[sender.tag]!)
    }
    
    @IBAction func buttonUp(_ sender: UIButton) {
        print("Sending message: \(stop)")
        mqttClient.publish("rover/move", withString: stop)
    }
    
    @IBAction func picButtonPressed(_ sender: UIButton) {
        print("Sending message: \(picture)")
        mqttClient.publish("rover/move", withString:picture)
    }
    @IBAction func StreamingButtonPressed(_ sender: UIButton) {
        videoView.playerVars = ["playsinline" : 1 as AnyObject]
        videoView.loadVideoID("wXBzac0uW-8");
        videoView.play()
    }
    
}
extension SecondViewController: CocoaMQTTDelegate{
    func mqtt(_ mqtt: CocoaMQTT, didPublishAck id: UInt16) {
       print("didPublishAsk")
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didSubscribeTopic topics: [String]) {
        print("didSubscribeTopic")
    }
    
    func mqttDidDisconnect(_ mqtt: CocoaMQTT, withError err: Error?) {
        print("didDisconnect")
    }
    
    func mqtt(mqtt: CocoaMQTT, didConnect host: String, port: Int) {
        print ("didconnect")
        
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didConnectAck ack: CocoaMQTTConnAck) {
        print("didconnectAck")
        mqttClient.subscribe("rover/temperature")
        mqttClient.subscribe("rover/humidity")
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didPublishMessage message: CocoaMQTTMessage, id: UInt16) {
        print("didpublishmessage")
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didReceiveMessage message: CocoaMQTTMessage, id: UInt16) {
        print("didReceiveMessage", message.topic)
        print("didReceiveMessage", message.string)
        if (message.topic == "rover/temperature" ){
        if let string = message.string {
            self.TempData.text = string
          }
        }
        if (message.topic == "rover/humidity" ){
        if let string = message.string {
            self.HumidityData.text = string
         }
        }
        
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didUnsubscribeTopic topic: String) {
        print("didsubscribetopic")
    }
        
    func mqttDidPing(_ mqtt: CocoaMQTT) {
        print("didping")
    }
    
    func mqttDidReceivePong(_ mqtt: CocoaMQTT) {
        print("receivepong")
    }
    
    func mqttDidDisconnect(mqtt: CocoaMQTT, withError err: NSError) {
        print("mqttdidDisconnect")
    }
    
}

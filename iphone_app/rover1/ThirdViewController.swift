//
//  ThirdViewController.swift
//  rover1
//
//  Created by Rover on 4/26/20.
//  Copyright © 2020 Rover. All rights reserved.
//

import UIKit
import CocoaMQTT

class ThirdViewController: UIViewController {
    
var upbrac = "upbrac"
var downbrac = "downbrac"
var leftbrac = "leftbrac"
var rightbrac = "rightbrac"
var quit = "quit"

let mqttClient = CocoaMQTT(clientID: "RoverApp", host: "Rover.local", port: 1883)
    
@IBOutlet weak var upButton: UIButton!
@IBOutlet weak var leftButtonBrac: UIButton!
@IBOutlet weak var downButton: UIButton!
@IBOutlet weak var rightButtonBrac: UIButton!
@IBOutlet weak var connectButtonBrac: UIButton!
@IBOutlet weak var quitButtonBrac: UIButton!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
    upButton.backgroundColor = UIColor.lightGray
    upButton.layer.cornerRadius = 7
    leftButtonBrac.backgroundColor = UIColor.lightGray
    leftButtonBrac.layer.cornerRadius = 7
    downButton.backgroundColor = UIColor.lightGray
    downButton.layer.cornerRadius = 7
    rightButtonBrac.backgroundColor = UIColor.lightGray
    rightButtonBrac.layer.cornerRadius = 7
    connectButtonBrac.backgroundColor = UIColor.lightGray
    connectButtonBrac.layer.cornerRadius = 7
    quitButtonBrac.backgroundColor = UIColor.lightGray
    quitButtonBrac.layer.cornerRadius = 7
        // Do any additional setup after loading the view.
    }

@IBAction func connectionBtnPressed(_ sender: UIButton) {
        print("Connecion Establish")
        mqttClient.connect()
    }
    
@IBAction func upButtonPressed(_ sender: UIButton) {
        print("Sending message: \(upbrac)")
        mqttClient.publish("rover/bracket", withString: upbrac)
    }
@IBAction func rightButtonPressed(_ sender: UIButton) {
        print("Sending message: \(rightbrac)")
        mqttClient.publish("rover/bracket", withString: rightbrac)
    }
@IBAction func leftButtonPressed(_ sender: UIButton) {
        print("Sending message: \(leftbrac)")
        mqttClient.publish("rover/bracket", withString: leftbrac)
    }
@IBAction func downButtonPressed(_ sender: UIButton) {
        print("Sending message: \(downbrac)")
        mqttClient.publish("rover/bracket", withString: downbrac)
    }
@IBAction func quitButtonPressed(_ sender: UIButton) {
        print("Sending message: \(quit)")
        mqttClient.publish("rover/bracket", withString: quit)
    }
}

//
//  Utils.swift
//  apollo_config

/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

import Foundation
import UIKit

class Utils {
    
    class func sendNotification(note:String, soundName:String) {
        let notification = UILocalNotification()
        notification.fireDate = NSDate(timeIntervalSinceNow: 1)
        notification.hasAction = false
        notification.alertBody = note
        notification.timeZone = NSTimeZone.defaultTimeZone()
        notification.soundName = soundName
        UIApplication.sharedApplication().scheduleLocalNotification(notification)
    }
    
    class func showAlert(message:String) {
        let alert = UIAlertView()
        alert.title = "Notification"
        alert.message = message
        alert.addButtonWithTitle("OK")
        alert.show()
    }
    
}
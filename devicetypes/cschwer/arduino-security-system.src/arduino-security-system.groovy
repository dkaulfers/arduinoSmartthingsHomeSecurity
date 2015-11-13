/**
 *  Copyright 2015 Charles Schwer
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 *  in compliance with the License. You may obtain a copy of the License at:
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
 *  on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License
 *  for the specific language governing permissions and limitations under the License.
 *
 *	Arduino Security System
 *
 *	Author: cschwer
 *	Date: 2015-10-27
 */
 preferences {
	input("ip", "text", title: "IP Address", description: "ip")
	input("port", "text", title: "Port", description: "port")
	input("mac", "text", title: "MAC Addr", description: "mac")
}

 metadata {
	definition (name: "Arduino Security System", namespace: "cschwer", author: "Charles Schwer") {
		capability "Polling"
		capability "Refresh"
		capability "Sensor"
        	capability "Contact Sensor"
        	capability 	"Motion Sensor"
	}

	// simulator metadata
	simulator {}

	// UI tile definitions
	tiles {
        standardTile("backdoor", "device.backdoor") {
        state "open", label: 'Back ${name}', icon: "st.contact.contact.open", backgroundColor: "#ffa81e"
        		state "closed", label: 'Back ${name}', icon: "st.contact.contact.closed", backgroundColor: "#79b821"
		}
    		standardTile("frontdoor", "device.frontdoor") {
			state "open", label: 'Front ${name}', icon: "st.contact.contact.open", backgroundColor: "#ffa81e"
        		state "closed", label: 'Front ${name}', icon: "st.contact.contact.closed", backgroundColor: "#79b821"
		}
    		standardTile("sidedoor", "device.sidedoor") {
			state "open", label: 'Side ${name}', icon: "st.contact.contact.open", backgroundColor: "#ffa81e"
        		state "closed", label: 'Side ${name}', icon: "st.contact.contact.closed", backgroundColor: "#79b821"
		}
		standardTile("hallmotion", "device.hallmotion") {
			state "active", label:'motion', icon:"st.motion.motion.active", backgroundColor:"#53a7c0"
			state "inactive", label:'no motion', icon:"st.motion.motion.inactive", backgroundColor:"#ffffff"
		}
		standardTile("refresh", "device.backdoor", inactiveLabel: false, decoration: "flat") {
			state "default", label:'', action:"refresh.refresh", icon:"st.secondary.refresh"
		}
		main "backdoor"
		details (["frontdoor", "backdoor", "sidedoor", "hallmotion", "refresh"])
	}
}

// parse events into attributes
def parse(String description) {
	log.debug "Parsing '${description}'"
	def msg = parseLanMessage(description)
	def headerString = msg.header

	if (!headerString) {
		log.debug "headerstring was null for some reason :("
    }

	def result = []
	def bodyString = msg.body
    def value = "";
	if (bodyString) {
        log.debug bodyString
        // default the contact and motion status to closed and inactive by default
        def allContactStatus = "closed"
        def allMotionStatus = "inactive"
        def json = msg.json;
        json?.house?.door?.each { door ->
            value = door?.status == 1 ? "open" : "closed"
            log.debug "${door.name} door status ${value}"
            // if any door is open, set contact to open
            if (value == "open") {
				allContactStatus = "open"
			}
			result << createEvent(name: "${door.name}door", value: value)
        }
        json?.house?.motion?.each { motion ->
			value = motion?.status == 1 ? "active" : "inactive"
			log.debug "${motion.name} motion status ${value}"
			// if any motion sensor is active, set motion to active
			if (value == "active") {
				allMotionStatus = "active"
			}
			result << createEvent(name: "${motion.name}motion", value: value)
		}

		result << createEvent(name: "contact", value: allContactStatus)
		result << createEvent(name: "motion", value: allMotionStatus)
    }
    result
}

private Integer convertHexToInt(hex) {
	Integer.parseInt(hex,16)
}

private getHostAddress() {
    def ip = settings.ip
    def port = settings.port

	log.debug "Using ip: ${ip} and port: ${port} for device: ${device.id}"
    return ip + ":" + port
}

def refresh() {
	if(device.deviceNetworkId!=settings.mac) {
    	log.debug "setting device network id"
    	device.deviceNetworkId = settings.mac;
    }
	log.debug "Executing Arduino 'poll'" 
    poll()
}

def poll() {
	log.debug "Executing 'poll' ${getHostAddress()}"
	new physicalgraph.device.HubAction(
    	method: "GET",
    	path: "/getstatus",
    	headers: [
        	HOST: "${getHostAddress()}"
    	]
	)
}

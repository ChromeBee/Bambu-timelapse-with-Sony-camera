# Bambu-timelapse-with-Sony-camera
Controlling a Sony camera using MQTT feed from Bambu Lab 3D printer

This project was inspired by two youtube videos I saw about two weeks ago.
The first was on the Margizmo channel which showed a smooth timelapse on a Bambu printer but recorded on an external camera. And I wondered, How did he do that?

The second video was on the Modbot channel, where he demonstrated an LED controller for Bambu printers -  made by his friend Dutch developer. This controller uses the MQTT protocol to get status information from the printer to control the LEDs.
Links to both videos in the description below.

This made me wonder if I could use the MQTT data to determine when a layer change happens and control a camera. And the simple answer is yes.

I wrote a simple MQTT client that connects to my home WiFi and then connects and subscribes to the MQTT feed on the Bambu printer. I did also configure my router to ensure the printer gets the same IP address every time. The WiFi  and printer credentials are all hard coded in the sketch and it runs on an ESP8266.

To control the camera I have used by Reinhard Nickels that allows WiFi control of a Sony camera from a secon ESP8266. As I have a Sony RX100 M5 it was too good not to try. here is a link to [Reinhard’s site](https://glaskugelsehen.wordpress.com/2016/01/08/sony-camera-remote-control-mit-esp8266/) 
.
So this is my solution two ESP8266s, one determining when to take the photo and the other controlling the camera. The camera code is mostly what Reinhard had written with the addition of a new interrupt routine to handle the input from the other ESP and the addition of a status LED to show when the ESP is connected to the cameras WiFi.

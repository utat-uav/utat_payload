# How to run the odroid
Comprehensive document about how to run the Odroid C1 and grab images.

## Connect via Ethernet (SSH thru the USB Ethernet Port)

Do not use the native ethernet port!

1. Go to "Network and Sharing Centre", then "Change adapter settings".
2. Double click on your Ethernet adaptor, go to Properties.
3. Double click on "Internet Protocol Version 4 (TCP/IP/IPv4)".
4. Click on the "Alternative Configuration" tab, select the "User Configured" radio button.
5. Enter IP Address: 192.168.1.42 
6. Press TAB, subnet mask should automatically populate with 255.255.255.0
8. Uncheck "Validate Settings".
8. Click OK.
9. Open up PuTTY, connect to odroid@192.168.1.2, port 22, password is: odroid.
10. If you cannot connect, try powering off the Odroid and powering it back on again.
11. If you still cannot connect, disable your wifi and try pinging the IP.

## Running the Camera

1. Navigate to ~/uav/utat_payload
2. Run ./init_teledyne, this initalizes the camera with configuration settings. Open up the file to look at the settings.
3. Make sure the camera is powered on and the ethernet cable is plugged into the native ethernet port, NOT the usb ethernet port.
4. If the script cannot detect the camera, wait a bit and try again.
5. Navigate to ~/ and run ./utxPayload, the camera should now be taking images.
6. If it is not taking pictures, open up ~/uav/utat_payload/options.cfg and make sure "savepicture=1", if using GPS also "usegps=1".
7. Pictures should show up in ~/Pictures, make sure that images are valid (use winSCP to copy the images over to your desktop).
8. GPS locations should be stored in uav_gps.log, make sure to delete all entries EXCEPT for the first line (header) when running again.

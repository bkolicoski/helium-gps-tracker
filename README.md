# Helium based DIY GPS vehicle tracker with RYS8839 and RYLR993 and ESP32
I was recently introduced to the existence of the Helium Network, which is a decentralized LoRaWAN network that runs on blockchain and I was surprised to realize that it already has a good coverage in my local area. 

Since the network was available, I wanted to try it out to see how good it works so I decided to built a GPS tracker for it and test the network by driving around through town and recording the path. 

To make the tracker, I used two modules from Reyax, the RYS8839 GPS module and the RYLE993 LoRa module.

The communication and control of the modules is done through an ESP32 development board and to save on credits, there is a feature that would update the position only if the tracker have moved more than 10 meters at a time. 

The full build video of the project is available on my YouTube channel:
[![Helium based DIY GPS vehicle tracker with RYS8839 and RYLR993 and ESP32](https://img.youtube.com/vi/iBvlPjHpbxE/0.jpg)](https://www.youtube.com/watch?v=iBvlPjHpbxE)

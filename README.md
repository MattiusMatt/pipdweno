# Pipdweno
## The song (because projects without songs... well, they suck!)
    Matt:
    Dave?
    (Knocking: Knock, knock, knock, knock, knock, knock)
    Do you wanna build a PipBoy?
    Come on lets go and play
    I hear the master is a bore
    Come out the door
    It's like you've gone away-
    We used to kill the mutants
    And now you're gone
    I wish you would tell me why!-
    ...
    Do you wanna build a PipBoy?
    ...
    It doesn't have to be a PipBoy.
    
    Dave:
    Go away, Matt
    
    Matt:
    Okay, bye...
    
    (Knocking)
    Do you wanna build a PipBoy?
    Or come and find some bears of fallout 4!
    I think some geekery is overdue
    I've started talking to
    the characters in Fallout 4-
    (Hang in there, Preston!)
    It gets a little lonely
    All these ghouls and raiders,
    
    Just shooting the b@#*%$£s dead-
    (Pow-Pow, Pow-Pow, Pow-Pow, Pow-Pow, Pow-Pow)
    
    (Orchestral, Super Mutants attack)
    
    Matt:
    (Knocking)
    Dave?
    Please, I know you're in there,
    Vault Dwellers are asking where you've been
    They say "have courage", and I'm trying to
    I'm right out here you screw, just let me in
    We only have 10 bottlecaps
    It's just you and me
    What the f#@k are we gonna doooooooo?
    
    Do you wanna build a PipBoy?

## Why?
I was googling around and came across a Raspberry Pi PipBoy! I thourght "How cool is that" and bourght my second Rasberry Pi with the intention of building a PipBoy out of it.
My new Pi arrived, a spangly new AdaFruit TFT shield and an electronics starter kit! Two electronics tutorials and some LEDs later (out of ten tutorials) I decided to install the RasPipBoy python source I'd seen in the article and within an hour I had a PipBoy....
That was way too boring and easy as someone had done all of the work for me!!! Also I'm a bit weird (in case you hadn't noticed) and I didnt like watching Linux boot before the PipBoy code ran!! So I watched some more YouTube (yay for YouTube... the source of all my power) and decided to build an actual PipBoy as authentic as I could!
## Rubbish Video Diary
[Pipdweno Video Diary](https://www.youtube.com/playlist?list=PL0KxT4IdSigvEKm1h8vPCSg0aH92uugvp "Pipdweno!")
## The hardware
[Arduino Uno (replaced with mega)](https://www.arduino.cc/en/Main/ArduinoBoardUno "Arduino Uno")

[Arduino Mega](https://www.arduino.cc/en/Main/ArduinoBoardMega2560 "Arduino Mega")

[AdaFruit TFT](https://www.adafruit.com/products/1480 "TFT")

[AdaFruit GPS](https://www.adafruit.com/products/746 "GPS")

[AdaFruit Sound](https://www.adafruit.com/product/2130 "Sound")

[AdaFruit Speaker](https://www.adafruit.com/products/1890 "Speaker")

[Knob (5 Position Rotary Switch)](http://www.amazon.co.uk/gp/product/B00DUYQJWQ?psc=1&redirect=true&ref_=oh_aui_detailpage_o05_s00 "Rotary Switch")

[Deadpool Badge (nothing to do with the project... but how cool)](http://www.amazon.co.uk/gp/product/B00K3O1VFQ?psc=1&redirect=true&ref_=oh_aui_detailpage_o08_s00 "Badge")

[Green LED Buttons](http://www.amazon.co.uk/gp/product/B0094GWD9W?psc=1&redirect=true&ref_=oh_aui_detailpage_o08_s00 "LED Button")

[Power Switch](http://www.amazon.co.uk/gp/product/B00HGAKDIQ?psc=1&redirect=true&ref_=oh_aui_detailpage_o08_s00 "Power Switch")

[Twisty Pushy Thing (Rotary Encoder)](http://www.amazon.co.uk/gp/product/B011AVHSYS?psc=1&redirect=true&ref_=oh_aui_detailpage_o06_s00 "Rotary Encoder")

[FM Radio](https://www.sparkfun.com/products/11083 "Radio")

WiFi or GSM Module to download Maps (Undecided as yet)
## The Software
ToDo, write stuff about Software...
## Wiring
ToDo, you get the idea...
## The PIP Editor
You may or may not know that Arduino's dont have that much memory (although I have more since upgrading my Uno to a Mega... but anyway), so I decided all my text, images and screen layouts should be loaded from an SD card. Which was handy as the AdaFruit TFT Breakout I'm using comes with a MicroSD reader (got to love AdaFruit!!!). Hmmmm what shall I call the layout files.... errr... .pip files.... How creative of me! Well making layout changes using a HEX addon for notepad++ and then copying the .pip files to a MicroSD to then try out on the Arduino only to find out I was still a few pixels out got old very quickly... Hence the PipEditor was born!

[PipEditor](https://github.com/MattiusMatt/pipeditor "PipEditor!")
##Original Inspiration
This is the project that inspired me in the first place! The page has links to the RasPipBoy Python project and AdaFruit TFT. I've since contacted "Hazy Vagrant" and hope to be working with him to build something that will take all of my electronics components! Although its a big ask!!


[RasPipBoy](https://www.etsy.com/listing/237060437/3d-printed-piboy-3000-mark-iv-raspberry?ref=pr_shop "RasPipBoy")

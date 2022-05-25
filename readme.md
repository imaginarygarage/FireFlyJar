# LED Firefly Jar
Yet another blinky project. Here, we blink 8 independent LEDs in patterns based on the research by McDermott and Buck (1959). Blinking is activated via touch sensor (AT42QT1010). Blinks are then chosen at random and assigned to a random LED with random delays in between. By selecting an appropriately colored and sized LED (see the 573nm 0603 LED from OSRAM: LG Q396-PS-35), and mounting them off the board with magnet wire, a very charming firefly effect can be achieved. Placing this board in a mason jar really drives home the illusion of summer nights catching fireflies. 


## The hardware
It would be simple enough to wire an LED to a PWM pin, in series with a resistor, and call it a day. But what's the fun in that? Additionally, I was looking for a project to utilize the DAC output of the STM32F301K6U6. I ended up using an analog switch (NX3L4051) to multiplex the DAC output to 8 independent voltage-controlled current drivers for the LEDs. Placing a hold up capacitor at each driver allows the DAC output to continuously cycle between the 8 LEDs, providing independent and simultaneous control of each "firefly". 

Is it ghoulish overkill? Sure, but the real question should be, "was it fun and educational", and on that front I've no doubt I've succeeded.

### Simplified schematic:
![\<A simplified schematic\>](/simplifed_schematic.jpg)
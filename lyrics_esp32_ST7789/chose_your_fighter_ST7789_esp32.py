import serial
from time import sleep
import time

ser = serial.Serial('COM3', 115200)
sleep(2)

def send_lyrics():
    lines = [
        ("You can be a lover or a fighter,", 0.062),
        ("Whatever you desire", 0.062),
        ("Life is like a runway", 0.062),
        ("And you're the designer", 0.062),
        ("Wings of a butterfly", 0.062),
        ("Eyes of a tiger", 0.072),
        ("Whatever you want", 0.062),
        ("Baby, choose your fighter", 0.062),

        ("-----------", 0.056),

        ("Volcra.lab", 0.1),
    ]

    delays = [0.4, 0.4, 0.4, 0.5, 0.4, 0.4, 0.4, 0.4,1.5,0.2]

    for i, (line, char_delay) in enumerate(lines):
        # typing effect di PC saja (optional)
        for char in line:
            sleep(char_delay)

        ser.write((line + "\n").encode())
        time.sleep(delays[i])

send_lyrics()

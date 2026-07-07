import serial
from time import sleep
import time

ser = serial.Serial('COM3', 115200) 
# Ganti dengan port yang sesuia dengan ESP 32 anda
sleep(2)

def send_lyrics():
    lines = [
        ("Ophelia...", 0.2),
        ("Keep it one hundred on the land", 0.06),
        ("The sea", 0.06),
        ("The sky", 0.06),
        ("Pledge allegiance to your hands", 0.06),
        ("Your team", 0.06),
        ("Your vibes", 0.06),
        ("Don't care where the hell you've been", 0.05),
        ("Cause now", 0.056),
        ("You're mine", 0.056),
        ("It's 'bout to be the sleepless night", 0.054),
        ("You've been dreaming of", 0.056),
        ("The fate of Ophelia", 0.1),

        ("-----------", 0.056),

        ("Volcra.lab", 0.1),
    ]

    delays = [1,0.4,0.4,0.4,0.3,0.3,0.3,0.2,0.2,0.2,0.2,1.5,0.2,1.5,0.2]

    for i, (line, char_delay) in enumerate(lines):
        # typing effect di PC saja (optional)
        for char in line:
            sleep(char_delay)

        ser.write((line + "\n").encode())
        time.sleep(delays[i])

send_lyrics()

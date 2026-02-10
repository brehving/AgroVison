import paho.mqtt.client as mqtt
import matplotlib.pyplot as plt
from collections import deque
import pandas as pd
from datetime import datetime
import os

# ================== CONFIG (CHANGE ONLY THESE) ==================
BROKER = "0e51fa6f1e4c48a59f07c93383606099.s1.eu.hivemq.cloud"   # eg: a1b2c3d4.s1.eu.hivemq.cloud
PORT = 8883
USERNAME = "Agrovision"                          # your HiveMQ username
PASSWORD = "Vinay@2006"                         # your HiveMQ password

TOPIC = "agrovision/data"
CSV_FILE = "agrovision_data.csv"

# ================== DATA BUFFERS ==================
temp = deque(maxlen=50)
soil = deque(maxlen=50)
csi  = deque(maxlen=50)
time_labels = deque(maxlen=50)

# ================== CREATE CSV IF NOT EXISTS ==================
if not os.path.exists(CSV_FILE):
    df = pd.DataFrame(columns=["Time", "Temperature", "Humidity", "Soil", "CSI"])
    df.to_csv(CSV_FILE, index=False)

# ================== MQTT CALLBACK ==================
def on_message(client, userdata, msg):
    t, h, s, c = map(float, msg.payload.decode().split(","))

    now_time = datetime.now().strftime("%H:%M:%S")

    temp.append(t)
    soil.append(s)
    csi.append(c)
    time_labels.append(now_time)

    try:
        row = pd.DataFrame([[datetime.now(), t, h, s, c]],
                           columns=["Time", "Temperature", "Humidity", "Soil", "CSI"])
        row.to_csv(CSV_FILE, mode="a", header=False, index=False)
    except PermissionError:
        print("⚠ CSV is open – close it to resume saving")

# ================== MQTT SETUP ==================
client = mqtt.Client()
client.username_pw_set(USERNAME, PASSWORD)
client.tls_set()
client.connect(BROKER, PORT)
client.subscribe(TOPIC)
client.on_message = on_message
client.loop_start()

# ================== LIVE GRAPHS ==================
plt.ion()

while True:
    # ---- GRAPH 1: SENSOR OVERVIEW ----
    plt.figure(1)
    plt.clf()
    plt.plot(temp, label="Temperature (°C)")
    plt.plot(soil, label="Soil Moisture (%)")
    plt.plot(csi, label="CSI")
    plt.legend()
    plt.title("AgroVision – Live Sensor Overview")
    plt.xlabel("Samples")
    plt.ylabel("Values")

    # ---- GRAPH 2: CSI vs TIME ----
    plt.figure(2)
    plt.clf()
    plt.plot(time_labels, csi, marker="o")
    plt.xticks(rotation=45)
    plt.title("CSI vs Time")
    plt.xlabel("Time")
    plt.ylabel("CSI")
    plt.tight_layout()

    plt.pause(0.5)

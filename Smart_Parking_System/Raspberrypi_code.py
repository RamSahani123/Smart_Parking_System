from flask import Flask, request, jsonify
from grove.display.jhd1802 import JHD1802
import threading

app = Flask(__name__)

# Initialize the LCD
lcd = JHD1802()

# Function to display text on the LCD
def display_text(line1, line2=""):
    lcd.setCursor(0, 0)  # Line 1
    lcd.write(line1)
    lcd.setCursor(1, 0)  # Line 2
    lcd.write(line2)

# Clear the LCD at startup
lcd.clear()

# Function to map slot values to display text
def map_slot_status(value):
    return "Occ" if value == 1 else "Avl" if value == 0 else "Err"

@app.route('/data', methods=['POST'])
def receive_data():
    try:
        # Parse JSON data from the request
        data = request.json
        slot1 = map_slot_status(data.get('slot1', -1))  # Default to "Err" if missing
        slot2 = map_slot_status(data.get('slot2', -1))
        slot3 = map_slot_status(data.get('slot3', -1))

        # Display the data on the LCD
        line1 = f"S1:{slot1} S2:{slot2}"
        line2 = f"S3:{slot3}"
        threading.Thread(target=display_text, args=(line1, line2)).start()

        # Log the received data to the console
        print(f"Received Data - S1: {slot1}, S2: {slot2}, S3: {slot3}")

        # Send a response back to the client
        return jsonify({"status": "success", "message": "Data received"}), 200
    except Exception as e:
        print(f"Error: {e}")
        return jsonify({"status": "error", "message": str(e)}), 400

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)

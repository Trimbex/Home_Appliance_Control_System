import tkinter as tk
import serial
import time
import threading

# Initialize serial communication
ser = serial.Serial('COM7', 9600, timeout=1)

def toggle_lamp():
    """Send the toggle command to the embedded system."""
    def send_command():
        ser.write(b'T')  # Send 'T' to toggle the LED
        time.sleep(0.1)  # Wait for the embedded system to process

    # Run the serial command in a separate thread
    threading.Thread(target=send_command).start()

# Create the GUI
root = tk.Tk()
root.title("Lamp Toggle Controller")

toggle_button = tk.Button(root, text="Toggle Lamp", command=toggle_lamp, font=("Arial", 20))
toggle_button.pack(pady=50)

# Run the GUI event loop
root.mainloop()

# Close the serial port when the GUI is closed
ser.close()

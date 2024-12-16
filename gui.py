import tkinter as tk
import serial
import threading
import time
from tkinter import PhotoImage

# Initialize serial communication
ser = serial.Serial('COM7', 9600, timeout=1)

def toggle_lamp():
    """Send the toggle command to the embedded system."""
    ser.write(b'T')  # Send 'T' to toggle the LED

def toggle_plug():
    """Send the toggle command to the embedded system."""
    ser.write(b'P')  # Send 'P' to toggle the plug

def receive_uart():
    """Continuously read from UART and update the GUI."""
    while True:
        if ser.in_waiting > 0:
            message = ser.readline().decode('utf-8').strip()
            display_message(message)

def display_message(message):
    """Update the text widget with a new message."""
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")  # Get the current timestamp
    text_widget.insert(tk.END, f"[{timestamp}] {message}\n")
    text_widget.see(tk.END)  # Auto-scroll to the latest message

# Create the GUI
root = tk.Tk()
root.title("Toggle Controller")
root.geometry("600x500")  # Set the window size

# Create a frame for organizing the widgets
frame = tk.Frame(root, padx=20, pady=20)
frame.pack(expand=True, fill="both")

# Placeholder for images (can be changed to actual images later)
image_label = tk.Label(frame, text="Image Placeholder", width=20, height=10, relief="solid", anchor="center")
image_label.grid(row=0, column=0, rowspan=3, padx=10, pady=10)

# Create a text widget for displaying messages with timestamp
text_widget = tk.Text(frame, height=10, width=40, font=("Arial", 12), wrap="word")
text_widget.grid(row=0, column=1, rowspan=3, padx=10, pady=10)

# Create a label for the title
label = tk.Label(frame, text="Toggle Controller", font=("Arial", 18, "bold"))
label.grid(row=0, column=2, padx=10, pady=10)

# Toggle Lamp Button
toggle_lamp_button = tk.Button(frame, text="Toggle Lamp", command=toggle_lamp, font=("Arial", 14), bg="#4CAF50", fg="white", relief="raised", height=2, width=15)
toggle_lamp_button.grid(row=1, column=2, padx=10, pady=10)

# Toggle Plug Button
toggle_plug_button = tk.Button(frame, text="Toggle Plug", command=toggle_plug, font=("Arial", 14), bg="#FF5733", fg="white", relief="raised", height=2, width=15)
toggle_plug_button.grid(row=2, column=2, padx=10, pady=10)

# Start a thread for receiving UART messages
threading.Thread(target=receive_uart, daemon=True).start()

# Run the GUI event loop
root.mainloop()

# Close the serial port when the GUI is closed
ser.close()

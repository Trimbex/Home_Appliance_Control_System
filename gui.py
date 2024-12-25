import tkinter as tk
from tkinter import ttk
import serial
import threading
import time

# Initialize serial communication
ser = serial.Serial('COM6', 9600, timeout=1)

# Global variable to track the lamp state
lamp_state = "OFF"  # Default state

def toggle_lamp():
    """Send the toggle command to the embedded system."""
    global lamp_state
    ser.write(b'T')  # Send 'T' to toggle the LED
    
    # Toggle the state
    if lamp_state == "OFF":
        lamp_state = "ON"
    else:
        lamp_state = "OFF"
    
    # Update the lamp state label
    lamp_state_label.config(text=f"Lamp State: {lamp_state}")

def toggle_plug():
    """Send the toggle command to the embedded system."""
    ser.write(b'P')  # Send 'P' to toggle the plug

def receive_uart():
    """Continuously read from UART and update the table with door states and temperature."""
    while True:
        if ser.in_waiting > 0:
            message = ser.readline().decode('utf-8').strip()
            
            # Handle temperature messages
            if message.startswith("TEMP:"):
                try:
                    temperature = float(message.split(":")[1].strip("°C"))
                    update_thermometer(temperature)
                except (IndexError, ValueError):
                    print(f"Invalid temperature message: {message}")
            # Handle door state messages
            elif message in ["OPEN", "CLOSED"]:
                update_door_state(message)
            # Handle lamp state messages (assuming the embedded system sends it)
            elif message in ["LAMP_ON", "LAMP_OFF"]:
                update_lamp_state(message)

def update_door_state(door_state):
    """Update the table with the door state and timestamp."""
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")  # Get the current timestamp

    # Insert the new data into the treeview
    treeview.insert('', 'end', values=(timestamp, door_state))

    # Color the row based on the door state (green for open, red for closed)
    if door_state == "OPEN":
        treeview.tag_configure("open", background="green", foreground="white")
        treeview.item(treeview.get_children()[-1], tags=("open",))
    elif door_state == "CLOSED":
        treeview.tag_configure("closed", background="red", foreground="white")
        treeview.item(treeview.get_children()[-1], tags=("closed",))

def update_lamp_state(message):
    """Update the lamp state label based on the received message."""
    global lamp_state
    if message == "LAMP_ON":
        lamp_state = "ON"
    elif message == "LAMP_OFF":
        lamp_state = "OFF"
    lamp_state_label.config(text=f"Lamp State: {lamp_state}")

def update_thermometer(temperature):
    """Update the thermometer bar based on the current temperature."""
    # Calculate the height of the thermometer based on the temperature (0 to 100)
    height = max(0, min(100, temperature))  # Limit the value between 0 and 100
    thermometer_canvas.coords(thermometer_fill, 10, 150 - height, 40, 150)
    thermometer_label.config(text=f"{temperature:.1f}°C")
    
    # Display a warning if the temperature exceeds limit
    tmp_max = 28.0
    if temperature >= tmp_max:
        warning_label.config(text=f"Warning: Temperature exceeds {tmp_max}°C!")
    else:
        warning_label.config(text="")

# Create the GUI
root = tk.Tk()
root.title("Toggle Controller")
root.geometry("600x500")

# Add a label for the warning in the GUI
warning_label = tk.Label(root, text="", font=("Arial", 14), fg="red")
warning_label.pack(pady=10)

# Create a frame for organizing the widgets
frame = tk.Frame(root, padx=20, pady=20)
frame.pack(expand=True, fill="both")

# Create a Treeview widget for displaying the table with door states and timestamps
columns = ('Timestamp', 'Door State')
treeview = ttk.Treeview(frame, columns=columns, show='headings', height=10)

# Define column headings
treeview.heading('Timestamp', text='Timestamp')
treeview.heading('Door State', text='Door State')

# Set column widths
treeview.column('Timestamp', width=180)
treeview.column('Door State', width=100)

# Add scrollbar
scrollbar = ttk.Scrollbar(frame, orient="vertical", command=treeview.yview)
scrollbar.grid(row=0, column=2, sticky='ns')
treeview.configure(yscrollcommand=scrollbar.set)

# Grid layout for Treeview
treeview.grid(row=0, column=0, columnspan=2, padx=10, pady=10)

# Create a label for the title
label = tk.Label(frame, text="Toggle Controller", font=("Arial", 18, "bold"))
label.grid(row=1, column=0, columnspan=2, padx=10, pady=10)

# Toggle Lamp Button
toggle_lamp_button = tk.Button(frame, text="Toggle Lamp", command=toggle_lamp, font=("Arial", 14), bg="#4CAF50", fg="white", relief="raised", height=2, width=15)
toggle_lamp_button.grid(row=2, column=0, padx=10, pady=10)

# Toggle Plug Button
toggle_plug_button = tk.Button(frame, text="Toggle Plug", command=toggle_plug, font=("Arial", 14), bg="#FF5733", fg="white", relief="raised", height=2, width=15)
toggle_plug_button.grid(row=2, column=1, padx=10, pady=10)

# Lamp State Label
lamp_state_label = tk.Label(frame, text=f"Lamp State: {lamp_state}", font=("Arial", 14))
lamp_state_label.grid(row=3, column=0, columnspan=2, padx=10, pady=10)

# Create the thermometer display
thermometer_frame = tk.Frame(root, padx=20, pady=20)
thermometer_frame.pack(side="right", padx=20)

thermometer_label = tk.Label(thermometer_frame, text="Temperature", font=("Arial", 16))
thermometer_label.pack()

thermometer_canvas = tk.Canvas(thermometer_frame, width=50, height=200, bg="lightgray")
thermometer_canvas.pack()

# Draw the thermometer outline
thermometer_canvas.create_rectangle(10, 10, 40, 150, outline="black", width=2)

# Create the filled part of the thermometer (starts empty)
thermometer_fill = thermometer_canvas.create_rectangle(10, 150, 40, 150, fill="red")

# Start a thread for receiving UART messages
threading.Thread(target=receive_uart, daemon=True).start()

# Run the GUI event loop
root.mainloop()

# Close the serial port when the GUI is closed
ser.close()

import socket
import struct
import csv
import os
import numpy as np
import glove_data_pb2 # This should be generated from glove_data.proto

UDP_IP = "0.0.0.0"
UDP_PORT = 8888

def start_collector():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    
    print(f"Listening on {UDP_IP}:{UDP_PORT}")
    
    label = input("Enter gesture label (e.g., HELLO, A, REST): ").upper()
    csv_file = f"dataset_{label}.csv"
    
    buffer = []
    window_size = 30
    
    with open(csv_file, mode='a', newline='') as f:
        writer = csv.writer(f)
        
        try:
            while True:
                data, addr = sock.recvfrom(1024)
                glove_msg = glove_data_pb2.GloveData()
                glove_msg.ParseFromString(data)
                
                # Combine features: 15 Hall + 6 IMU
                features = list(glove_msg.hall_features) + list(glove_msg.imu_features)
                writer.writerow([glove_msg.timestamp] + features)
                
                buffer.append(features)
                if len(buffer) > window_size:
                    buffer.pop(0)
                    
                if len(buffer) == window_size:
                    # Save sliding window as npy for training
                    # np.save(f"windows/{label}_{glove_msg.timestamp}.npy", np.array(buffer))
                    pass
                    
        except KeyboardInterrupt:
            print("\nStopping collector...")
        finally:
            sock.close()

if __name__ == "__main__":
    if not os.path.exists("windows"):
        os.makedirs("windows")
    start_collector()

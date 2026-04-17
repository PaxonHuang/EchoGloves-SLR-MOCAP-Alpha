import torch
import torch.nn as nn
import glove_data_pb2
import socket
import edge_tts
import asyncio

class PseudoSkeletonMapper(nn.Module):
    def __init__(self):
        super(PseudoSkeletonMapper, self).__init__()
        # Eq 16: S = W_map * x_sensor + b_map
        # Map 21 features to 42 skeleton features (21 landmarks * 2D)
        self.mapping = nn.Linear(21, 42)

    def forward(self, x):
        return self.mapping(x)

class STGCNModel(nn.Module):
    def __init__(self, num_classes=46):
        super(STGCNModel, self).__init__()
        # Simplified ST-GCN wrapper
        self.fc = nn.Linear(42 * 30, num_classes)

    def forward(self, x):
        # x shape: (Batch, Time=30, Landmarks=21, Coords=2)
        x = x.view(x.size(0), -1)
        return self.fc(x)

def nlp_grammar_correction(words):
    # Table V: NLP Grammar Correction Rules
    # R1: SOV -> SVO (I apple eat -> I eat apple)
    # This is a simplified rule-based implementation
    if len(words) >= 3:
        # Assume [Subject, Object, Verb]
        s, o, v = words[0], words[1], words[2]
        return f"{s} {v} {o}"
    return " ".join(words)

async def speak_text(text):
    communicate = edge_tts.Communicate(text, "en-US-GuyNeural")
    await communicate.save("output.mp3")
    # os.system("mpg123 output.mp3")

async def main_l2_loop():
    UDP_IP = "0.0.0.0"
    UDP_PORT = 8888
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    
    mapper = PseudoSkeletonMapper()
    stgcn = STGCNModel()
    
    current_sentence = []
    
    print("L2 Inference & NLP Pipeline started...")
    
    while True:
        data, addr = sock.recvfrom(1024)
        msg = glove_data_pb2.GloveData()
        msg.ParseFromString(data)
        
        gesture_id = msg.l1_gesture_id
        
        if gesture_id == 0:
            # Trigger L2 Inference
            features = torch.tensor(list(msg.hall_features) + list(msg.imu_features)).float()
            skeleton = mapper(features)
            # Assume we have a window of skeletons
            # output = stgcn(skeleton_window)
            # gesture_id = torch.argmax(output)
            pass
            
        if gesture_id > 0:
            word = f"WORD_{gesture_id}" # Map ID to word
            current_sentence.append(word)
            
            if len(current_sentence) >= 3:
                corrected = nlp_grammar_correction(current_sentence)
                print(f"Translated: {corrected}")
                await speak_text(corrected)
                current_sentence = []

if __name__ == "__main__":
    asyncio.run(main_l2_loop())

with open("./model/model.tflite", "rb") as f:
      data = f.read()

with open("./gesture_inference/model_data.h", "w") as f:
      f.write("const unsigned char model_tflite[] = {\n  ")
      f.write(", ".join(f"0x{b:02x}" for b in data))
      f.write("\n};\n")
      f.write(f"const unsigned int model_tflite_len = {len(data)};\n")

print(f"完成，模型大小: {len(data)} bytes")
import os
import sys

# 1. 动态获取当前 python 脚本所在的绝对目录
script_dir = os.path.dirname(os.path.abspath(__file__))

# 2. 拼接出图片和头文件的绝对路径（确保脚本和图片在同一个文件夹下）
image_file = os.path.join(script_dir, "筒体靶标球布局示意图.png")
output_file = os.path.join(script_dir, "LogoImage1.h")

if not os.path.exists(image_file):
    print(f"错误：找不到文件\n路径: {image_file}\n请检查图片是否放在了和本脚本同一个文件夹下，且名字完全对应！")
    sys.exit(1)

with open(image_file, "rb") as f:
    data = f.read()

# 3. 生成头文件
with open(output_file, "w", encoding="utf-8") as f:
    f.write("#pragma once\n\n")
    f.write("// 自动生成的说明图片 C++ 数组\n")
    
    # 注意变量名为 manual_png_data
    f.write("const unsigned char manual_png_data[] = {\n")
    
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        line = "    " + ", ".join([f"0x{b:02X}" for b in chunk])
        if i + 16 < len(data):
            f.write(line + ",\n")
        else:
            f.write(line + "\n")
            
    f.write("};\n")
    
    # 注意变量名为 manual_png_size
    f.write(f"const unsigned int manual_png_size = {len(data)};\n")

print(f"成功转换了 {len(data)} 个字节！")
print(f"文件已生成至: {output_file}")
import os

root = r"D:\project\plcpl\ConsoleApplication1"

def convert_file(path):
    with open(path, "rb") as f:
        raw = f.read()

    text = None

    # 先按 GBK 试
    try:
        text = raw.decode("gbk")
    except:
        # 再按 UTF-8（容错）
        text = raw.decode("utf-8", errors="replace")

    # 写回为 UTF-8（无 BOM）
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(text)

for dirpath, _, filenames in os.walk(root):
    for name in filenames:
        if name.lower().endswith((".h", ".cpp")):
            full = os.path.join(dirpath, name)
            convert_file(full)
            print("converted:", full)
print("done")

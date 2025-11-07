import os, json, numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
from matplotlib import animation
import re

# ---------------- Bone Parser ----------------
def load_bone_file(path):
    animation_data = {
        "header": {},
        "frames": []
    }
    
    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        lines = [line.strip() for line in f if line.strip()]
    
    # 1. 헤더 파싱
    idx = 0
    while idx < len(lines):
        line = lines[idx]
        if line == "---":
            idx += 1
            break
        if ":" in line:
            key, value = line.split(":", 1)
            key, value = key.strip(), value.strip()
            try:
                value = float(value) if "." in value or value.isdigit() else value
            except:
                pass
            animation_data["header"][key] = value
        idx += 1
    
    # 2. 프레임별 파싱
    frame_pattern = re.compile(r"Frame\[(\d+)\]")
    bone_pattern = re.compile(r"Bone\[(\d+)\]:\s*(.+)")
    
    while idx < len(lines):
        line = lines[idx]
        frame_match = frame_pattern.match(line)
        if frame_match:
            frame_idx = int(frame_match.group(1))
            frame_data = {"index": frame_idx, "bones": {}}
            idx += 1
            while idx < len(lines) and not frame_pattern.match(lines[idx]):
                bone_match = bone_pattern.match(lines[idx])
                if bone_match:
                    bone_id = int(bone_match.group(1))
                    bone_name = bone_match.group(2)
                    idx += 1
                    # 4줄의 4x4 행렬 읽기
                    matrix = []
                    for _ in range(4):
                        if idx < len(lines):
                            row = list(map(float, lines[idx].split()))
                            matrix.append(row)
                            idx += 1
                    frame_data["bones"][bone_id] = {
                        "name": bone_name,
                        "matrix": np.array(matrix, dtype=np.float32)
                    }
                else:
                    idx += 1
            animation_data["frames"].append(frame_data)
        else:
            idx += 1
    
    return animation_data


# ---------------- Cylinder Mesh ----------------
def create_cylinder_mesh(center, direction, radius, height, segments=12):
    dir_vec = np.array(direction, dtype=np.float32)
    norm = np.linalg.norm(dir_vec)
    if norm < 1e-6: 
        dir_vec = np.array([0,1,0],dtype=np.float32)
    else: 
        dir_vec /= norm
    # perpendicular basis
    if abs(dir_vec[0]) < 0.9:
        tmp = np.array([1,0,0])
    else:
        tmp = np.array([0,1,0])
    v = np.cross(dir_vec, tmp); v /= np.linalg.norm(v)
    u = np.cross(dir_vec, v)

    angles = np.linspace(0, 2*np.pi, segments, endpoint=False)
    circle = [radius*np.cos(a)*u + radius*np.sin(a)*v for a in angles]

    c = np.array(center)
    top_center = c + dir_vec*(height/2)
    bot_center = c - dir_vec*(height/2)
    top = [top_center + p for p in circle]
    bot = [bot_center + p for p in circle]

    faces = []
    for i in range(segments):
        j = (i+1)%segments
        faces.append([bot[i], bot[j], top[j]])
        faces.append([bot[i], top[j], top[i]])
    return np.array(faces)

def transform_point(M, p):
    v = np.array([p[0], p[1], p[2], 1.0])
    return (M @ v)[:3]

# ---------------- Main ----------------
def main(json_path, bone_path, out_gif="knight_cylinders_anim.gif"):
    # Load cylinder definitions
    with open(json_path,"r",encoding="utf-8") as f:
        cyl_data = json.load(f)

    cylinders = []
    for mesh_name, bones in cyl_data["Meshes"].items():
        for bone_id, params in bones.items():
            cylinders.append({
                "bone": int(bone_id),
                "offset": np.array(params["Offset"],dtype=np.float32),
                "radius": float(params["Radius"]),
                "height": float(params["Height"]),
                "direction": np.array(params["Direction"],dtype=np.float32)
            })

    # Load bone animation
    anim_data = load_bone_file(bone_path)
    frames = anim_data["frames"]
    n_frames = len(frames)

    # Rendering
    def render_frame(ax, frame_idx):
        ax.clear()
        bones = frames[frame_idx]["bones"]
        for cyl in cylinders:
            bone_id = cyl["bone"]
            if bone_id not in bones: 
                continue
            M = bones[bone_id]["matrix"]
            offset = cyl["offset"]; dir_vec = cyl["direction"]
            base = transform_point(M, offset)
            dir_end = transform_point(M, offset + dir_vec)
            new_dir = dir_end - base
            faces = create_cylinder_mesh(base, new_dir, cyl["radius"], cyl["height"], segments=12)
            poly = Poly3DCollection(faces, alpha=0.3, facecolor="blue")
            ax.add_collection3d(poly)
        ax.set_xlim(-100,100); ax.set_ylim(0,200); ax.set_zlim(-100,100)
        ax.set_xlabel("X"); ax.set_ylabel("Y"); ax.set_zlabel("Z")
        ax.view_init(20,40)
        ax.set_title(f"Frame {frame_idx}")

    # Preview frame 0
    fig = plt.figure(figsize=(6,6))
    ax = fig.add_subplot(111, projection='3d')
    render_frame(ax,0)
    plt.show()

    # Make gif
    fig2 = plt.figure(figsize=(6,6))
    ax2 = fig2.add_subplot(111, projection='3d')
    def anim_func(i):
        render_frame(ax2,i)
        return []
    anim = animation.FuncAnimation(fig2, anim_func, frames=n_frames, interval=80, blit=False)
    anim.save(out_gif, writer="pillow", fps=12)
    plt.close(fig2)
    print("Saved animation to", out_gif)

if __name__ == "__main__":
    # 파일 경로 수정해서 사용하세요
    
    cylinder_path = r"C:\Users\Hadenpel\Desktop\GameServer\Animation Parser\knight_cylinders.json"
    animation_path = r"C:\Users\Hadenpel\Desktop\GameServer\Animation Parser\knight5_Walk_mixamo.com_baked.bone"
    
    main(cylinder_path, animation_path)
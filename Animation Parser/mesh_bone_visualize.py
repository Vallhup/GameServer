import os, re, numpy as np, matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
from matplotlib import animation

# ---------------- Mesh Parser ----------------
def parse_mesh_text(text):
    vert_pattern = re.compile(
        r"Vertex\[(\d+)\]\s*"
        r"Position:\s*([-\d\.eE]+)\s+([-\d\.eE]+)\s+([-\d\.eE]+)\s*"
        r"Normal:\s*([-\d\.eE]+)\s+([-\d\.eE]+)\s+([-\d\.eE]+)\s*"
        r"UV:\s*([-\d\.eE]+)\s+([-\d\.eE]+)\s*"
        r"Tangent:\s*([-\d\.eE]+)\s+([-\d\.eE]+)\s+([-\d\.eE]+)\s*"
        r"BoneIndices:\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s*"
        r"BoneWeights:\s*([-\d\.eE]+)\s+([-\d\.eE]+)\s+([-\d\.eE]+)\s+([-\d\.eE]+)",
        re.MULTILINE
    )
    verts = []
    for m in vert_pattern.finditer(text.replace("\r","")):
        pos = np.array([float(m.group(2)), float(m.group(3)), float(m.group(4)), 1.0], dtype=np.float32)
        nrm = np.array([float(m.group(5)), float(m.group(6)), float(m.group(7))], dtype=np.float32)
        uv  = np.array([float(m.group(8)), float(m.group(9))], dtype=np.float32)
        tan = np.array([float(m.group(10)), float(m.group(11)), float(m.group(12))], dtype=np.float32)
        bones = [int(m.group(13)), int(m.group(14)), int(m.group(15)), int(m.group(16))]
        weights = np.array([float(m.group(17)), float(m.group(18)), float(m.group(19)), float(m.group(20))], dtype=np.float32)
        verts.append({"pos": pos, "nrm": nrm, "uv": uv, "tan": tan, "bones": bones, "weights": weights})
    idx_block_start = text.find("[INDICES]")
    indices = []
    if idx_block_start != -1:
        tri_pattern = re.compile(r"Triangle\[\d+\]:\s+(\d+)\s+(\d+)\s+(\d+)")
        for a,b,c in tri_pattern.findall(text[idx_block_start:]):
            indices.extend([int(a), int(b), int(c)])
    return verts, np.array(indices, dtype=np.int32)

def load_mesh(path):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        text = f.read()
    return parse_mesh_text(text)

# ---------------- Bone Parser (대체) ----------------
def load_bone_file(path):
    frames = []
    with open(path, "r", encoding="utf-8") as f:
        lines = [line.rstrip() for line in f]
    
    i = 0
    while i < len(lines):
        if lines[i].startswith("Frame["):
            frame_bones = {}
            i += 1
            
            while i < len(lines) and not lines[i].startswith("Frame["):
                if lines[i].strip().startswith("Bone["):
                    bone_match = re.match(r'\s*Bone\[(\d+)\]:', lines[i])
                    if bone_match:
                        bone_id = int(bone_match.group(1))
                        i += 1
                        
                        matrix = []
                        for _ in range(4):
                            if i < len(lines) and lines[i].strip():
                                row = [float(x) for x in lines[i].split()]
                                matrix.append(row)
                                i += 1
                            else:
                                i += 1
                        
                        if len(matrix) == 4:
                            mat = np.array(matrix, dtype=np.float32)
                            # ★ 전치 (DirectX 행 우선 → NumPy 열 우선)
                            frame_bones[bone_id] = mat.T
                else:
                    i += 1
            
            frames.append(frame_bones)
        else:
            i += 1
    
    return frames

# ---------------- Skinning ----------------
def skin_vertices(verts, bones_dict):
    out = np.zeros((len(verts), 3), dtype=np.float32)
    for vi, v in enumerate(verts):
        P = v["pos"]
        bones = v["bones"]
        weights = v["weights"]
        wsum = float(weights.sum()) if float(weights.sum()) > 0 else 1.0
        w = weights / wsum
        skinned = np.zeros(4, dtype=np.float32)
        for j in range(4):
            b = bones[j]
            if b not in bones_dict or w[j] == 0:
                continue
            skinned += w[j] * (bones_dict[b] @ P)
        
        # ★ 변환 제거 (베이킹 파일이 이미 최종 좌표계)
        out[vi] = skinned[:3]
    
    return out

# ---------------- Rendering ----------------
def build_mesh_faces(indices):
    return np.array(indices, dtype=np.int32).reshape(-1,3)

def render_frame(ax, verts_xyz, faces):
    tris = []
    for a,b,c in faces:
        tris.append([verts_xyz[a], verts_xyz[b], verts_xyz[c]])
    poly = Poly3DCollection(tris, linewidths=0.3, edgecolors=None, facecolors=None, alpha=0.4)
    ax.add_collection3d(poly)
    ax.scatter(verts_xyz[:,0], verts_xyz[:,1], verts_xyz[:,2], s=4)
    mins = verts_xyz.min(axis=0)
    maxs = verts_xyz.max(axis=0)
    center = (mins+maxs)/2
    span = max(1.0, float((maxs-mins).max()))
    ax.set_xlim(center[0]-span*0.6, center[0]+span*0.6)
    ax.set_ylim(center[1]-span*0.6, center[1]+span*0.6)
    ax.set_zlim(center[2]-span*0.6, center[2]+span*0.6)
    ax.set_xlabel("X"); ax.set_ylabel("Y"); ax.set_zlabel("Z")
    ax.view_init(elev=20, azim=40)

# ---------------- Main ----------------
def main(asset_dir):
    mesh_files = sorted([os.path.join(asset_dir,f) for f in os.listdir(asset_dir) if f.endswith(".mesh")])
    all_verts = []; all_indices = []; offset = 0
    for mf in mesh_files:
        vs, idx = load_mesh(mf)
        all_verts.extend(vs)
        all_indices.append(idx + offset)
        offset += len(vs)
    faces = build_mesh_faces(np.concatenate(all_indices))

    bone_frames = load_bone_file(os.path.join(asset_dir,"knight5_Walk_mixamo.com_baked.bone"))
    n_frames = len(bone_frames)

    print("\n=== Frame 0, Bone 53 ===")
    print(bone_frames[0][53])
    print("\n=== Frame 1, Bone 53 ===")
    print(bone_frames[1][53])

    # Preview
    fig = plt.figure(figsize=(6,6)); ax = fig.add_subplot(111, projection='3d')
    verts0 = skin_vertices(all_verts, bone_frames[4])
    render_frame(ax, verts0, faces)
    plt.title("Frame 1 (skinned)"); plt.show()

    # GIF
    fig2 = plt.figure(figsize=(6,6)); ax2 = fig2.add_subplot(111, projection='3d')
    def animate_func(i):
        ax2.clear()
        verts = skin_vertices(all_verts, bone_frames[i])
        render_frame(ax2, verts, faces)
        ax2.set_title(f"Frame {i}")
        return []
    anim = animation.FuncAnimation(fig2, animate_func, frames=n_frames, interval=80, blit=False)
    out_path = os.path.join(asset_dir, "preview.gif")
    anim.save(out_path, writer='pillow', fps=12)
    plt.close(fig2)
    print("Saved to:", out_path)

if __name__ == "__main__":
    main(os.path.dirname(__file__) or ".")
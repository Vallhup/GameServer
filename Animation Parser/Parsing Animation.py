import numpy as np
import json
from math import sqrt
import matplotlib.pyplot as plt

def swap_yz(v): 
    return np.array([v[0], v[2], v[1]], dtype=np.float32)

AXIS_SWAP = np.array([
    [1, 0, 0],
    [0, 1, 0],
    [0, 0, 1],
], dtype=np.float32)

def rotation_matrix_to_quaternion(R):
    m00, m01, m02 = R[0]
    m10, m11, m12 = R[1]
    m20, m21, m22 = R[2]

    trace = m00 + m11 + m22
    if trace > 0:
        s = 0.5 / sqrt(trace + 1.0)
        w = 0.25 / s
        x = (m21 - m12) * s
        y = (m02 - m20) * s
        z = (m10 - m01) * s
    elif m00 > m11 and m00 > m22:
        s = 2.0 * sqrt(1.0 + m00 - m11 - m22)
        w = (m21 - m12) / s
        x = 0.25 * s
        y = (m01 + m10) / s
        z = (m02 + m20) / s
    elif m11 > m22:
        s = 2.0 * sqrt(1.0 + m11 - m00 - m22)
        w = (m02 - m20) / s
        x = (m01 + m10) / s
        y = 0.25 * s
        z = (m12 + m21) / s
    else:
        s = 2.0 * sqrt(1.0 + m22 - m00 - m11)
        w = (m10 - m01) / s
        x = (m02 + m20) / s
        y = (m12 + m21) / s
        z = 0.25 * s

    return np.array([x, y, z, w], dtype=np.float32)


def quat_mul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return np.array([
        aw*bx + ax*bw + ay*bz - az*by,
        aw*by - ax*bz + ay*bw + az*bx,
        aw*bz + ax*by - ay*bx + az*bw,
        aw*bw - ax*bx - ay*by - az*bz
    ], dtype=np.float32)


def quat_apply(q, v):
    qvec = np.array([v[0], v[1], v[2], 0], dtype=np.float32)
    qc = np.array([-q[0], -q[1], -q[2], q[3]], dtype=np.float32)
    return quat_mul(quat_mul(q, qvec), qc)[:3]


# ======================================================
# Bone File Parser (With Matrix Transpose)
# ======================================================
def parse_bone_file(path):
    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        lines = [l.strip() for l in f.readlines()]

    i = 0
    assert "BAKED_ANIMATION" in lines[i]
    i += 1

    anim_name = lines[i].split(":")[1].strip(); i+=1
    bone_count = int(lines[i].split(":")[1].strip()); i+=1
    frame_count = int(lines[i].split(":")[1].strip()); i+=1
    duration = float(lines[i].split(":")[1].strip()); i+=1
    fps = float(lines[i].split(":")[1].strip()); i+=1

    while lines[i] != "---": i+=1
    i+=1

    frames = []

    def skip_blank(idx):
        while idx < len(lines) and lines[idx] == "":
            idx += 1
        return idx

    for _ in range(frame_count):
        i = skip_blank(i)
        assert "Frame[" in lines[i]; i+=1

        bone_matrices = []
        for _ in range(bone_count):
            i = skip_blank(i)
            assert "Bone[" in lines[i]; i+=1

            r0 = list(map(float, lines[i].split())); i+=1
            r1 = list(map(float, lines[i].split())); i+=1
            r2 = list(map(float, lines[i].split())); i+=1
            r3 = list(map(float, lines[i].split())); i+=1

            M = np.array([r0, r1, r2, r3], dtype=np.float32)

            # ★ DirectX row-major matrix → NumPy math matrix (col-major)
            # 이 단계를 mesh 시각화 코드와 동일하게 맞춘다.
            M = M.T

            bone_matrices.append(M)

        frames.append(bone_matrices)

    return {
        "fps": fps,
        "boneCount": bone_count,
        "frameCount": frame_count,
        "frames": frames
    }


# ======================================================
# Capsule Loader
# ======================================================
def load_capsules(path):
    with open(path, "r") as f:
        j = json.load(f)

    out = {}
    for meshName, nodes in j["Meshes"].items():
        for k, v in nodes.items():
            idx = int(k)
            out[idx] = {
                "radius": v["radius"],
                "halfHeight": v["halfHeight"],
                "localOffset": np.array(v["center"], dtype=np.float32),
                "localDir": np.array(v["direction"], dtype=np.float32),
            }
    return out


# ======================================================
# Prebake p0/p1 Using Correct Transform Rules
# ======================================================    
def prebake(anim, capsules, weapon_bones=None,
            weapon_roles="hit",
            default_roles="hurt"):

    weapon_bones = set(weapon_bones or [])
    bone_indices = sorted(capsules.keys())  # 순서 고정

    # 정적 메타: bone/radius/roles 1회 저장
    capsule_defs = []
    for boneIndex in bone_indices:
        cap = capsules[boneIndex]
        roles = weapon_roles if boneIndex in weapon_bones else default_roles
        capsule_defs.append({
            "bone": boneIndex,
            "radius": cap["radius"],
            "roles": roles
        })

    # 동적 포즈: p0/p1만 저장 (capsules와 같은 인덱스 순서)
    out_frames = []
    for bones in anim["frames"]:
        frame_list = []
        for boneIndex in bone_indices:
            cap = capsules[boneIndex]
            M = bones[boneIndex]  # transpose 적용 행렬

            pos = AXIS_SWAP @ M[:3, 3]
            R   = AXIS_SWAP @ M[:3, :3]

            rot_offset = R @ cap["localOffset"]
            rot_dir    = R @ cap["localDir"]
            n = np.linalg.norm(rot_dir)
            if n > 0:
                rot_dir /= n

            centerWorld = pos + rot_offset
            hh = cap["halfHeight"]
            p0 = centerWorld + rot_dir * hh
            p1 = centerWorld - rot_dir * hh

            frame_list.append({
                "p0": p0.tolist(),
                "p1": p1.tolist()
            })

        out_frames.append(frame_list)

    return {
        "version": 2,
        "fps": anim["fps"],
        "numFrames": anim["frameCount"],
        "capsules": capsule_defs,
        "frames": out_frames
    }


# # # ----------------------------------------------------
# # # 설정
# # # ----------------------------------------------------
# JSON_PATH = r"C:\Users\Hadenpel\Desktop\GameServer\Animation Parser\Output\Animation\knight_animation_attack.json"
# FRAME_INDEX = 1  # 보고 싶은 프레임 인덱스

# # # ----------------------------------------------------
# # # 데이터 로드
# # # ----------------------------------------------------
# with open(JSON_PATH, "r", encoding="utf-8") as f:
#     data = json.load(f)

# capsules = data["capsules"]       # 정적 메타
# frame = data["frames"][FRAME_INDEX]  # 동적 포즈 (p0/p1만)

# fig = plt.figure()
# ax = fig.add_subplot(111, projection="3d")

# xs, ys, zs = [], [], []

# for i, pose in enumerate(frame):
#     meta = capsules[i]
#     p0 = np.array(pose["p0"], dtype=np.float32)
#     p1 = np.array(pose["p1"], dtype=np.float32)

#     bone_idx = meta["bone"]
#     radius = meta["radius"]
#     roles = meta["roles"]

#     ax.plot([p0[0], p1[0]], [p0[1], p1[1]], [p0[2], p1[2]])
#     ax.scatter(p0[0], p0[1], p0[2])
#     ax.scatter(p1[0], p1[1], p1[2])

#     xs.extend([p0[0], p1[0]])
#     ys.extend([p0[1], p1[1]])
#     zs.extend([p0[2], p1[2]])

# ax.set_title(f"Frame {FRAME_INDEX} Capsule Colliders (V2)")
# ax.set_xlabel("X")
# ax.set_ylabel("Y")
# ax.set_zlabel("Z")

# if xs and ys and zs:
#     xmid = (min(xs) + max(xs)) * 0.5
#     ymid = (min(ys) + max(ys)) * 0.5
#     zmid = (min(zs) + max(zs)) * 0.5
#     max_range = max(max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs)) * 0.5

#     ax.set_xlim(xmid - max_range, xmid + max_range)
#     ax.set_ylim(ymid - max_range, ymid + max_range)
#     ax.set_zlim(zmid - max_range, zmid + max_range)

# plt.show()



anim = parse_bone_file(r"C:\Users\Hadenpel\Desktop\GameServer\Animation Parser\Final_Boss\Animation\boss_animation_walk_baked.bone")
colliders = load_capsules(r"C:\Users\Hadenpel\Desktop\GameServer\Animation Parser\Output\Capsule\final_boss_capsules.json")
output = r"C:\Users\Hadenpel\Desktop\GameServer\Animation Parser\Output\Animation\final_boss_capsule_walk.json"

weapon_bone_list = [45]
prebaked = prebake(anim, colliders, weapon_bones=weapon_bone_list)

with open(output, "w") as f:
    json.dump(prebaked, f, indent=2)

print("Prebaked collider animation 생성 완료!")

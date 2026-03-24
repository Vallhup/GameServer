import os
import re
import json
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from collections import defaultdict

def compute_pca_axis(points):
    # 정점 Position 집합
    P = np.asarray(points)
    
    # 중심점
    mu = P.mean(axis = 0)
    
    # 중심화 벡터
    Q = P - mu
    
    # 공분산 행렬
    C = np.cov(Q, rowvar = False)
    
    # 고유분해
    eigvals, eigvecs = np.linalg.eigh(C)
    
    # 가장 큰 고유값의 고유벡터 (축 벡터)
    axis = eigvecs[:, np.argmax(eigvals)]
    axis /= np.linalg.norm(axis)
    
    return mu, axis

def compute_endpoints(points, mu, axis):
    P = np.asarray(points)
    
    # 스칼라 투영
    t = (P - mu) @ axis
    
    c1 = mu + t.min() * axis
    c2 = mu + t.max() * axis

    return c1, c2

def point_segment_distance(p, c1, c2):
    u = c2 - c1
    t = np.dot(p - c1, u) / (np.dot(u, u) + 1e-12)
    t_clamped = np.clip(t, 0.0, 1.0)
    closest = c1 + t_clamped * u
    return np.linalg.norm(p - closest)

def compute_radius(points, c1, c2):
    distances = np.array([point_segment_distance(p, c1, c2) for p in points])
    return np.quantile(distances, 1.0)

def CapsuleExtractor(points):
    mu, axis = compute_pca_axis(points)
    c1, c2 = compute_endpoints(points, mu, axis)
    radius = compute_radius(points, c1, c2)
    
    return axis, radius, c1, c2
    
def parse_mesh(path):
    vertices = []
    with open(path, "r") as f:
        v = {}
        for line in f:
            # 공백 제거
            line = line.strip()
            if line.startswith("Position:"):
                v["pos"] = list(map(float, re.findall(r"[-+]?(?:\d*\.\d+|\d+)(?:[eE][-+]?\d+)?", line)))
            
            elif line.startswith("BoneIndices:"):
                v["bone_idx"] = list(map(int, re.findall(r"\d+", line)))
                
            elif line.startswith("BoneWeights:"):
                v["bone_wt"] = list(map(float, re.findall(r"[-+]?(?:\d*\.\d+|\d+)(?:[eE][-+]?\d+)?", line)))
                vertices.append(v)
                v = {}
                
    return vertices

def group_by_bone(vertices):
    bones = defaultdict(list)
    for v in vertices:
        bone = v["bone_idx"][np.argmax(v["bone_wt"])]
        bones[bone].append(v["pos"])
    return bones

def save_capsule_to_json(object_name, mesh_folder, output_path):
    result = {
        "ObjectName": object_name,
        "Meshes": {}
    }

    for file in sorted(os.listdir(mesh_folder)):
        if not file.endswith(".mesh"):
            continue

        mesh_name = os.path.splitext(file)[0]
        print(f"[처리 중] {mesh_name} ...")

        vertices = parse_mesh(os.path.join(mesh_folder, file))
        bone_groups = group_by_bone(vertices)

        mesh_capsules = {}

        for bone_id, points in bone_groups.items():
            if len(points) < 5:
                continue

            axis, radius, c1, c2 = CapsuleExtractor(points)
            center = (c1 + c2) / 2.0
            direction = (c2 - c1)
            direction /= np.linalg.norm(direction)

            height = np.linalg.norm(c2 - c1)
            half_height = height * 0.5

            mesh_capsules[str(bone_id)] = {
                "radius": float(radius),
                "halfHeight": float(half_height),
                "center": center.tolist(),
                "direction": direction.tolist(),
            }

        # Meshes[mesh_name] = { boneIndex → capsule }
        result["Meshes"][mesh_name] = mesh_capsules

    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(result, f, indent=4)

    print(f"\n {output_path} 저장 완료")

if __name__ == "__main__":
    save_capsule_to_json(
        object_name="Imp",
        mesh_folder=r"C:\Users\Hadenpel\Desktop\GameServer\Animation Parser\Imp\Mesh",
        output_path=r"C:\Users\Hadenpel\Desktop\GameServer\Animation Parser\Output\Capsule\imp_capsules.json"
    )

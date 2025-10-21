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
            center = (c1 + c2) / 2
            direction = (c2 - c1)
            direction /= np.linalg.norm(direction)
            height = np.linalg.norm(c2 - c1)
            
            mesh_capsules[str(bone_id)] = {
                "Offset": center.tolist(),
                "Radius": float(radius),
                "Height": float(height),
                "Direction": direction.tolist()
            }
            
        result["Meshes"][mesh_name] = mesh_capsules
    
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(result, f, indent=4)
    print(f"\n {output_path} 저장 완료")

def draw_cylinder_surface(ax, c1, c2, radius, color="skyblue", n_steps=24):
    """c1, c2, radius로 실린더를 3D로 그림 (구머리 없음)"""
    import numpy as np

    u = c2 - c1
    height = np.linalg.norm(u)
    if height < 1e-6:
        return
    u /= height

    # 임의의 보조 축 계산 (u와 수직)
    v = np.array([1, 0, 0])
    if np.allclose(np.cross(u, v), 0):
        v = np.array([0, 1, 0])
    w = np.cross(u, v)
    v = np.cross(w, u)
    v /= np.linalg.norm(v)
    w /= np.linalg.norm(w)

    # 원통 생성
    theta = np.linspace(0, 2*np.pi, n_steps)
    z = np.linspace(0, height, n_steps)
    theta_grid, z_grid = np.meshgrid(theta, z)

    X = radius * np.cos(theta_grid)
    Y = radius * np.sin(theta_grid)
    Z = z_grid

    # 로컬 → 월드 변환
    Xw = c1[0] + X*v[0] + Y*w[0] + Z*u[0]
    Yw = c1[1] + X*v[1] + Y*w[1] + Z*u[1]
    Zw = c1[2] + X*v[2] + Y*w[2] + Z*u[2]

    ax.plot_surface(Xw, Yw, Zw, color=color, alpha=0.4, linewidth=0)

def draw_all_meshes(mesh_folder):
    fig = plt.figure(figsize=(8, 8))
    ax = fig.add_subplot(111, projection='3d')

    colors = plt.cm.tab10(np.linspace(0, 1, 10))
    color_idx = 0

    for file in sorted(os.listdir(mesh_folder)):
        if not file.endswith(".mesh"):
            continue

        mesh_name = os.path.splitext(file)[0]
        print(f"[시각화 중] {mesh_name}")

        vertices = parse_mesh(os.path.join(mesh_folder, file))
        bone_groups = group_by_bone(vertices)

        for bone_id, points in bone_groups.items():
            if len(points) < 5:
                continue

            axis, radius, c1, c2 = CapsuleExtractor(points)

            color = colors[color_idx % len(colors)]
            draw_cylinder_surface(ax, c1, c2, radius, color=color)
            color_idx += 1

    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.set_zlabel("Z")
    ax.set_title("All Mesh Capsules")
    ax.set_box_aspect([1, 1, 1])

    def set_axes_equal(ax):
        import numpy as np
        xlim = ax.get_xlim3d(); xmid = np.mean(xlim); xrad = (xlim[1]-xlim[0]) * 0.5
        ylim = ax.get_ylim3d(); ymid = np.mean(ylim); yrad = (ylim[1]-ylim[0]) * 0.5
        zlim = ax.get_zlim3d(); zmid = np.mean(zlim); zrad = (zlim[1]-zlim[0]) * 0.5
        R = max(xrad, yrad, zrad)
        ax.set_xlim3d([xmid-R, xmid+R])
        ax.set_ylim3d([ymid-R, ymid+R])
        ax.set_zlim3d([zmid-R, zmid+R])

    set_axes_equal(ax)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    # save_capsule_to_json(
    #     object_name="Knight",
    #     mesh_folder=r"C:\Users\Hadenpel\Desktop\GameServer\Graduation Project Server\Graduation Project ServerCore\animation",
    #     output_path=r"C:\Users\Hadenpel\Desktop\GameServer\Graduation Project Server\Graduation Project ServerCore\animation\knight_capules.json"
    # )
    
   mesh_folder = r"C:\Users\Hadenpel\Desktop\GameServer\Graduation Project Server\Graduation Project ServerCore\animation"
   draw_all_meshes(mesh_folder)
    
    
    
def draw_capsule(points, c1, c2, radius, n_steps=40):
    P = np.array(points)
    u = c2 - c1
    height = np.linalg.norm(u)
    u /= height  # 방향 단위벡터

    # 임의의 보조 축 벡터 (u와 수직)
    v = np.array([1, 0, 0])
    if np.allclose(np.cross(u, v), 0):  # 거의 평행일 경우
        v = np.array([0, 1, 0])
    w = np.cross(u, v)
    v = np.cross(w, u)
    v /= np.linalg.norm(v)
    w /= np.linalg.norm(w)

    # 각도 샘플
    theta = np.linspace(0, 2*np.pi, n_steps)
    z = np.linspace(0, height, n_steps)
    theta_grid, z_grid = np.meshgrid(theta, z)

    # 실린더 표면 생성
    X = radius * np.cos(theta_grid)
    Y = radius * np.sin(theta_grid)
    Z = z_grid

    # 로컬 → 월드 변환
    Xw = c1[0] + X*v[0] + Y*w[0] + Z*u[0]
    Yw = c1[1] + X*v[1] + Y*w[1] + Z*u[1]
    Zw = c1[2] + X*v[2] + Y*w[2] + Z*u[2]

    # 시각화
    fig = plt.figure(figsize=(6, 6))
    ax = fig.add_subplot(111, projection='3d')

    # 점 구름
    ax.scatter(P[:,0], P[:,1], P[:,2], s=10, color='gray', alpha=0.5)

    # 축 (중심선)
    ax.plot([c1[0], c2[0]], [c1[1], c2[1]], [c1[2], c2[2]],
            color='red', linewidth=3, label='Capsule Axis')

    # 캡슐 원통 표면
    ax.plot_surface(Xw, Yw, Zw, color='skyblue', alpha=0.5, linewidth=0)

    # 양쪽 구머리
    def draw_sphere(center):
        phi, theta = np.mgrid[0:np.pi:20j, 0:2*np.pi:20j]
        Xs = center[0] + radius * np.sin(phi) * np.cos(theta)
        Ys = center[1] + radius * np.sin(phi) * np.sin(theta)
        Zs = center[2] + radius * np.cos(phi)
        ax.plot_surface(Xs, Ys, Zs, color='skyblue', alpha=0.5, linewidth=0)
        
    draw_sphere(c1)
    draw_sphere(c2)

    # 축 설정
    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.set_zlabel("Z")
    ax.legend()
    ax.set_box_aspect([1, 1, 1])
    
    xlim = ax.get_xlim3d();  xmid = np.mean(xlim);  xrad = (xlim[1]-xlim[0]) * 0.5
    ylim = ax.get_ylim3d();  ymid = np.mean(ylim);  yrad = (ylim[1]-ylim[0]) * 0.5
    zlim = ax.get_zlim3d();  zmid = np.mean(zlim);  zrad = (zlim[1]-zlim[0]) * 0.5
    R = max(xrad, yrad, zrad)
    ax.set_xlim3d([xmid-R, xmid+R])
    ax.set_ylim3d([ymid-R, ymid+R])
    ax.set_zlim3d([zmid-R, zmid+R])
    
    plt.tight_layout()
    plt.show()

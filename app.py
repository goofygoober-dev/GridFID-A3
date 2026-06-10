import os
import tempfile

import numpy as np
import streamlit as st
import trimesh
import cv2


# Gridfinity specifications from the referenced PDF profile.
PITCH = 42.0
TOLERANCE = 0.5
BASE_W = PITCH - TOLERANCE
FOOT_H = 0.8
TAPER_H = 2.15
NECK_H = 1.8
TOTAL_BASE_H = 4.75
UNIT_H = 7.0
DEFAULT_SCALE = 0.4


st.set_page_config(page_title="Gridfinity Forge", layout="wide")


def create_gridfinity_base(grid_x: int, grid_y: int) -> trimesh.Trimesh:
    """Create a standard Gridfinity base using extruded rectangles."""
    meshes = []

    for ix in range(grid_x):
        for iy in range(grid_y):
            cx = (ix * PITCH) - (grid_x * PITCH / 2) + (PITCH / 2)
            cy = (iy * PITCH) - (grid_y * PITCH / 2) + (PITCH / 2)

            # Create foot
            foot_verts = np.array([
                [-BASE_W/2, -BASE_W/2, 0],
                [BASE_W/2, -BASE_W/2, 0],
                [BASE_W/2, BASE_W/2, 0],
                [-BASE_W/2, BASE_W/2, 0],
                [-BASE_W/2, -BASE_W/2, FOOT_H],
                [BASE_W/2, -BASE_W/2, FOOT_H],
                [BASE_W/2, BASE_W/2, FOOT_H],
                [-BASE_W/2, BASE_W/2, FOOT_H],
            ]) + np.array([cx, cy, 0])

            foot_faces = np.array([
                [0, 1, 5], [0, 5, 4],
                [1, 2, 6], [1, 6, 5],
                [2, 3, 7], [2, 7, 6],
                [3, 0, 4], [3, 4, 7],
                [4, 5, 6], [4, 6, 7],
                [0, 3, 2], [0, 2, 1],
            ])

            foot = trimesh.Trimesh(vertices=foot_verts, faces=foot_faces)
            meshes.append(foot)

            # Create neck
            neck_w = 37.2
            neck_verts = np.array([
                [-neck_w/2, -neck_w/2, FOOT_H],
                [neck_w/2, -neck_w/2, FOOT_H],
                [neck_w/2, neck_w/2, FOOT_H],
                [-neck_w/2, neck_w/2, FOOT_H],
                [-neck_w/2, -neck_w/2, FOOT_H + NECK_H],
                [neck_w/2, -neck_w/2, FOOT_H + NECK_H],
                [neck_w/2, neck_w/2, FOOT_H + NECK_H],
                [-neck_w/2, neck_w/2, FOOT_H + NECK_H],
            ]) + np.array([cx, cy, 0])

            neck_faces = np.array([
                [0, 1, 5], [0, 5, 4],
                [1, 2, 6], [1, 6, 5],
                [2, 3, 7], [2, 7, 6],
                [3, 0, 4], [3, 4, 7],
                [4, 5, 6], [4, 6, 7],
            ])

            neck = trimesh.Trimesh(vertices=neck_verts, faces=neck_faces)
            meshes.append(neck)

    return trimesh.util.concatenate(meshes)


def decode_uploaded_image(uploaded_file) -> np.ndarray:
    """Decode uploaded image file to numpy array."""
    file_bytes = np.asarray(bytearray(uploaded_file.read()), dtype=np.uint8)
    return cv2.imdecode(file_bytes, cv2.IMREAD_COLOR)


def detect_tool_contour(image: np.ndarray, threshold: int):
    """Detect tool contour from image using threshold."""
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    _, thresh = cv2.threshold(gray, threshold, 255, cv2.THRESH_BINARY)
    contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    if not contours:
        return None, thresh

    image_area = image.shape[0] * image.shape[1]
    candidates = [cnt for cnt in contours if cv2.contourArea(cnt) > image_area * 0.001]

    if not candidates:
        return None, thresh

    return max(candidates, key=cv2.contourArea), thresh


def contour_to_centered_points(contour, scale: float, x: int, y: int, w: int, h: int):
    """Convert contour to centered polygon points for CAD model."""
    epsilon = 0.005 * cv2.arcLength(contour, True)
    approx = cv2.approxPolyDP(contour, epsilon, True)
    center_x = x * scale + (w * scale / 2)
    center_y = y * scale + (h * scale / 2)

    return [
        ((point[0][0] * scale) - center_x, (point[0][1] * scale) - center_y)
        for point in approx
    ]


def build_model(
    contour,
    grid_x: int,
    grid_y: int,
    bbox,
    scale: float,
    tool_depth: float,
    extra_h_units: int,
    add_mags: bool,
    add_scoops: bool,
    scoop_rad: float,
) -> trimesh.Trimesh:
    """Build the 3D CAD model for the Gridfinity bin."""
    x, y, w, h = bbox
    model = create_gridfinity_base(grid_x, grid_y)

    requested_body_h = extra_h_units * UNIT_H
    body_h = max(requested_body_h, tool_depth + 2.0)
    
    # Create body box
    body_width = grid_x * PITCH
    body_height = grid_y * PITCH
    body_box = trimesh.creation.box(
        extents=[body_width, body_height, body_h],
        transform=trimesh.transformations.translation_matrix([0, 0, FOOT_H + body_h/2])
    )
    model = trimesh.util.concatenate([model, body_box])

    # Create pocket from contour
    pocket_points = contour_to_centered_points(contour, scale, x, y, w, h)
    pocket_polygon = np.array([(p[0], p[1]) for p in pocket_points])
    
    try:
        # Extrude pocket
        pocket_mesh = trimesh.creation.extrude_polygon(
            pocket_polygon,
            height=tool_depth,
            transform=trimesh.transformations.translation_matrix([0, 0, FOOT_H + body_h - tool_depth/2])
        )
        # Subtract pocket from model
        model = model.difference(pocket_mesh)
    except Exception as e:
        st.warning(f"Could not create pocket: {e}")

    return model


st.title("Gridfinity Advanced Forge")
st.write("Scan a tool on your A3 black board with the 42 mm white grid.")

with st.sidebar:
    st.header("1. Model Specs")
    tool_depth = st.slider("Pocket Depth (mm)", 5, 40, 12)
    extra_h_units = st.number_input("Extra Height Units (u)", 0, 10, 2)
    padding = st.slider("Tool Padding (mm)", 0.0, 5.0, 1.0, 0.5)
    scale = st.number_input("Image Scale (mm / px)", 0.01, 5.0, DEFAULT_SCALE, 0.01)
    threshold = st.slider("Detection Threshold", 1, 254, 70)

    st.header("2. Features")
    add_mags = st.checkbox("Add Magnet Holes (6.5 mm)", True)
    add_scoops = st.checkbox("Add Finger Scoops", True)
    scoop_rad = st.slider("Scoop Size (mm)", 10, 30, 18)

img_file = st.file_uploader("Upload Tool Photo", type=["png", "jpg", "jpeg"])

if img_file:
    img = decode_uploaded_image(img_file)

    if img is None:
        st.error("Could not read that image. Try a PNG or JPG exported from the camera roll.")
        st.stop()

    tool_cnt, preview_mask = detect_tool_contour(img, threshold)

    col_img, col_mask = st.columns(2)
    with col_img:
        st.image(cv2.cvtColor(img, cv2.COLOR_BGR2RGB), caption="Uploaded image")
    with col_mask:
        st.image(preview_mask, caption="Detection mask")

    if tool_cnt is None:
        st.warning("No tool outline found. Try adjusting the detection threshold or image contrast.")
        st.stop()

    x, y, w, h = cv2.boundingRect(tool_cnt)
    w_mm = (w * scale) + (padding * 2)
    h_mm = (h * scale) + (padding * 2)
    grid_x = max(1, int(np.ceil(w_mm / PITCH)))
    grid_y = max(1, int(np.ceil(h_mm / PITCH)))

    st.success(
        f"Detected tool: {w_mm:.1f} mm x {h_mm:.1f} mm. "
        f"Using a {grid_x} x {grid_y} Gridfinity footprint."
    )

    if st.button("Build STL Now", type="primary"):
        with st.spinner("Forging your 3D model..."):
            tmp_name = None
            try:
                model = build_model(
                    contour=tool_cnt,
                    grid_x=grid_x,
                    grid_y=grid_y,
                    bbox=(x, y, w, h),
                    scale=scale,
                    tool_depth=tool_depth,
                    extra_h_units=int(extra_h_units),
                    add_mags=add_mags,
                    add_scoops=add_scoops,
                    scoop_rad=scoop_rad,
                )

                with tempfile.NamedTemporaryFile(delete=False, suffix=".stl") as tmp:
                    tmp_name = tmp.name

                model.export(tmp_name)

                with open(tmp_name, "rb") as stl_file:
                    st.download_button(
                        "Download Gridfinity STL",
                        stl_file,
                        file_name="custom_tool_bin.stl",
                        mime="model/stl",
                    )
            except Exception as exc:
                st.error(f"Could not build STL: {exc}")
            finally:
                if tmp_name and os.path.exists(tmp_name):
                    os.unlink(tmp_name)

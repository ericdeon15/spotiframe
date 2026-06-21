from flask import Flask, jsonify, request, send_file
import spotipy
from spotipy.exceptions import SpotifyException
from spotipy.oauth2 import SpotifyOAuth
import requests
from io import BytesIO
from PIL import Image, ImageStat
from dotenv import load_dotenv
import os

load_dotenv()
app = Flask(__name__)

# --- Cache state ---
cached_track_id = None
cached_payload = {}
cached_device_id = None

# --- Album size ---
RESIZE_PIXELS = 420
SPOTIFY_SCOPE = (
    "user-read-currently-playing "
    "user-read-playback-state "
    "user-modify-playback-state"
)

def open_image_and_color(url, get_color=True):
    response = requests.get(url)
    response.raise_for_status()

    img = Image.open(BytesIO(response.content)).convert("RGB")

    color_hex = None
    if get_color:
        small = img.convert("RGB").resize((50, 50))
        stat = ImageStat.Stat(small)
        r, g, b = [int(x) for x in stat.mean[:3]]
        color_hex = f"#{r:02x}{g:02x}{b:02x}"

    return img, color_hex

def get_spotify_client():
    refresh_token = os.getenv("SPOTIFY_REFRESH_TOKEN")
    if refresh_token:
        # Use the stored refresh token (no interactive login)
        auth_manager = SpotifyOAuth(
            client_id=os.getenv("SPOTIPY_CLIENT_ID"),
            client_secret=os.getenv("SPOTIPY_CLIENT_SECRET"),
            redirect_uri=os.getenv("SPOTIPY_REDIRECT_URI"),
            scope=SPOTIFY_SCOPE
        )
        auth_manager.refresh_access_token(refresh_token)
        return spotipy.Spotify(auth_manager=auth_manager)
    else:
        # Fallback for local testing
        return spotipy.Spotify(auth_manager=SpotifyOAuth(
            scope=SPOTIFY_SCOPE
        ))

sp = get_spotify_client()

@app.route("/current")
def current():
    global cached_track_id, cached_payload, cached_device_id

    track = sp.current_playback()

    if not track or not track.get("item"):
        return jsonify({"status": "stopped"})

    device = track.get("device") or {}
    if device.get("id"):
        cached_device_id = device["id"]
    
    item = track["item"]
    track_id = item["id"]
    
    is_playing = bool(track.get("is_playing"))

    # Reuse cached metadata if track hasn't changed, but keep playback state live.
    if track_id == cached_track_id:
        cached_payload["is_playing"] = is_playing
        return jsonify(cached_payload)

    title = item["name"]
    artist = ", ".join(a["name"] for a in item["artists"])
    album_url = item["album"]["images"][0]["url"] if item["album"]["images"] else None
    _, color_hex = open_image_and_color(album_url)

    payload = {
        "title": title,
        "artist": artist,
        "id": track_id,
        "album_url": album_url,
        "color" : color_hex,
        "status": "playing",
        "is_playing": is_playing
    }

    cached_track_id = track_id
    cached_payload = payload

    return jsonify(payload)

@app.route("/album")
def album():
    global cached_payload

    url = cached_payload.get("album_url")

    if not url:
        return "no cached album", 404
    
    img, _ = open_image_and_color(url, get_color=False)
    img = img.resize((RESIZE_PIXELS, RESIZE_PIXELS))

    buf = BytesIO()
    img.save(buf, "PNG")
    buf.seek(0)
    return send_file(buf, mimetype="image/png")

def get_playback_device_id(playback=None):
    global cached_device_id

    device = (playback or {}).get("device") or {}
    if device.get("id"):
        cached_device_id = device["id"]
        return cached_device_id

    if cached_device_id:
        return cached_device_id

    devices = sp.devices().get("devices", [])
    available_devices = [
        device for device in devices
        if device.get("id") and not device.get("is_restricted")
    ]

    if not available_devices:
        return None

    active_device = next(
        (device for device in available_devices if device.get("is_active")),
        available_devices[0]
    )
    cached_device_id = active_device["id"]
    return cached_device_id

def toggle_playback():
    playback = sp.current_playback()
    device_id = get_playback_device_id(playback)

    if not device_id:
        raise ValueError("No active Spotify playback device")

    if playback and playback.get("is_playing"):
        sp.pause_playback(device_id=device_id)
        return {"is_playing": False}

    sp.start_playback(device_id=device_id)
    return {"is_playing": True}

CONTROL_ACTIONS = {
    "toggle_playback": toggle_playback,
}

@app.put("/control")
def control():
    payload = request.get_json(silent=True)

    if not isinstance(payload, dict):
        return jsonify({"success": False, "error": "Expected a JSON object"}), 400

    action = payload.get("action")
    handler = CONTROL_ACTIONS.get(action)

    if handler is None:
        return jsonify({
            "success": False,
            "error": f"Unsupported action: {action}"
        }), 400

    try:
        result = handler()
        return jsonify({"success": True, "action": action, **result})
    except ValueError as exc:
        return jsonify({"success": False, "error": str(exc)}), 409
    except SpotifyException as exc:
        app.logger.exception("Spotify control action failed")
        return jsonify({
            "success": False,
            "error": exc.msg or "Spotify API request failed"
        }), exc.http_status or 502
    except Exception:
        app.logger.exception("Unexpected control action failure")
        return jsonify({
            "success": False,
            "error": "Unexpected backend error"
        }), 500

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5050)

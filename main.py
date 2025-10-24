from flask import Flask, jsonify, send_file
import spotipy
from spotipy.oauth2 import SpotifyOAuth, SpotifyClientCredentials
import requests
from io import BytesIO
from PIL import Image
from dotenv import load_dotenv
import os
from colorthief import ColorThief

load_dotenv()
app = Flask(__name__)

# --- Cache state ---
cached_track_id = None
cached_payload = {}

# --- Album size ---
RESIZE_PIXELS = 420

def open_image_and_color(url, get_color=True):
    response = requests.get(url)
    response.raise_for_status()

    img = Image.open(BytesIO(response.content)).convert("RGB")

    color_hex = None
    if get_color:
        ct = ColorThief(BytesIO(response.content))
        r, g, b = ct.get_color(quality=1)
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
            scope="user-read-currently-playing user-read-playback-state"
        )
        auth_manager.refresh_access_token(refresh_token)
        return spotipy.Spotify(auth_manager=auth_manager)
    else:
        # Fallback for local testing
        return spotipy.Spotify(auth_manager=SpotifyOAuth(
            scope="user-read-currently-playing user-read-playback-state"
        ))

sp = get_spotify_client()

@app.after_request
def disable_chunked(response):
    response.headers["Connection"] = "close"
    response.headers["Transfer-Encoding"] = "identity"
    return response

@app.route("/current")
def current():
    global cached_track_id, cached_payload

    track = sp.current_user_playing_track()

    if not track or not track.get("item"):
        return jsonify({"status": "stopped"})
    
    item = track["item"]
    track_id = item["id"]
    
    # Reuse cache if track hasn’t changed
    if track_id == cached_track_id:
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
        "status": "playing"
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

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5050)

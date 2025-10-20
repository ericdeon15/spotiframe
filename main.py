from flask import Flask, jsonify, send_file
import spotipy
from spotipy.oauth2 import SpotifyOAuth
import requests
from io import BytesIO
from PIL import Image
from dotenv import load_dotenv

load_dotenv()

app = Flask(__name__)

sp = spotipy.Spotify(auth_manager=SpotifyOAuth(
    scope="user-read-currently-playing"
))

@app.route("/current")
def current():
    track = sp.current_user_playing_track()
    if not track or not track.get("item"):
        return jsonify({"status": "stopped"})

    item = track["item"]
    title = item["name"]
    artist = ", ".join(a["name"] for a in item["artists"])
    album_url = item["album"]["images"][0]["url"] if item["album"]["images"] else None

    return jsonify({
        "title": title,
        "artist": artist,
        "album_url": album_url,
        "status": "playing"
    })

@app.route("/album")
def album():
    track = sp.current_user_playing_track()
    if not track or not track.get("item"):
        return "no track", 404
    url = track["item"]["album"]["images"][0]["url"]
    img = Image.open(BytesIO(requests.get(url).content))
    img = img.resize((200, 200))
    buf = BytesIO()
    img.save(buf, format="PNG")
    buf.seek(0)
    return send_file(buf, mimetype="image/png")

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5050)

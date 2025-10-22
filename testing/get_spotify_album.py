'''
Simple program that gets info about the currently playing song
'''
import spotipy
from spotipy.oauth2 import SpotifyOAuth
from dotenv import load_dotenv

load_dotenv()

sp = spotipy.Spotify(auth_manager=SpotifyOAuth(
    scope="user-read-currently-playing user-read-playback-state"
))

try:
    result = sp.current_user_playing_track()
except Exception as e:
    import traceback
    traceback.print_exc()

if result and result.get("item"):
    track = result["item"]
    print(f"Now playing: {track['name']} — {track['artists'][0]['name']}")
else:
    print("Nothing is currently playing or permission missing.")


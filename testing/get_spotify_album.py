

# sp = spotipy.Spotify(auth_manager=SpotifyClientCredentials(client_id="", client_secret=""))

# # results = sp.search(q='noahkahan', limit=20)    
# # for idx, track in enumerate(results['tracks']['items']):
# #     print(idx, track['name'])

# # Get the currently playing track
# current_track = sp.current_user_playing_track()

# if current_track is not None and current_track['is_playing']:
#     track_name = current_track['item']['name']
#     artist_name = current_track['item']['artists'][0]['name']
#     print(f"🎵 Currently playing: {track_name} — {artist_name}")
# else:
#     print("No song currently playing.")

import spotipy
from spotipy.oauth2 import SpotifyOAuth
from dotenv import load_dotenv

load_dotenv()

sp = spotipy.Spotify(auth_manager=SpotifyOAuth(
    scope="user-read-currently-playing user-read-playback-state"
))

# result = sp.current_user_playing_track()

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


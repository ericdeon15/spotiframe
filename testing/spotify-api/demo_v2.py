'''
Sample program of similar visual and functional experience to what is desired for the
Spotiframe
'''

import requests
from io import BytesIO
from PIL import Image, ImageTk, ImageStat
import tkinter as tk
import spotipy
from spotipy.oauth2 import SpotifyOAuth
from dotenv import load_dotenv

load_dotenv()

sp = spotipy.Spotify(auth_manager=SpotifyOAuth(
    scope="user-read-currently-playing user-read-playback-state"
))

root = tk.Tk()
root.title("Spotify Now Playing")
root.geometry("420x540")

img_label = tk.Label(root)
img_label.pack(pady=20)

title_label = tk.Label(root, text="", font=("Helvetica", 14, "bold"), wraplength=380)
title_label.pack()
artist_label = tk.Label(root, text="", font=("Helvetica", 12), wraplength=380)
artist_label.pack()

current_id = None
current_color = (30, 30, 30)  # default dark tone


def rgb_to_hex(rgb):
    return f"#{int(rgb[0]):02x}{int(rgb[1]):02x}{int(rgb[2]):02x}"


def get_dominant_color(image):
    # Ensure RGB, shrink for speed, average all pixels
    small = image.convert("RGB").resize((50, 50))
    stat = ImageStat.Stat(small)
    mean = stat.mean[:3]  # just R,G,B
    return tuple(int(v) for v in mean)


def transition_color(start, end, steps=20, delay=20):
    # handle bad color input gracefully
    if not (isinstance(start, (tuple, list)) and len(start) == 3):
        start = (30, 30, 30)
    if not (isinstance(end, (tuple, list)) and len(end) == 3):
        end = (30, 30, 30)

    r1, g1, b1 = start
    r2, g2, b2 = end

    for i in range(1, steps + 1):
        r = r1 + (r2 - r1) * i / steps
        g = g1 + (g2 - g1) * i / steps
        b = b1 + (b2 - b1) * i / steps
        color = f"#{int(r):02x}{int(g):02x}{int(b):02x}"
        root.configure(bg=color)
        title_label.configure(bg=color)
        artist_label.configure(bg=color)
        root.update()
        root.after(delay)


def update_track():
    global current_id, current_color
    track = sp.current_user_playing_track()

    if track and track.get("item"):
        item = track["item"]
        track_id = item["id"]

        if track_id != current_id:
            current_id = track_id
            name = item["name"]
            artists = ", ".join(a["name"] for a in item["artists"])
            url = item["album"]["images"][0]["url"]

            # Update image + color
            img_data = requests.get(url).content
            img = Image.open(BytesIO(img_data)).resize((320, 320))
            new_color = get_dominant_color(img)

            # Smooth transition to new album color
            transition_color(current_color, new_color)
            current_color = new_color

            tk_img = ImageTk.PhotoImage(img)
            img_label.configure(image=tk_img)
            img_label.image = tk_img

            title_label.config(text=name)
            artist_label.config(text=artists)

    root.after(1000, update_track)


update_track()
root.mainloop()

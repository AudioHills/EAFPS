"""Synthesizes the placeholder sound effects referenced by the game code.

Standard library only (wave/math/random/struct). Writes 16-bit mono WAVs to
SourceAssets/Audio/, which Scripts/Setup.py imports into /Game/Audio.

    python Tools/gen_sfx.py
"""

import math
import os
import random
import struct
import wave

RATE = 44100
OUT_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "SourceAssets", "Audio")

rng = random.Random(1337)  # deterministic output


# ---------------------------------------------------------------------------
# Building blocks. A "signal" is a plain list of floats in [-1, 1].
# ---------------------------------------------------------------------------

def n(seconds):
    return max(1, int(seconds * RATE))


def silence(seconds):
    return [0.0] * n(seconds)


def noise(seconds):
    return [rng.uniform(-1.0, 1.0) for _ in range(n(seconds))]


def tone(seconds, freq, shape="sine", freq_end=None, vibrato=0.0, vib_rate=5.0):
    """Oscillator with optional linear pitch sweep and vibrato (in semitones)."""
    count = n(seconds)
    out = []
    phase = 0.0
    f_end = freq if freq_end is None else freq_end
    for i in range(count):
        t = i / RATE
        f = freq + (f_end - freq) * (i / count)
        if vibrato:
            f *= 2 ** (vibrato * math.sin(2 * math.pi * vib_rate * t) / 12.0)
        phase += f / RATE
        p = phase % 1.0
        if shape == "sine":
            v = math.sin(2 * math.pi * p)
        elif shape == "saw":
            v = 2.0 * p - 1.0
        elif shape == "square":
            v = 1.0 if p < 0.5 else -1.0
        elif shape == "tri":
            v = 4.0 * abs(p - 0.5) - 1.0
        else:
            raise ValueError(shape)
        out.append(v)
    return out


def env_exp(sig, decay, attack=0.002):
    """Linear attack then exponential decay (decay = time to fall to ~37%)."""
    a = max(1, n(attack))
    return [s * (i / a if i < a else math.exp(-(i - a) / (decay * RATE))) for i, s in enumerate(sig)]


def env_adsr(sig, attack, release):
    count = len(sig)
    a, r = max(1, n(attack)), max(1, n(release))
    out = []
    for i, s in enumerate(sig):
        g = 1.0
        if i < a:
            g = i / a
        if i > count - r:
            g *= max(0.0, (count - i) / r)
        out.append(s * g)
    return out


def lowpass(sig, cutoff, cutoff_end=None):
    out, y = [], 0.0
    count = len(sig)
    for i, s in enumerate(sig):
        c = cutoff if cutoff_end is None else cutoff + (cutoff_end - cutoff) * i / count
        alpha = 1.0 - math.exp(-2 * math.pi * c / RATE)
        y += alpha * (s - y)
        out.append(y)
    return out


def highpass(sig, cutoff):
    low = lowpass(sig, cutoff)
    return [s - l for s, l in zip(sig, low)]


def bandpass(sig, lo, hi):
    return lowpass(highpass(sig, lo), hi)


def gain(sig, g):
    return [s * g for s in sig]


def mix(*sigs):
    length = max(len(s) for s in sigs)
    out = [0.0] * length
    for s in sigs:
        for i, v in enumerate(s):
            out[i] += v
    return out


def at(sig, offset):
    """Delay a signal by offset seconds."""
    return silence(offset) + sig


def concat(*sigs):
    out = []
    for s in sigs:
        out.extend(s)
    return out


def drive(sig, amount):
    """Soft clip for grit."""
    k = math.tanh(amount)
    return [math.tanh(s * amount) / k for s in sig]


def normalize(sig, peak=0.89):
    m = max((abs(s) for s in sig), default=0.0)
    return sig if m < 1e-9 else [s * peak / m for s in sig]


def fade_out(sig, seconds=0.01):
    r = min(len(sig), n(seconds))
    for i in range(r):
        sig[len(sig) - r + i] *= 1.0 - i / r
    return sig


def write(name, sig, peak=0.89):
    sig = fade_out(normalize(sig, peak))
    path = os.path.join(OUT_DIR, name + ".wav")
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(b"".join(struct.pack("<h", int(max(-1.0, min(1.0, s)) * 32767)) for s in sig))
    print(f"  {name}.wav  {len(sig) / RATE:.2f}s")


# ---------------------------------------------------------------------------
# Instruments
# ---------------------------------------------------------------------------

def gunshot(length, thump_hz, crack_cut, body_cut, thump_amt=1.0, crack_amt=1.0, tail=0.0, grit=2.0):
    crack = env_exp(highpass(noise(0.05), 1800), 0.006)
    crack = lowpass(crack, crack_cut)
    body = env_exp(lowpass(noise(length), body_cut, body_cut * 0.25), length * 0.22)
    thump = env_exp(tone(0.25, thump_hz, "sine", thump_hz * 0.45), 0.05)
    parts = [gain(crack, crack_amt), body, gain(thump, thump_amt)]
    if tail:
        # Slap-back off nearby walls.
        echo = env_exp(lowpass(noise(tail), 900), tail * 0.35)
        parts.append(at(gain(echo, 0.25), 0.06))
    return drive(mix(*parts), grit)


def click(freq=3000, decay=0.004, amt=1.0):
    return gain(env_exp(bandpass(noise(0.03), freq * 0.5, freq * 1.5), decay, 0.0005), amt)


def bell(seconds, freq, decay=0.6, bright=0.5):
    partials = [(1.0, 1.0), (2.76, 0.5 * bright), (5.4, 0.25 * bright), (8.93, 0.12 * bright)]
    return mix(*[gain(env_exp(tone(seconds, freq * r), decay / (1 + i * 0.8)), a) for i, (r, a) in enumerate(partials)])


def growl(seconds, base, end, rough=0.6, cutoff=900, vib=1.2):
    voice = tone(seconds, base, "saw", end, vibrato=vib, vib_rate=7.3)
    sub = tone(seconds, base * 0.5, "square", end * 0.5, vibrato=vib, vib_rate=7.3)
    breath = lowpass(noise(seconds), 1400)
    sig = mix(voice, gain(sub, 0.4), gain(breath, rough))
    # Formant-ish wobble: sweep the filter a little.
    sig = lowpass(sig, cutoff, cutoff * 0.6)
    # Amplitude flutter.
    sig = [s * (0.75 + 0.25 * math.sin(2 * math.pi * 11.0 * i / RATE + math.sin(i / 3000.0))) for i, s in enumerate(sig)]
    return drive(env_adsr(sig, 0.08, seconds * 0.4), 1.8)


def pad(seconds, freqs, cutoff=1200, attack=0.6, release=1.5, detune=0.004):
    voices = []
    for f in freqs:
        for d in (-detune, 0.0, detune):
            voices.append(gain(tone(seconds, f * (1 + d), "saw"), 0.3))
    return env_adsr(lowpass(mix(*voices), cutoff), attack, release)


def note(semitones_from_a4):
    return 440.0 * 2 ** (semitones_from_a4 / 12.0)


def boom(seconds=1.6, depth=1.0):
    rumble = env_exp(lowpass(noise(seconds), 400, 60), seconds * 0.3, 0.01)
    thump = env_exp(tone(0.6, 70, "sine", 28), 0.18)
    crack = env_exp(highpass(noise(0.15), 1200), 0.03)
    return drive(mix(gain(rumble, 1.4 * depth), gain(thump, 1.2 * depth), gain(crack, 0.5)), 2.2)


# ---------------------------------------------------------------------------
# The sound list (names must match the code).
# ---------------------------------------------------------------------------

def build():
    os.makedirs(OUT_DIR, exist_ok=True)
    print(f"Writing to {OUT_DIR}")

    # Weapons
    write("SFX_Pistol_Fire", gunshot(0.35, 160, 9000, 5000, thump_amt=0.8, tail=0.3))
    write("SFX_SMG_Fire", gunshot(0.22, 180, 9000, 5500, thump_amt=0.6, tail=0.2, grit=2.5))
    write("SFX_Rifle_Fire", gunshot(0.4, 120, 10000, 4500, thump_amt=1.0, tail=0.35, grit=2.6))
    write("SFX_Shotgun_Fire", gunshot(0.7, 85, 6000, 3000, thump_amt=1.6, crack_amt=0.8, tail=0.6, grit=3.0))
    write("SFX_LMG_Fire", gunshot(0.45, 100, 8000, 3800, thump_amt=1.3, tail=0.35, grit=3.0))
    write("SFX_Sniper_Fire", gunshot(1.1, 70, 11000, 4000, thump_amt=1.8, crack_amt=1.3, tail=1.0, grit=3.2))
    write("SFX_Revolver_Fire", gunshot(0.6, 95, 8000, 3500, thump_amt=1.5, tail=0.5, grit=2.8))

    write("SFX_Reload_Out", mix(click(2200, 0.006), at(lowpass(env_exp(noise(0.12), 0.03), 2500), 0.03), at(click(1400, 0.01, 0.7), 0.12)))
    write("SFX_Reload_In", mix(lowpass(env_exp(noise(0.08), 0.02), 3000), at(click(2600, 0.005), 0.07), at(click(3200, 0.004, 1.2), 0.2), at(click(1800, 0.006, 0.8), 0.26)))
    write("SFX_Dry_Fire", mix(click(3500, 0.003), at(click(1500, 0.006, 0.5), 0.015)), peak=0.6)
    write("SFX_Weapon_Raise", mix(lowpass(env_exp(noise(0.25), 0.08, 0.05), 1800), at(click(2400, 0.005), 0.18)), peak=0.6)
    write("SFX_Low_Ammo", env_exp(tone(0.06, 2600, "tri"), 0.015), peak=0.4)

    # Feedback
    write("SFX_Hit_Marker", mix(env_exp(tone(0.05, 2900, "tri"), 0.008), click(5000, 0.003, 0.5)), peak=0.55)
    write("SFX_Hit_Kill", mix(env_exp(tone(0.09, 2200, "tri"), 0.02), env_exp(tone(0.09, 1100, "sine"), 0.03), click(4000, 0.004)), peak=0.65)
    write("SFX_Hit_Head", mix(bell(0.35, 1750, 0.18, 0.7), click(6000, 0.003, 0.6)), peak=0.6)
    write("SFX_Impact", mix(env_exp(bandpass(noise(0.15), 400, 5000), 0.025), gain(click(1200, 0.008), 0.6)), peak=0.6)

    # Melee
    whoosh = bandpass(noise(0.3), 500, 3500)
    whoosh = [s * math.sin(math.pi * i / len(whoosh)) ** 2 for i, s in enumerate(whoosh)]
    write("SFX_Knife_Swing", whoosh, peak=0.6)
    write("SFX_Knife_Hit", drive(mix(env_exp(tone(0.2, 130, "sine", 60), 0.05), env_exp(lowpass(noise(0.2), 1200), 0.04), at(env_exp(bandpass(noise(0.15), 300, 1500), 0.05), 0.02)), 2.0))

    # Player
    write("SFX_Player_Hurt", drive(mix(env_exp(tone(0.35, 110, "sine", 55), 0.08), env_exp(lowpass(noise(0.3), 700), 0.06), gain(env_exp(tone(0.3, 330, "saw", 220), 0.06), 0.3)), 2.0))
    heartbeat = concat(env_exp(tone(0.18, 55, "sine", 40), 0.05), silence(0.12), gain(env_exp(tone(0.18, 50, "sine", 38), 0.05), 0.7))
    write("SFX_Player_Down", mix(pad(2.4, [note(-24), note(-21), note(-17)], cutoff=600, attack=0.05, release=1.8), gain(heartbeat, 2.0), at(gain(heartbeat, 1.6), 0.9), at(gain(heartbeat, 1.2), 1.8)))
    slide = lowpass(noise(0.65), 2200, 600)
    write("SFX_Slide", env_adsr([s * (1.0 - 0.6 * i / len(slide)) for i, s in enumerate(slide)], 0.03, 0.25), peak=0.55)
    write("SFX_Land", mix(env_exp(tone(0.2, 90, "sine", 45), 0.05), env_exp(lowpass(noise(0.15), 900), 0.03)), peak=0.6)

    # Zombies (a few variants are baked into one file by pitch at runtime)
    write("SFX_Zombie_Groan", mix(growl(1.6, 88, 72, rough=0.7, cutoff=800), at(growl(0.7, 96, 80, rough=0.9, cutoff=700), 0.9)), peak=0.75)
    write("SFX_Zombie_Attack", growl(0.6, 150, 110, rough=1.2, cutoff=1600, vib=2.0), peak=0.85)
    write("SFX_Zombie_Death", mix(growl(1.0, 120, 50, rough=0.8, cutoff=1000, vib=2.5), env_exp(lowpass(noise(0.5), 600), 0.1)), peak=0.8)

    # Rounds & economy (original motifs: minor-key low brass / pads)
    write("SFX_Round_Start", mix(
        pad(4.0, [note(-33), note(-26)], cutoff=700, attack=0.8, release=2.0),
        at(pad(3.0, [note(-21), note(-18), note(-14)], cutoff=1100, attack=0.4, release=1.8), 0.9),
        at(boom(1.5, 0.5), 0.0)))
    round_end = []
    for i, semi in enumerate([-14, -16, -18, -21]):
        round_end.append(at(pad(2.2, [note(semi), note(semi - 7)], cutoff=900, attack=0.1, release=1.2), i * 0.75))
    write("SFX_Round_End", mix(*round_end, at(pad(2.8, [note(-33)], cutoff=500, attack=0.5, release=1.5), 2.25)))
    write("SFX_Purchase", mix(bell(0.6, note(15), 0.25), at(bell(0.8, note(22), 0.35), 0.09), click(3000, 0.005)), peak=0.7)
    write("SFX_Deny", env_adsr(lowpass(tone(0.25, 110, "square"), 900), 0.005, 0.05), peak=0.5)
    write("SFX_Door_Open", mix(boom(1.0, 0.6), *[at(env_exp(bandpass(noise(0.2), 200, 2500), 0.05), t) for t in (0.0, 0.12, 0.3, 0.45, 0.62)]))
    write("SFX_Perk_Drink", mix(*[at(env_exp(tone(0.12, 180 + i * 25, "sine", 120), 0.04), i * 0.22) for i in range(4)],
                                 at(bell(1.0, note(10), 0.4, 0.3), 1.0), at(bell(1.0, note(17), 0.4, 0.3), 1.12)))

    # Music-box jingle for the mystery box (original melody).
    melody = [7, 10, 14, 12, 10, 7, 5, 7, 10, 7, 3, 5, 7]
    box = []
    for i, semi in enumerate(melody):
        box.append(at(bell(0.9, note(semi + 12), 0.35, 0.4), i * 0.3))
    write("SFX_Box_Jingle", mix(*box, at(pad(4.4, [note(-14)], cutoff=600, attack=0.4, release=1.0), 0.0)), peak=0.7)

    # Power-ups
    write("SFX_PowerUp_Spawn", mix(*[at(env_exp(tone(0.5, f, "sine"), 0.15), i * 0.06) for i, f in enumerate([880, 1175, 1480, 1760])]), peak=0.5)
    arp = lambda base, shape: mix(*[at(env_exp(tone(0.5, note(base + s), shape), 0.12), i * 0.08) for i, s in enumerate([0, 4, 7, 12, 16])])
    write("SFX_PowerUp_MaxAmmo", mix(arp(0, "tri"), at(click(2500, 0.006), 0.0), at(click(2500, 0.006), 0.35)), peak=0.75)
    write("SFX_PowerUp_InstaKill", mix(drive(arp(-5, "saw"), 1.5), at(bell(1.2, note(-17), 0.5, 0.9), 0.4)), peak=0.75)
    write("SFX_PowerUp_DoublePoints", mix(arp(3, "square"), at(arp(15, "sine"), 0.2)), peak=0.7)
    write("SFX_PowerUp_Nuke", mix(boom(2.5, 1.0), at(boom(1.5, 0.6), 0.25)))

    # Game over: slow falling minor chords.
    go = []
    for i, root in enumerate([-21, -23, -26, -28]):
        go.append(at(pad(2.6, [note(root), note(root + 3), note(root + 7)], cutoff=800, attack=0.3, release=1.6), i * 1.2))
    write("SFX_Game_Over", mix(*go, at(pad(4.0, [note(-40)], cutoff=300, attack=1.0, release=2.0), 2.4)))


if __name__ == "__main__":
    build()

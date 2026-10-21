#!/usr/bin/env python3
"""
Piirtää INA226-lokista (CSV) jännite-, virta- ja tehokäyrät.

Käyttö:
    python plot_log.py loki.csv                 # näyttää kuvan
    python plot_log.py loki.csv -o kuva.png     # tallentaa kuvan
    python plot_log.py loki.csv --sep , --decimal .   # englanninkielinen CSV

Vaatii: pandas, matplotlib   (pip install pandas matplotlib)
"""
import argparse
import io
import sys

import pandas as pd
import matplotlib.pyplot as plt


def load_log(path: str, sep: str, decimal: str) -> pd.DataFrame:
    # Sarjaporttilokissa voi olla ylimääräisiä rivejä (boottiloki, "Valmis",
    # "VIRHE: ..."). Pidetään otsikkorivi ja numerolla alkavat datarivit.
    with open(path, encoding="utf-8", errors="replace") as f:
        lines = [ln.strip() for ln in f if ln.strip()]
    header_idx = next((i for i, ln in enumerate(lines) if ln.startswith("Aika(s)")), None)
    if header_idx is None:
        sys.exit("Otsikkoriviä 'Aika(s);...' ei löytynyt tiedostosta.")
    data = [lines[header_idx]] + [ln for ln in lines[header_idx + 1:] if ln[0].isdigit()]

    df = pd.read_csv(io.StringIO("\n".join(data)), sep=sep, decimal=decimal)
    df.columns = [c.strip() for c in df.columns]
    expected = ["Aika(s)", "Jannite(V)", "Virta(A)", "Teho(W)"]
    if list(df.columns[:4]) != expected:
        sys.exit(f"Odottamaton otsikkorivi: {list(df.columns)}\nOdotettiin: {expected}")
    return df.dropna()


def main() -> None:
    ap = argparse.ArgumentParser(description="Piirrä INA226-lokin käyrät.")
    ap.add_argument("csv", help="lokitiedosto")
    ap.add_argument("-o", "--output", help="tallenna kuva tähän tiedostoon (png/svg/pdf)")
    ap.add_argument("--sep", default=";", help="sarake-erotin (oletus ';')")
    ap.add_argument("--decimal", default=",", help="desimaalimerkki (oletus ',')")
    ap.add_argument("--title", default="INA226 power log")
    args = ap.parse_args()

    df = load_log(args.csv, args.sep, args.decimal)
    t = df["Aika(s)"]

    fig, axes = plt.subplots(3, 1, figsize=(10, 7.5), sharex=True)
    series = [
        ("Jannite(V)", "Jännite (V)", "#1f6fb2"),
        ("Virta(A)",   "Virta (A)",   "#c2410c"),
        ("Teho(W)",    "Teho (W)",    "#15803d"),
    ]
    for ax, (col, label, color) in zip(axes, series):
        ax.plot(t, df[col], color=color, linewidth=1.4)
        ax.set_ylabel(label)
        ax.grid(True, alpha=0.3)
        ax.margins(y=0.15)

    axes[-1].set_xlabel("Aika (s)")
    energy_wh = (df["Teho(W)"] * t.diff().fillna(0)).sum() / 3600.0
    fig.suptitle(
        f"{args.title}  |  {len(df)} näytettä, {t.iloc[-1]:.0f} s  |  "
        f"keskiteho {df['Teho(W)'].mean():.3f} W, energia {energy_wh:.4f} Wh"
    )
    fig.tight_layout()

    if args.output:
        fig.savefig(args.output, dpi=150)
        print(f"Tallennettu: {args.output}")
    else:
        plt.show()


if __name__ == "__main__":
    main()

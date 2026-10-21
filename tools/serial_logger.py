#!/usr/bin/env python3
"""
Lukee ESP32-C3:n sarjaporttia ja tallentaa INA226-lokin CSV-tiedostoon.

Tallentaa vain otsikkorivin ja datarivit, joten ESP32:n boottiloki ja
"Valmis"-rivi eivät sotke tiedostoa. Lopettaa automaattisesti, kun laite
tulostaa "Valmis", tai Ctrl+C:llä.

Käyttö:
    python serial_logger.py COM5                  # Windows
    python serial_logger.py /dev/ttyUSB0          # Linux
    python serial_logger.py /dev/cu.usbmodem101   # macOS
    python serial_logger.py COM5 -o mittaus.csv

Vaatii: pyserial   (pip install pyserial)
"""
import argparse
import datetime as dt
import sys

try:
    import serial
except ImportError:
    sys.exit("pyserial puuttuu: pip install pyserial")


def main() -> None:
    ap = argparse.ArgumentParser(description="Tallenna INA226-loki sarjaportista.")
    ap.add_argument("port", help="sarjaportti, esim. COM5 tai /dev/ttyUSB0")
    ap.add_argument("-b", "--baud", type=int, default=115200)
    ap.add_argument("-o", "--output", help="tiedostonimi (oletus: ina226_YYYYMMDD_HHMMSS.csv)")
    ap.add_argument("--reset", action="store_true",
                    help="resetoi ESP32 avattaessa (DTR/RTS), jotta loki alkaa alusta")
    args = ap.parse_args()

    out_name = args.output or dt.datetime.now().strftime("ina226_%Y%m%d_%H%M%S.csv")

    with serial.Serial(args.port, args.baud, timeout=2) as ser, \
         open(out_name, "w", encoding="utf-8", newline="\n") as out:

        if args.reset:
            ser.dtr = False
            ser.rts = True
            ser.rts = False
            ser.reset_input_buffer()

        print(f"Kuunnellaan {args.port} @ {args.baud} -> {out_name}  (Ctrl+C lopettaa)")
        header_seen = False
        rows = 0
        try:
            while True:
                raw = ser.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="replace").strip()
                if not line:
                    continue

                if line.startswith("Aika(s)"):
                    header_seen = True
                    out.write(line + "\n")
                    print(line)
                elif header_seen and line[0].isdigit():
                    out.write(line + "\n")
                    rows += 1
                    if rows % 10 == 0:
                        print(f"\r{rows} riviä...", end="", flush=True)
                elif line.startswith("Valmis"):
                    print(f"\nLaite ilmoitti: Valmis. {rows} riviä tallennettu.")
                    break
                elif line.startswith("VIRHE"):
                    print(f"\nLaite: {line}")
                else:
                    pass  # boottiloki ym. ohitetaan
        except KeyboardInterrupt:
            print(f"\nKeskeytetty. {rows} riviä tallennettu.")

    print(f"Tiedosto: {out_name}")


if __name__ == "__main__":
    main()

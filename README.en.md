# INA226 Power Logger (ESP32-C3)

A minimal voltage/current/power logger: an ESP32-C3 reads an INA226 sensor once per second and prints CSV to the serial port. By default it records 10 minutes (600 samples). The CSV is formatted for Finnish Excel (`;` separator, decimal comma) – change `CSV_SEPARATOR` and `DECIMAL_POINT` at the top of the sketch for `,` and `.`.

The full documentation is in Finnish in [README.md](README.md). This file is a short summary.

![Example plot](docs/example_plot.png)

## Wiring

![Wiring](docs/wiring.png)

| ESP32-C3 | INA226 |
|---|---|
| 3V3 | VCC |
| GND | GND |
| GPIO8 | SDA |
| GPIO9 | SCL |

High-side measurement: supply (+) → VIN+ → shunt → VIN- → load (+). Supply (−) and load (−) must share ground with the INA226/ESP32 GND. VBUS must be tied to VIN+ (most modules do this on the board).

## Library and limits

- Library: **INA226 by Rob Tillaart** (Arduino Library Manager, ≥ 0.6.0).
- The INA226 shunt input range is ±81.92 mV. With the common 0.1 Ω (R100) module the maximum current is **0.8 A**, whatever the code says. `MAX_CURRENT_A × SHUNT_OHM` must stay below 0.080 V or `setMaxCurrentShunt()` returns error 0x8000 and the current/power readings stay at zero.
- `setAverage()` takes a register value 0–7 (`INA226_256_SAMPLES` = 5), not a sample count.

## Tools

```
pip install pyserial pandas matplotlib
python tools/serial_logger.py COM5          # capture to CSV, stops at "Valmis"
python tools/plot_log.py log.csv -o plot.png
```

## License

MIT.

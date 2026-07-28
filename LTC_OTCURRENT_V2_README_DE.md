# otcurrent v2.0 – LTC-Erweiterung

Dieser Quellstand basiert auf `otcurrent_pi` und erweitert die Auswahl der
Tiden-/Strömungsdaten.

## Bedienung

Im otcurrent-Fenster gibt es nur **einen** Datenordner:

1. Das schreibgeschützte Feld zeigt den aktiven Ordner.
2. `Select folder and TCD files...` öffnet zuerst die Ordnerauswahl.
3. Danach können eine oder mehrere `.tcd`-Dateien dieses Ordners markiert
   werden.

Die Auswahl wird in der OpenCPN-Konfiguration gespeichert. Bei mehreren
aktivierten Dateien sollten sich deren Gebiete möglichst nicht überlappen,
weil sonst doppelte Stationen und Pfeile entstehen können.

Alle Dateinamen mit der Endung `.tcd` werden unterstützt. Der frühere feste
Name `harmonics-dwf-20210110-free.tcd` ist nicht mehr erforderlich.

Eine im selben Ordner vorhandene `HARMONIC.IDX` wird zusätzlich als
Legacy-Datensatz geladen.

## Abgrenzung zu OpenCPN

`OpenCPN > Optionen > Seekarten > Tiden + Strömungen` ist eine eigene
OpenCPN-Datenquellenliste. Sie ist nicht die zweite Ordnerauswahl von
otcurrent und steuert nicht die großen otcurrent-Pfeile.

## Build-Status

Der Quellstand `otcurrent_LTC_V.2.2` wurde unter Windows für OpenCPN
erfolgreich kompiliert und praktisch getestet.

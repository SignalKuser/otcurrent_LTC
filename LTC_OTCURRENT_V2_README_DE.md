# otcurrent_LTC v2.5

Dieser Quellstand basiert auf `otcurrent_pi` und erweitert die Auswahl der
Tiden-/Strömungsdaten.

## Bedienung

IDX/HARMONIC-Paare werden wie gewohnt in OpenCPN unter **Optionen →
Seekarten → Tiden und Strömungen** eingetragen. Version 2.5 übernimmt alle
dort konfigurierten, vorhandenen `.idx`-Dateien automatisch – auch mehrere
frei benannte Paare gleichzeitig.

Im otcurrent-Fenster kann direkt zwischen drei Betriebsarten umgeschaltet
werden: **OpenCPN IDX**, **ausgewählte TCD-Dateien** oder **IDX + TCD
gleichzeitig**. Für TCD-Dateien gibt es **einen** Datenordner:

1. Das schreibgeschützte Feld zeigt den aktiven Ordner.
2. `Select folder and TCD files...` öffnet zuerst die Ordnerauswahl.
3. Danach können eine oder mehrere `.tcd`-Dateien dieses Ordners markiert
   werden.

Die Auswahl wird in der OpenCPN-Konfiguration gespeichert. Bei mehreren
aktivierten Dateien sollten sich deren Gebiete möglichst nicht überlappen,
weil sonst doppelte Stationen und Pfeile entstehen können.

Alle Dateinamen mit der Endung `.tcd` werden unterstützt. Der frühere feste
Name `harmonics-dwf-20210110-free.tcd` ist nicht mehr erforderlich.

## Abgrenzung zu OpenCPN

`OpenCPN > Optionen > Seekarten > Tiden + Strömungen` ist die zentrale
IDX-Datenquellenliste. Die dort eingetragenen IDX-Dateien steuern in Version
2.5 auch die großen otcurrent-Pfeile, wenn der IDX- oder Kombinationsmodus
aktiv ist.

## Build-Status

Der Quellstand ist vorbereitet, aber auf diesem Rechner noch nicht
kompiliert. Es fehlen derzeit eine vollständige C++-/CMake-Buildumgebung und
die Inhalte des Git-Submoduls `opencpn-libs`.

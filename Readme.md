# Cache

## Aufgabenstellung

Das Ziel dieses Projektes ist der Vergleich von direkt abbildenden und voll-assoziativen Caches mit verschiedenen Konfigurationen in Bezug auf Zyklenzahl, Gatterzahl und Hit/Miss-Rate. 

## Rechercheergebnisse
Einleitung bliblablub
Übliche Größe von:

- Direkt abbildenden Caches:
- Voll-assoziativen Caches:
- Latenzen memory cache:

### Speicherintensiver Algorithmus
Wir untersuchen im folgenden die Speicherzugriffsmuster der Algorithmen Merge- und Radix-Sort. Diese haben ein stark unterschiedliches Zugriffsmuster: Merge-Sort weist eine höhere räumliche Lokalität auf als Radix-Sort. In eigener Messung [zitat file] war die durchschnittliche Adressabweichung bei aufeinanderfolgenden Memory-Accesses bei Radix-Sort zw. etwa 13% und 60% höher, wobei diese mit größerem Sortier-Array steigt. Weiters weist Merge-Sort auch eine höhere temporale Lokalität auf, da auf jedem Merge-Segment bestimmter Größe genau 1 Mal operiert wird, während Radix-Sort bei jeder Iteration das gesamte Array durchläuft.   [zitat paper]


## Methodik & Messumgebung

Das Speicherzugriffsmuster der Algorithmen wurden mithilfe eines LLVM-Passes für verschiedene Eingabegrößen analysiert und aufgezeichnet. Der LLVM-Pass instrumentiert während des Kompilierprozesses jede Instruktion, die einen Speicherzugriff darstellt, mit einem Aufruf zu einer Log-Funktion, die diesen Zugriff im richtigen Format an ein erstelltes Log-File anfügt.

Diese Algorithmen werden zur Vereinfachung auf einem System in reiner Harvard-Architektur simuliert, in welchem ein CPU über je einen Cache mit Instruktions- und Daten-RAM verbunden ist. Variierbare Parameter an diesem System sind die Cachelatenz, Memorylatenz, Cacheline-Größe, Cacheline-Zahl, der Mapping-Typ und die Replacement-Policy des Caches.

## Implementierung


## Ergebnisse
Diagramme


## Persönlicher Beitrag

- Matilda: C-Rahmenprogramm, Recherche, Tests, Project-Management
- Dominik: Cache, Write-Buffer, Speicherzugriffanalyse, Design Intermodulkommunikation, Tests
- Emina: CPU, RAM, Tests, Aufsetzen der Simulation 

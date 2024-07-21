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
Wir untersuchen im folgenden die Speicherzugriffsmuster der Algorithmen [Merge- und Radix-Sort](tools/BenchmarkInputGenerator/Sort.h). Diese haben [ein stark unterschiedliches Zugriffsmuster](https://doi.org/10.1006/jagm.1998.0985): Merge-Sort weist eine höhere räumliche Lokalität auf als Radix-Sort. In [eigener Messung](tools/BenchmarkInputGenerator/BenchmarkInputGenerator.py) war die durchschnittliche Adressabweichung bei aufeinanderfolgenden Memory-Accesses bei Radix-Sort zw. etwa 13% und 60% höher, wobei diese mit größerem Sortier-Array steigt. Weiters weist Merge-Sort auch eine höhere temporale Lokalität auf, da auf jedem Merge-Segment bestimmter Größe genau 1 Mal operiert wird, während Radix-Sort bei jeder Iteration das gesamte Array durchläuft.


## Methodik & Messumgebung

Das Speicherzugriffsmuster der Algorithmen wurden mithilfe eines [LLVM-Passes](tools/MemoryAnalyser/MemoryAnalyser.cpp) für verschiedene Eingabegrößen analysiert und [aufgezeichnet](examples/merge_sort_10.csv). Der LLVM-Pass instrumentiert während des Kompilierprozesses jede Instruktion, die einen Speicherzugriff darstellt, mit einem Aufruf zu einer Log-Funktion, die diesen Zugriff im richtigen Format an ein erstelltes Log-File anfügt.

Diese Algorithmen werden zur Vereinfachung auf einem System in reiner Harvard-Architektur simuliert, in welchem ein CPU über je einen Cache mit Instruktions- und Daten-RAM verbunden ist. Variierbare Parameter an diesem System sind die Cachelatenz, Memorylatenz, Cacheline-Größe, Cacheline-Zahl, der Mapping-Typ und die Replacement-Policy des Caches.

## Implementierung

Das Design dieses [Caches](src/Simulation/Cache.h) ist angelehnt an das, das in Computer Organization and Design, Sixth Edition, von David A. Patterson and John L. Hennessy. 2020, vorgestellt wird. Kommt es zu einem Cache Miss wird, egal ob Lese- oder Schreibzugriff, erst die Cacheline in den Cache geladen und dann entweder ein 32 Bit Wort an den RAM gesandt oder das gelesene Wort an die CPU. Um durch Writes weniger Zeit zu verlieren, gibt es einen [Write-Buffer](src/Simulation/WriteBuffer.h), wodurch die CPU bereits nach einlesen der Zeile in den Cache den nächsten Befehl ausführen kann. Dieses Verhalten ist ausschaltbar über die Definition von STRICT_INSTRUCTION_ORDER. Das bei der Messung simulierte System besteht aus in Harvard-Architektur organisierten [CPU](src/Simulation/CPU.h), [Instruktion](src/Simulation/InstructionCache.h)- und Datencache sowie Instruktions- und Daten-[RAM](src/Simulation/RAM.h).

## Ergebnisse
Diagramme


## Persönlicher Beitrag

- Matilda: C-Rahmenprogramm, Recherche, Tests, Project-Management
- Dominik: Cache, Write-Buffer, Speicherzugriffanalyse, Design Intermodulkommunikation, Tests
- Emina: CPU, RAM, Tests, Aufsetzen der Simulation 

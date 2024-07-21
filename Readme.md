# Cache

## Aufgabenstellung

Das Ziel dieses Projektes ist der Vergleich von direkt abbildenden und voll-assoziativen Caches mit verschiedenen Konfigurationen in Bezug auf Zyklenzahl, Gatterzahl und Hit/Miss-Rate. 

## Rechercheergebnisse
### Übliche Größen für Caches und ihre Latenzen
Direkt-abbildende Caches haben üblicherweise eine Größe zwischen 16 KB bis 64 KB in L1 Caches und bis zu 1 MB in L2 Caches. Sie werden unter anderem in den L1-Caches von Intel x86- und AMD-Architekturen verwendet [2, S. 3].
Voll-assoziative Caches sind oft kleiner, typischerweise zwischen 8 KB und 64 KB für L1 Caches. Sie werden häufig in Translation Lookaside Buffern (TLBs) verwendet, beispielsweise in den Instruction- und Data-TLBs von Intel® 64 und IA-32 Architekturen, wo sie zwischen 32 und 256 Einträge für L1-TLBs und bis zu 2048 Einträge für L2-TLBs speichern können [3].
Die Latenzen für Caches variieren typischerweise zwischen 1 und 40 Zyklen, abhängig vom Cache-Level. Im Vergleich dazu liegt die Latenz für den Hauptspeicher bei etwa 60 bis 150 Zyklen, was den von Neumann-Flaschenhals deutlich macht.

### Speicherzugriffsverhalten von Merge- und Radixsort
Wir untersuchen im Folgenden die Speicherzugriffsmuster der Algorithmen Merge- und Radix-Sort. Diese haben ein stark unterschiedliches Zugriffsmuster: Merge-Sort weist eine höhere räumliche Lokalität auf als Radix-Sort. In eigener Messung [zitat file] war die durchschnittliche Adressabweichung bei aufeinanderfolgenden Memory-Accesses bei Radix-Sort zw. etwa 13% und 60% höher, wobei diese mit größerem Sortier-Array steigt. Weiters weist Merge-Sort auch eine höhere temporale Lokalität auf, da auf jedem Merge-Segment bestimmter Größe genau 1 Mal operiert wird, während Radix-Sort bei jeder Iteration das gesamte Array durchläuft.   [zitat paper]


## Methodik & Messumgebung

Das Speicherzugriffsmuster der Algorithmen wurden mithilfe eines [LLVM-Passes](tools/MemoryAnalyser/MemoryAnalyser.cpp) für verschiedene Eingabegrößen analysiert und [aufgezeichnet](examples/merge_sort_10.csv). Der LLVM-Pass instrumentiert während des Kompilierprozesses jede Instruktion, die einen Speicherzugriff darstellt, mit einem Aufruf zu einer Log-Funktion, die diesen Zugriff im richtigen Format an ein erstelltes Log-File anfügt.

Diese Algorithmen werden zur Vereinfachung auf einem System in reiner Harvard-Architektur simuliert, in welchem ein CPU über je einen Cache mit Instruktions- und Daten-RAM verbunden ist. Variierbare Parameter an diesem System sind die Cachelatenz, Memorylatenz, Cacheline-Größe, Cacheline-Zahl, der Mapping-Typ und die Replacement-Policy des Caches.
## Implementierung

Das Design dieses [Caches](src/Simulation/Cache.h) ist angelehnt an das, das in Computer Organization and Design, Sixth Edition, von David A. Patterson and John L. Hennessy. 2020, vorgestellt wird. Kommt es zu einem Cache Miss wird, egal ob Lese- oder Schreibzugriff, erst die Cacheline in den Cache geladen und dann entweder ein 32 Bit Wort an den RAM gesandt oder das gelesene Wort an die CPU. Um durch Writes weniger Zeit zu verlieren, gibt es einen [Write-Buffer](src/Simulation/WriteBuffer.h), wodurch die CPU bereits nach einlesen der Zeile in den Cache den nächsten Befehl ausführen kann. Dieses Verhalten ist ausschaltbar über die Definition von STRICT_INSTRUCTION_ORDER. Das bei der Messung simulierte System besteht aus in Harvard-Architektur organisierten [CPU](src/Simulation/CPU.h), [Instruktion](src/Simulation/InstructionCache.h)- und Datencache sowie Instruktions- und Daten-[RAM](src/Simulation/RAM.h).

## Ergebnisse
Diagramme


## Persönliche Beiträge
- Dominik: Cache, Write-Buffer, Speicherzugriffanalyse, Design Intermodulkommunikation, Tests
- Emina: CPU, RAM, Tests, Aufsetzen der Simulation 
- Matilda: C-Rahmenprogramm, Recherche, Tests, Projekt-Management

## Quellen 
[1] D. A. Patterson und J. L. Hennessy, Computer Organization and Design: The Hardware/Software Interface: RISC-V Edition, 5. Aufl., San Francisco: Morgan Kaufmann, 2021, Kapitel 5, pp. [754-804].

[2] S. Kumar und P. K. Singh, "An overview of modern cache memory and performance analysis of replacement policies," 2016 IEEE International Conference on Engineering and Technology (ICETECH), Coimbatore, Indien, 2016, S. 210-214, doi: 10.1109/ICETECH.2016.7569243.

[3] Intel, Intel® 64 and IA-32 Architectures Optimization Reference Manual: Volume 1, Order Number 248966-048, August 2023, Sec. 4.1.7.
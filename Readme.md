# Cache

## Aufgabenstellung

Das Ziel dieses Projektes ist der Vergleich von direkt abbildenden und voll-assoziativen Caches in verschiedenen Anwendungsfällen in Bezug auf Zyklenzahl, Gatterzahl und Hit/Miss rate. 

## Rechercheergebnisse
Übliche Größe von:

- Direkt abbildenden Caches:
- Voll-assoziativen Caches:
- RAM

### Speicherintensiver Algorithmus
Wir untersuchen im folgenden die Speicherzugriffsmuster der Algorithmen Merge- und Radix-Sort. Diese haben ein stark unterschiedliches Zugriffsmuster: Merge-Sort weist eine höhere räumliche Lokalität auf als Radix-Sort. In eigener Messung [zitat file] war die durchschnittliche Adressabweichung bei aufeinanderfolgenden Memory-Accesses bei Radix-Sort zw. etwa 13% und 60% höher, wobei diese mit größerem Sortier-Array steigt. Weiters weist Merge-Sort auch eine höhere temporale Lokalität auf, da auf jedem Merge-Segment bestimmter Größe genau 1 Mal operiert wird, während Radix-Sort bei jeder Iteration das gesamte Array durchläuft.   [zitat paper]



## Methodik & Messumgebung

Das Speicherzugriffsmuster der Algorithmen wurden mithilfe eines LLVM-Passes analysiert und aufgezeichnet. Die Algorithmen selbst wurden in C implementiert und werden von einem Driverprogramm aufgerufen, welches erst zuvor generierte Daten von einem File einliest und nach Sortierung ausgibt, um etwaiges "Wegoptimieren" durch den Compiler zu verhindern. Diese Vor- und Nacharbeit stellen einen geringen Overhead dar, die das Messergebnis nicht verfälschen.

Der LLVM-Pass instrumentiert während des Kompilierprozesses jede Instruktion, die einen Speicherzugriff darstellt, mit einem Aufruf zu einer Log-Funktion, die diesen Zugriff im richtigen Format an ein erstelltes Log-File anfügt. Nach Beendigung der Ausführung der zu analysierenden Binary enthält dieses Log-File nun sämtliche Speicherzugriffe des Algorithmus.

## Ergebnisse

## Persönlicher Beitrag


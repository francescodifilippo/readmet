# Metinfo

## English

### Description
Metinfo is a utility program designed to extract ED2K hashes from .part.met files used by the eDonkey2000/Overnet and eMule peer-to-peer file sharing networks. The program automatically detects the file format version (14.0 or 14.1) and extracts the MD4 hash that uniquely identifies the file in the network, as well as all meta tags contained within the file.

### Installation
To compile and install metinfo, follow these instructions:

```bash
# Clone or download the source code
# Navigate to the project directory
cd metinfo

# Compile the program
make

# Optionally install system-wide (requires root privileges)
sudo make install
```

### Usage
```bash
# Basic usage (shows all meta tags)
./metinfo -f /path/to/file.part.met

# Display version information
./metinfo -V

# Display help information
./metinfo -h

# Show specific tag types
./metinfo -f /path/to/file.part.met -s  # Show only special tags
./metinfo -f /path/to/file.part.met -g  # Show only gap tags
./metinfo -f /path/to/file.part.met -t  # Show only standard tags
./metinfo -f /path/to/file.part.met -u  # Show only unknown tags

# Visualize download status
./metinfo -f /path/to/file.part.met -z

# Detailed output
./metinfo -f /path/to/file.part.met -v
```

### Script-Friendly Single Value Output
For script integration, metinfo provides options to output single values without labels or formatting:

```bash
# Get only the filename 
./metinfo -f /path/to/file.part.met -n
# Output: example_movie.mkv

# Get only the file size (in bytes)
./metinfo -f /path/to/file.part.met -S
# Output: 104857600

# Get only the ED2K hash
./metinfo -f /path/to/file.part.met -e
# Output: E7D81234AB56C890DEF12345ABC67890

# Get only the .part.met file version
./metinfo -f /path/to/file.part.met -m
# Output: 14.0

# Get only the number of meta tags
./metinfo -f /path/to/file.part.met -c
# Output: 42

# Get only the download progress percentage
./metinfo -f /path/to/file.part.met -p
# Output: 37.8
```

### JSON Output
All information can be output in JSON format for easy integration with other tools:

```bash
# Full JSON output
./metinfo -f /path/to/file.part.met -j

# Single value JSON output
./metinfo -f /path/to/file.part.met -e -j
# Output: {"ed2k_hash":"E7D81234AB56C890DEF12345ABC67890"}

./metinfo -f /path/to/file.part.met -m -j
# Output: {"format_version":"14.0"}
```

### Script Examples
```bash
# Check if a file is completely downloaded
if [ $(./metinfo -f /path/to/file.part.met -p) = "100.0" ]; then
    echo "Download complete!"
else
    echo "Download in progress..."
fi

# Collect information about a download
FILENAME=$(./metinfo -f /path/to/file.part.met -n)
SIZE=$(./metinfo -f /path/to/file.part.met -S)
PROGRESS=$(./metinfo -f /path/to/file.part.met -p)
HASH=$(./metinfo -f /path/to/file.part.met -e)

echo "File: $FILENAME"
echo "Size: $SIZE bytes"
echo "Progress: $PROGRESS%"
echo "Hash: $HASH"
```

### How It Works
The program reads the .part.met file and analyzes its structure:

1. It reads the first byte to determine the file format version:
   - 224 (0xE0) = Version 14.0 (eMule, eDonkey pre-0.49)
   - 225 (0xE1) = Version 14.1 (eDonkey/Overnet 0.49+)

2. It then positions the file pointer at the appropriate location for the ED2K hash:
   - Version 14.0: Position 5
   - Version 14.1: Position 6

3. It reads the 16 bytes of the MD4 hash and converts them to a hexadecimal string.

4. It reads and processes all meta tags in the file, including:
   - Special tags (with 1-byte names)
   - Gap tags (indicating undownloaded areas)
   - Standard tags (like artist, album, title)
   - Unknown tags

5. For visualization, it creates a map of downloaded and missing parts of the file.

### Command Line Options
```
Display options:
  -f, --file=FILE      Specify the .part.met file to analyze
  -a, --all            Show all tags (default)
  -s, --special        Show only special tags
  -g, --gap            Show only gap tags
  -t, --standard       Show only standard tags
  -u, --unknown        Show unknown tags

Specific fields (script-friendly, raw output):
  -n, --name           Show filename only
  -S, --size           Show file size only
  -d, --date           Show last seen complete date only
  -p, --progress       Show download progress only
  -e, --hash           Show ED2K hash only
  -m, --metversion     Show .part.met version only (14.0 or 14.1)
  -c, --tagcount       Show number of meta tags only

Output format:
  -j, --json           Output in JSON format

Other options:
  -v, --verbose        Show detailed information
  -V, --version        Show program version
  -z, --visualize      Visualize file download status
  -h, --help           Show this help message
```

### License
This program is provided as-is, without any express or implied warranty.

---

## Latin

### Descriptio
metinfo est instrumentum ad extrahendum "ED2K tesserae" (nexus identificationis) ex "part.met" documentis quae in eDonkey2000/Overnet et eMule systemis communicationis inter pares (P2P) utuntur. Programma automatice formam documenti (14.0 vel 14.1) detegit et extrahit "MD4 tesseram" quae documentum in reti singulariter identificat, atque omnes meta indicationes in documento contentas.

### Installatio
Ad metinfo compilandum et installandum, has instructiones sequere:

```bash
# Codicem fontem cape vel descarga
# Ad directorium projecti naviga
cd metinfo

# Programma compila
make

# Si velis, installa in systemate (privilegia radicis requirit)
sudo make install
```

### Usus
```bash
# Usus fundamentalis (omnes meta indicationes monstrat)
./metinfo -f /via/ad/documentum.part.met

# Exhibe informationem de versione
./metinfo -V

# Exhibe auxilium
./metinfo -h
```

### Usus Pro Scriptis
Pro scriptis, metinfo praebet optiones ad producendum singulos valores sine etiquettis aut formatione:

```bash
# Solum nomen documenti obtine
./metinfo -f /via/ad/documentum.part.met -n
# Exitus: exemplum_pellicula.mkv

# Solum magnitudinem documenti obtine (in octettis)
./metinfo -f /via/ad/documentum.part.met -S
# Exitus: 104857600

# Solum ED2K tesseram obtine
./metinfo -f /via/ad/documentum.part.met -e
# Exitus: E7D81234AB56C890DEF12345ABC67890
```

### Exitus JSON
Omnis informatio potest in forma JSON produci:

```bash
# Plenus exitus JSON
./metinfo -f /via/ad/documentum.part.met -j

# Unius valoris exitus JSON
./metinfo -f /via/ad/documentum.part.met -e -j
# Exitus: {"ed2k_hash":"E7D81234AB56C890DEF12345ABC67890"}
```

### Licentia
Hoc programma "sicut est" praebetur, sine ulla garantia expressa vel implicita.

---

## Italiano

### Descrizione
metinfo è un programma di utilità progettato per estrarre gli hash ED2K dai file .part.met utilizzati dalle reti di condivisione file peer-to-peer eDonkey2000/Overnet ed eMule. Il programma rileva automaticamente la versione del formato del file (14.0 o 14.1) ed estrae l'hash MD4 che identifica in modo univoco il file nella rete, oltre a tutti i meta tag contenuti nel file.

### Installazione
Per compilare e installare metinfo, segui queste istruzioni:

```bash
# Clona o scarica il codice sorgente
# Naviga nella directory del progetto
cd metinfo

# Compila il programma
make

# Opzionalmente installa a livello di sistema (richiede privilegi di root)
sudo make install
```

### Utilizzo
```bash
# Utilizzo base (mostra tutti i meta tag)
./metinfo -f /percorso/al/file.part.met

# Visualizza informazioni sulla versione
./metinfo -V

# Visualizza informazioni di aiuto
./metinfo -h

# Visualizza tipi specifici di tag
./metinfo -f /percorso/al/file.part.met -s  # Mostra solo tag speciali
./metinfo -f /percorso/al/file.part.met -g  # Mostra solo tag gap
./metinfo -f /percorso/al/file.part.met -t  # Mostra solo tag standard
./metinfo -f /percorso/al/file.part.met -u  # Mostra solo tag sconosciuti

# Visualizza lo stato del download
./metinfo -f /percorso/al/file.part.met -z

# Output dettagliato
./metinfo -f /percorso/al/file.part.met -v
```

### Output per Script
Per l'integrazione con script, metinfo fornisce opzioni per produrre valori singoli senza etichette o formattazione:

```bash
# Ottieni solo il nome del file
./metinfo -f /percorso/al/file.part.met -n
# Output: esempio_film.mkv

# Ottieni solo la dimensione del file (in byte)
./metinfo -f /percorso/al/file.part.met -S
# Output: 104857600

# Ottieni solo l'hash ED2K
./metinfo -f /percorso/al/file.part.met -e
# Output: E7D81234AB56C890DEF12345ABC67890

# Ottieni solo la versione del file .part.met
./metinfo -f /percorso/al/file.part.met -m
# Output: 14.0

# Ottieni solo il numero di meta tag
./metinfo -f /percorso/al/file.part.met -c
# Output: 42

# Ottieni solo la percentuale di progresso del download
./metinfo -f /percorso/al/file.part.met -p
# Output: 37.8
```

### Output JSON
Tutte le informazioni possono essere prodotte in formato JSON per una facile integrazione con altri strumenti:

```bash
# Output JSON completo
./metinfo -f /percorso/al/file.part.met -j

# Output JSON di un singolo valore
./metinfo -f /percorso/al/file.part.met -e -j
# Output: {"ed2k_hash":"E7D81234AB56C890DEF12345ABC67890"}
```

### Esempi di Script
```bash
# Verifica se un file è completamente scaricato
if [ $(./metinfo -f /percorso/al/file.part.met -p) = "100.0" ]; then
    echo "Download completato!"
else
    echo "Download in corso..."
fi

# Raccogli informazioni su un download
NOME_FILE=$(./metinfo -f /percorso/al/file.part.met -n)
DIMENSIONE=$(./metinfo -f /percorso/al/file.part.met -S)
PROGRESSO=$(./metinfo -f /percorso/al/file.part.met -p)
HASH=$(./metinfo -f /percorso/al/file.part.met -e)

echo "File: $NOME_FILE"
echo "Dimensione: $DIMENSIONE byte"
echo "Progresso: $PROGRESSO%"
echo "Hash: $HASH"
```

### Opzioni della Linea di Comando
```
Opzioni di visualizzazione:
  -f, --file=FILE      Specifica il file .part.met da analizzare
  -a, --all            Mostra tutti i tag (default)
  -s, --special        Mostra solo i tag speciali
  -g, --gap            Mostra solo i tag gap
  -t, --standard       Mostra solo i tag standard
  -u, --unknown        Mostra i tag sconosciuti

Campi specifici (per script, output grezzo):
  -n, --name           Mostra solo il nome del file
  -S, --size           Mostra solo la dimensione del file
  -d, --date           Mostra solo la data dell'ultima volta visto completo
  -p, --progress       Mostra solo la percentuale di download
  -e, --hash           Mostra solo l'hash ED2K
  -m, --metversion     Mostra solo la versione del file .part.met (14.0 o 14.1)
  -c, --tagcount       Mostra solo il numero di meta tag

Formato di output:
  -j, --json           Output in formato JSON

Altre opzioni:
  -v, --verbose        Mostra informazioni dettagliate
  -V, --version        Mostra la versione del programma
  -z, --visualize      Visualizza lo stato del download
  -h, --help           Mostra questo messaggio di aiuto
```

### Licenza
Questo programma viene fornito così com'è, senza alcuna garanzia espressa o implicita.

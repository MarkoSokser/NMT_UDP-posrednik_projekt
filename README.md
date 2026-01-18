# NMT UDP Posrednik Projekt

![Language](https://img.shields.io/badge/language-C-blue)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)

UDP posrednik (relay) koji omogućava komunikaciju između dva entiteta koji ne mogu komunicirati izravno (npr. oba su iza NAT-a).

## Sadržaj

- [Arhitektura](#arhitektura)
- [Preduvjeti](#preduvjeti)
- [Kompajliranje](#kompajliranje)
- [Pokretanje](#pokretanje)
- [Testiranje](#testiranje)
- [Datoteke](#datoteke)
- [Funkcionalnosti](#funkcionalnosti)
- [Troubleshooting](#troubleshooting)

## Arhitektura

```
┌─────────────────────────────────────────────────────────────────┐
│                        UDP POSREDNIK                            │
│                                                                 │
│   ┌─────────┐         ┌──────────────┐         ┌─────────┐     │
│   │  PEER1  │◄──UDP──►│   SERVER     │◄──UDP──►│  PEER2  │     │
│   │  (NAT)  │         │  :data_port  │         │  (NAT)  │     │
│   └─────────┘         └──────────────┘         └─────────┘     │
│                              ▲                                  │
│                              │                                  │
│                         UDP (ON/OFF/RESET)                      │
│                              │                                  │
│                       ┌──────┴──────┐                          │
│                       │    CTRL     │                          │
│                       │ :ctrl_port  │                          │
│                       └─────────────┘                          │
└─────────────────────────────────────────────────────────────────┘
```

### Kako radi

1. **Registracija** - Dva klijenta šalju pakete na data port posrednika. Posrednik automatski registrira prva dva klijenta kao peer1 i peer2.

2. **Prosljeđivanje** - Kada je forwarding uključen (ON), posrednik prosljeđuje pakete dvosmjerno:
   - peer1 → posrednik → peer2
   - peer2 → posrednik → peer1

3. **Kontrola** - Kontrolni klijent može upravljati posrednikom preko control porta:
   - `ON` - uključuje prosljeđivanje
   - `OFF` - isključuje prosljeđivanje
   - `RESET` - briše registrirane peerove

## Preduvjeti

Za kompajliranje programa potreban je GCC kompajler. Na Ubuntu/Debian sistemima:

```bash
sudo apt update
sudo apt install build-essential
```

Provjera instalacije:
```bash
gcc --version
```

Za testiranje potreban je **Python 3** i **netcat**:
```bash
sudo apt install python3 netcat-openbsd
```

## Kompajliranje

```bash
# Kompajliranje servera
gcc -Wall -o server server.c

# Kompajliranje kontrolnog klijenta
gcc -Wall -o ctrl control_client.c
```

## Pokretanje

### 1. Pokretanje servera

```bash
./server <data_port> <control_port>
```

**Primjer:**
```bash
./server 5000 5001
```

| Parametar | Opis |
|-----------|------|
| `data_port` | Port za promet između klijenata (npr. 5000) |
| `control_port` | Port za kontrolne naredbe (npr. 5001) |

### 2. Kontrola posrednika

```bash
./ctrl <server_ip> <control_port> <naredba>
```

| Naredba | Opis |
|---------|------|
| `ON` | Uključuje prosljeđivanje paketa |
| `OFF` | Isključuje prosljeđivanje paketa |
| `RESET` | Briše registrirane peerove |
| `STATUS` | Vraća trenutno stanje posrednika |

**Primjeri:**
```bash
./ctrl 127.0.0.1 5001 ON      # Uključi prosljeđivanje
./ctrl 127.0.0.1 5001 OFF     # Isključi prosljeđivanje
./ctrl 127.0.0.1 5001 RESET   # Reset peerova
./ctrl 127.0.0.1 5001 STATUS  # Provjeri stanje
```

## Testiranje

### Automatizirani test

```bash
python3 test_forward.py
```

### Ručno testiranje s netcat-om

Otvorite 4 terminala:

```bash
# Terminal 1 - Pokreni server
./server 5000 5001

# Terminal 2 - Uključi forwarding
./ctrl 127.0.0.1 5001 ON

# Terminal 3 - Prvi klijent
nc -u 127.0.0.1 5000

# Terminal 4 - Drugi klijent
nc -u 127.0.0.1 5000
```

Nakon što oba klijenta pošalju barem jedan paket, posrednik će ih registrirati i početi prosljeđivati pakete između njih.

## Datoteke

| Datoteka | Opis |
|----------|------|
| `server.c` | UDP posrednik server |
| `control_client.c` | Kontrolni klijent za upravljanje posrednikom |
| `test_forward.py` | Automatizirani test skript |

## Funkcionalnosti

- Automatska registracija dva peer-a
- Dvosmjerno prosljeđivanje UDP paketa
- Kontrola uključivanja/isključivanja prosljeđivanja
- Reset funkcionalnost za ponovnu registraciju peerova
- STATUS naredba za provjeru stanja posrednika
- Validacija portova (1-65535)
- Ignoriranje nepoznatih izvora (treća strana)

## Troubleshooting

### Port je zauzet

```
bind data: Address already in use
```

**Rješenje:** Koristite drugi port ili pričekajte da se prethodni proces ugasi.

```bash
# Pronađi proces koji koristi port
lsof -i :5000

# Ili koristite drugi port
./server 6000 6001
```

### Paketi se ne prosljeđuju

1. Provjerite je li forwarding uključen:
   ```bash
   ./ctrl 127.0.0.1 5001 ON
   ```

2. Provjerite jesu li oba peera registrirana (trebate vidjeti poruke `[DATA] Registriran peer1/peer2` u output-u servera)

3. Provjerite firewall postavke

### Test neuspješan - timeout

```
TEST NEUSPJEH: prekoračeno vrijeme čekanja za proslijeđenu poruku
```

**Moguća rješenja:**
- Provjerite da portovi 5000 i 5001 nisu zauzeti
- Provjerite da `server` i `ctrl` binarne datoteke postoje

## Napomene

- Posrednik čeka na prvi paket od svakog peera da bi ih registrirao
- Forwarding mora biti uključen (`ON`) da bi paketi bili prosljeđeni
- `RESET` briše informacije o peerovima i omogućava registraciju novih klijenata
- Treći klijent koji pokuša slati pakete će biti ignoriran
- Maksimalna veličina paketa: 1500 bajtova (MTU)

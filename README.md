# NMT UDP Posrednik Projekt

UDP posrednik koji služi kao relay između dva entiteta koja komuniciraju UDP-om, ali ne mogu komunicirati izravno (npr. iza NAT-a).

## Opis projekta

Projekt se sastoji od:
1. **UDP posrednika (server)** - prosljeđuje pakete između dva peer-a
2. **Kontrolnog klijenta** - omogućava uključivanje/isključivanje prosljeđivanja

### Kako radi

1. Dva klijenta šalju pakete na data port posrednika
2. Posrednik automatski registrira prva dva klijenta kao peer1 i peer2
3. Kada je forwarding uključen (ON), posrednik prosljeđuje pakete između njih
4. Kontrolni klijent može uključiti (ON), isključiti (OFF) ili resetirati (RESET) posrednika

## Kompajliranje

### Preduvjeti

Za kompajliranje programa potreban vam je GCC kompajler. Na Ubuntu/Debian sistemima:

```bash
sudo apt update
sudo apt install build-essential
```

Nakon instalacije, provjerite da li je GCC dostupan:
```bash
gcc --version
```

### Kompajliranje programa

```bash
# Kompajliranje servera
gcc -Wall -o server server.c

# Kompajliranje kontrolnog klijenta
gcc -Wall -o ctrl control_client.c
```

## Pokretanje

### 1. Pokretanje servera

```bash
./server 5000 5001
```

Parametri:
- `5000` - data port (port za promet između klijenata)
- `5001` - control port (port za kontrolne naredbe)

### 2. Kontrola posrednika

**Uključivanje prosljeđivanja:**
```bash
./ctrl 127.0.0.1 5001 ON
```

**Isključivanje prosljeđivanja:**
```bash
./ctrl 127.0.0.1 5001 OFF
```

**Reset peerova:**
```bash
./ctrl 127.0.0.1 5001 RESET
```

## Testiranje

Možete koristiti `netcat` (nc) za testiranje:

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

- `server.c` - UDP posrednik server 
- `control_client.c` - Kontrolni klijent za upravljanje posrednikom 
- `test_forward.py` - Automatizirani test skript za provjeru prosljeđivanja

## Funkcionalnosti

-  Automatska registracija dva peer-a
-  Dvosmjerno prosljeđivanje UDP paketa
-  Kontrola uključivanja/isključivanja prosljeđivanja
-  Reset funkcionalnost za ponovnu registraciju peerova

## Napomene

- Posrednik čeka na prvi paket od svakog peera da bi ih registrirao
- Forwarding mora biti uključen (ON) da bi paketi bili prosljeđeni
- RESET briše informacije o peerovima i omogućava registraciju novih klijenata
- Treći klijent koji pokuša slati pakete će biti ignoriran


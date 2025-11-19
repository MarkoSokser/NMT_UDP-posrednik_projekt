#!/usr/bin/env python3
"""Jednostavan automatizirani test prosljeđivanja UDP paketa.

Pokreće izvršnu datoteku `server`, šalje naredbu `ON` preko kontrolnog
klijenta, zatim stvara dva UDP soketa povezana na fiksne lokalne porte
koji simuliraju dva peera. Šalje poruke od peer1 -> server i očekuje
da će peer2 primiti proslijeđenu poruku, a zatim peer2 -> server i
očekuje da će peer1 primiti proslijeđenu poruku.

Kod izlaza: 0 znači uspjeh (TEST USPJEŠAN), različito od 0 znači grešku.
Pokretanje se radi preko komandne linije: `python3 test_forward.py`
"""
import socket
import subprocess
import time
import sys


SERVER_CMD = ['./server', '5000', '5001']
CTRL_CMD = ['./ctrl', '127.0.0.1', '5001', 'ON']

# Portovi na koje ćemo vezati dva simulirana peera
PEER1_PORT = 35303
PEER2_PORT = 40055

def main():
    # pokreni server
    proc = subprocess.Popen(SERVER_CMD, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        time.sleep(0.3)

        # uključi prosljeđivanje (pošalji ON)
        subprocess.run(CTRL_CMD, check=False)
        time.sleep(0.1)

        sock1 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock2 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            sock1.bind(('127.0.0.1', PEER1_PORT))
            sock2.bind(('127.0.0.1', PEER2_PORT))
            sock1.settimeout(2.0)
            sock2.settimeout(2.0)

            # Prvo pošalji male pakete za registraciju s oba peera tako da
            # server registrira peer1 i peer2. Server obično registrira prve
            # dvije različite izvorišne adrese koje vidi.
            sock1.sendto(b'registracija1', ('127.0.0.1', 5000))
            sock2.sendto(b'registracija2', ('127.0.0.1', 5000))
            time.sleep(0.05)

            # Sada pošalji stvarni testni payload: peer1 -> server; peer2
            # bi trebao primiti proslijeđenu poruku.
            sock1.sendto(b'pozdrav od klijenta1', ('127.0.0.1', 5000))
            data, addr = sock2.recvfrom(4096)
            print('peer2 je primio:', data.decode(), 'od', addr)

            # peer2 -> server
            sock2.sendto(b'pozdrav natrag od klijenta2', ('127.0.0.1', 5000))
            data2, addr2 = sock1.recvfrom(4096)
            print('peer1 je primio:', data2.decode(), 'od', addr2)

            print('TEST USPJEŠAN')
            rc = 0
        except socket.timeout:
            print('TEST NEUSPJEH: prekoračeno vrijeme čekanja za proslijeđenu poruku')
            rc = 2
        finally:
            sock1.close()
            sock2.close()

    finally:
        # terminate server and print its output to help debugging
        proc.terminate()
        try:
            out, err = proc.communicate(timeout=1)
        except Exception:
            proc.kill()
            out, err = proc.communicate()
        if out:
            print('--- server stdout ---')
            try:
                print(out.decode())
            except Exception:
                print(out)
        if err:
            print('--- server stderr ---')
            try:
                print(err.decode())
            except Exception:
                print(err)

    sys.exit(rc)


if __name__ == '__main__':
    main()

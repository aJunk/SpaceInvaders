 ____            _      _    _   _                    _              _ _                       
|  _ \ _ __ ___ (_) ___| | _| |_| |__   ___  ___  ___| |__  _ __ ___(_) |__  _   _ _ __   __ _ 
| |_) | '__/ _ \| |/ _ \ |/ / __| '_ \ / _ \/ __|/ __| '_ \| '__/ _ \ | '_ \| | | | '_ \ / _` |
|  __/| | | (_) | |  __/   <| |_| |_) |  __/\__ \ (__| | | | | |  __/ | |_) | |_| | | | | (_| |
|_|   |_|  \___// |\___|_|\_\\__|_.__/ \___||___/\___|_| |_|_|  \___|_|_.__/ \__,_|_| |_|\__, |
              |__/                                                                       |___/ 
Das Spiel „Space Invaders“ soll mit Hilfe von TCP/IP-Sockets implementiert werden.
Die Spiel-Steuerung übernimmt ein Server-Prozess, der auf einem TCP-Port auf neue Spieler wartet.
Verbindet sich ein Client mit dem Server werden sämtliche aktiven Spiele angezeigt und der Client
kann auswählen, zu welchem er als Zuschauer verbunden werden will. Er kann auch ein neues Spiel
starten, bei dem dann andere zuschauen können.
             
Die Spielfeldgröße wird im Zuge der Implementierung definiert. Im unteren Bereich befindet sich
der Spieler. Dieser kann mithilfe der Pfeiltasten bewegt werden, jedoch maximal bis zu der
Trennlinie. Er kann weiters mithilfe der Space-Taste Schüsse abgeben, um Gegner zu zerstören.
Es befinden sich in diesem Bereich außerdem Hindernisse, die nicht durchfahren/durchschossen
werden können (INFO: WURDE IM ZUGE DER IMPLEMENTIERUNG AUS SPIEL-SPASS-GRÜNDEN ENTFERNT, kann
jedoch jederzeit wieder eingeführt werden - Funktion existiert).
Am oberen Spielfeldrand erscheinen stetig in Reihen Gegner, welche zufällig platziert sind.
Alle Gegner bilden ein Feld, das so lange auf eine Seite wandert, bis ein Gegner den Rand berührt.
Dann erfolgt eine Bewegung nach unten und die Gegner bewegen sich in die entgegengesetzte Richtung,
wieder bis der andere Rand berührt wird, usw.
Am oberen Rand wird der eigene Name, der Score (Punkte sammeln durch Abschießen von Gegnern)
und die Anzahl der Leben angezeigt.
			 
Optional: Gegner schießen zu zufälligen Zeiten gezielt auf den Spieler
 _  __                     _ _ _                             
| |/ /___  _ __ ___  _ __ (_) (_) ___ _ __ _   _ _ __   __ _ 
| ' // _ \| '_ ` _ \| '_ \| | | |/ _ \ '__| | | | '_ \ / _` |
| . \ (_) | | | | | | |_) | | | |  __/ |  | |_| | | | | (_| |
|_|\_\___/|_| |_| |_| .__/|_|_|_|\___|_|   \__,_|_| |_|\__, |
                    |_|                                |___/ 

Mithilfe des makefiles:
	make client
	make server
oder händisch kompilieren:
	gcc graphX.c client.c s_c_func.c -o client -std=c99 -Wall -pedantic -lncurses -DNOSOUND
	gcc graphX.c server.c s_c_func.c -o server -std=c99 -Wall -pedantic -lncurses
    _              __ _   _ _                            
   / \  _   _ ___ / _(_) (_) |__  _ __ _   _ _ __   __ _ 
  / _ \| | | / __| |_| | | | '_ \| '__| | | | '_ \ / _` |
 / ___ \ |_| \__ \  _| |_| | | | | |  | |_| | | | | (_| |
/_/   \_\__,_|___/_|  \__,_|_| |_|_|   \__,_|_| |_|\__, |
                                                   |___/ 									
	server [-p <port>]
	client [-i <server ip>] [-p <server port>] [-n <player name>]
	
Der Parameter für die -p Option gibt den TCP Port des Sockets an, über den kommuniziert werden soll.
Der Server soll auf allen möglichen Schnittstellen/IPs lauschen. Beim Client soll zusätzlich mittels -i
Parameter die IP-Adresse des Servers angeben werden können. Sollte dies fehlen, wird 127.0.0.1 verwendet.
Wird kein Port übergeben, soll ein passender, vordefinierter Port verwendet werden, der in Server und
Client gleich ist. Sollte kein Spielername eingegeben werden, wird er anonym angemeldet.
Sollten ungültige Parameter übergeben werden, soll zumindest eine usage message ausgegeben werden.

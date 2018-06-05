#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>	// Wird fuer die Semaphore benoetigt. Diese Bibliothek wird beim Kompilieren eingebunden, indem man den Zusatzparameter -pthread angibt.
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

// Kompilierung: gcc prime_number_calculation_parallel_processes.c -o prime_number_calculation_parallel_processes -pthread
// Ausfuehrung: ./prime_number_calculation_parallel_processes

// Konstanten:
#define ARRAY_SIZE 100000
#define NUMBER_OF_REPEATS 50	// In dieser Variable wird gespeichert, wie oft das Programm hintereinander ausgefuehrt wird, um eine anstaendige Zeitmessung zu gewaehrleisten und Ausreiser auszugleichen.
#define MAXIMUM_NUMBER_OF_PROCESSES 11000	// In dieser Variable wird die maximale Anzahl an Prozessen gespeichert, die im Rahmen dieses Programmes erstellt wird. Auf meinem Rechner lassen sich nicht mehr als 11.000 Prozesse erzeugen. Dies habe ich durch Ausloten der Grenzen festgestellt.

// Struktur:
struct shared_memory_data {
	bool prime_array[ARRAY_SIZE];		// Dieses Array besteht aus Elementen mit den Indizes von 0 bis 99.999. Ein Element des Array wird auf true gesetzt, wenn es sich beim dazugehoerigen Index um eine Primzahl handelt, ansonsten wird das Element auf false gesetzt.
	int current_start_index;
	int current_end_index;
	sem_t semaphore;	// Die Semaphore muss fuer alle Prozesse zugreifbar sein.
	int number_of_calulating_processes;		// Diese Variable wird benoetigt, um den letzten Prozess ausfindig machen zu koennen.
};

// Funktions-Deklarationen:
void initialize_prime_array(bool prime_array[]);
void find_prime_numbers_parallel_processes(int number_of_processes, struct shared_memory_data * data);
void process_function(bool prime_array[], int start_index, int end_index);
bool check_if_calculation_is_right(bool prime_array[]);

// Funktionen:
int main(int argc, char *argv[]) {
	double average_measured_time = 0;	// Durchschnittlich gemessene Zeit.
	bool all_calculations_right = true;	// Diese Variable speichert, ob alle Berechnungen tatsaechlich die Primzahlen berechnet haben. Die Ueberpruefung geschieht mittels einer Datei, in welcher die Primzahlen von 1 bis 100.000 enthalten sind.
	long number_of_processes = 1;	// Anzahl von eingesetzten Prozesse im jeweiligen Durchlauf.
	long seconds, useconds;		// Benoetigt fuer die Zeitmessung.
	struct timeval begin, end;	// Benoetigt fuer die Zeitmessung.
	FILE * file_ptr;	// Wird fuer das Auslesen der tatsaechlichen Primzahlen aus der Datei benoetigt.
	int i;

	printf("Parallele Berechnung der Primzahlen von 1 bis 100.000 mit Hilfe von Prozessen und %d Wiederholung(en):\n", NUMBER_OF_REPEATS);

	file_ptr = fopen("./prime_number_calculation_parallel_processes_csv.csv", "w");		// In diese Datei werden die Ergebnisse der Zeitmessung geschrieben.

	// Tabellen-Kopf der CSV-Datei erstellen:
	fprintf(file_ptr, "Anzahl von Prozessen;");

	for (i = 1; i <= NUMBER_OF_REPEATS; i++) {
		fprintf(file_ptr, "%d. Berechnung [s];", i);
	}

	fprintf(file_ptr, "Durchschnittswert [s]\n");
	fflush(file_ptr);		// Mit dieser Funktion werden die dem Datei-Stream zugeordneten Puffer geleert. Dies ist in diesem Zusammenhang wichtig, da ansonsten Inhalte mehrfach in der Datei gespeichert werden, welche nur ein Mal dort enthalten sein sollten.

	// Da einzelne Prozesse im Gegensatz zu Threads keinen gemeinsamen Adressraum besitzen, muss ein gemeinsamer Speicherbereich, ein sogenanntes Shared-Memory-Segment, eingerichtet werden, welches sich dann alle Prozesse teilen.

	int shmid;	// Shared-Memory-ID, mit der spaeter auf das Shared-Memory-Segment zugegriffen werden kann.
	key_t key = ftok(".", 'x');	// Schluessel, der zur eindeutigen Identifizierung des Shared-Memory-Segments intern fuer UNIX dient. Meist handelt es sich hierbei um einen Integer-Wert. Die Funktion ftok() dient zur Generierung dieses Schluessels.
	int size = sizeof(struct shared_memory_data);	// Groesse des neu anzulegenden Shared-Memory-Segments.
	int shmflg = IPC_CREAT | 0777;	// Mit der Variable shmflg werden die Zugriffsprivilegien vergeben. Zugriffsrechte: 0777 bedeutet Vollzugriff fuer alle.

	shmid = shmget(key, size, shmflg);	// Nun wird das Shared-Memory-Segment angelegt und dessen Shared-Memory-ID zurueckgegeben und in der Variable shmid gespeichert.

	// Nun muss das Shared-Memory-Segment an den Adressraum des Prozesses angebunden (attach) werden. Beim Aufruf von fork() wird das angebundene Shared-Memory-Segment auch an den Kindprozess weitervererbt.

	struct shared_memory_data * data = (struct shared_memory_data *) shmat(shmid, NULL, 0);		// Diese Funktion gibt einen Pointer auf das Shared-Memory-Segment zurueck, ueber welchen man dann auf das Segment zugreifen kann. Allerdings gibt die Funktionen einen generischen Pointer (vom Typ void *) zurueck, der zuerst noch gecastet werden muss. Mit dem zweiten Parameter koennte die Adresse festgelegt werden, an welcher im Shared-Memory-Segment man sich anbinden will. Aus Portabilitaetsgruenden sollte man dies dem Kernel ueberlassen und hierfuer NULL uebergeben. Wird beim dritten Parameter das Flag SHM_RDONLY angegeben, so wird das Segment nur zum Lesen angebunden, ansonsten auch zum Schreiben.

	// Da der Zugriff auf einen gemeinsamen Speicher bei mehreren Prozessen zu einem zeitkritischen Ablauf fuehren kann, muessen Synchronisationsmechanismen eingebaut werden. Dies geschieht in diesem Fall anhand einer Semaphore.

	sem_init(&(data->semaphore), 1, 1);	// Diese Funktion initialisiert eine Semaphore, deren Adresse als erster Parameter uebergeben wird. Ist der zweite Parameter ungleich 0, so wird die Semaphoren zwischen mehreren Prozessen teilbar, ansonsten wird die Semaphore nur zwischen den Threads des Prozesses, in dem sie erstellt wurde, geteilt. Der dritte Parameter ist der Wert, mit dem die Semaphore initialisiert wird. Da sie nur einem Prozess gleichzeitig den Zugriff auf das Shared-Memory-Segment erlauben soll, wird dieser Parameter auf 1 gesetzt.

	while (number_of_processes <= MAXIMUM_NUMBER_OF_PROCESSES) {
		average_measured_time = 0;
		bool calculations_right_specific_number_of_processes = true;

		printf("\tBerechnung mittels %ld Prozess(en):\n", number_of_processes);
		fprintf(file_ptr, "%ld;", number_of_processes);
		fflush(file_ptr);		// Mit dieser Funktion werden die dem Datei-Stream zugeordneten Puffer geleert. Dies ist in diesem Zusammenhang wichtig, da ansonsten Inhalte mehrfach in der Datei gespeichert werden, welche nur ein Mal dort enthalten sein sollten.

		for (i = 1; i <= NUMBER_OF_REPEATS; i++) {
			initialize_prime_array(data->prime_array);
			data->number_of_calulating_processes = 0;

			if (gettimeofday(&begin, (struct timezone *) 0)) {	// Zeitmessung starten.
				fprintf(stderr, "Fehler beim Zeitmessen.\n");
				return EXIT_FAILURE;
			}

			find_prime_numbers_parallel_processes(number_of_processes, data);
		
			if (gettimeofday(&end, (struct timezone *) 0)) {	// Zeitmessung beenden.
				fprintf(stderr, "Fehler beim Zeitmessen.\n");
				return EXIT_FAILURE;
			}

			// Laufzeit-Berechnung:
			seconds = end.tv_sec - begin.tv_sec;		// Sekunden, die das Programm zur Ausfuehrung benoetigt hat, berechnen.
			useconds = end.tv_usec - begin.tv_usec;		// Mikro-Sekunden, die das Programm zur Ausfuehrung benoetigt hat, berechnen.
			if (useconds < 0) {
				useconds += 1000000;	// Mikro-Sekunden muessen immer als positive Zahl angegeben werden.
				seconds--;
			}

			double measured_time = seconds + (((double) useconds) / 1000000);
			average_measured_time = average_measured_time + measured_time;

			printf("\t\t%d. Berechnung: %f s. ", i, measured_time);
			fprintf(file_ptr, "%f;", measured_time);
			fflush(file_ptr);		// Mit dieser Funktion werden die dem Datei-Stream zugeordneten Puffer geleert. Dies ist in diesem Zusammenhang wichtig, da ansonsten Inhalte mehrfach in der Datei gespeichert werden, welche nur ein Mal dort enthalten sein sollten.

			// Ueberpruefen, ob die berechneten Primzahlen korrekt sind:
			if (!check_if_calculation_is_right(data->prime_array)) {
				calculations_right_specific_number_of_processes = false;
				printf("Die %d. Berechnung der Primzahlen mit %ld Prozess(en) weist Fehler auf.\n", i, number_of_processes);
			} else {
				printf("Die %d. Berechnung der Primzahlen mit %ld Prozess(en) ist korrekt.\n", i, number_of_processes);
			}
		}
		
		average_measured_time = average_measured_time / NUMBER_OF_REPEATS;

		printf("\t\tDurchschnittliche Ausfuehrungszeit des Programmes bei %d Durchlaeufen mit %ld Prozess(en): %f s. ", NUMBER_OF_REPEATS, number_of_processes, average_measured_time);
		fprintf(file_ptr, "%f\n", average_measured_time);
		fflush(file_ptr);		// Mit dieser Funktion werden die dem Datei-Stream zugeordneten Puffer geleert. Dies ist in diesem Zusammenhang wichtig, da ansonsten Inhalte mehrfach in der Datei gespeichert werden, welche nur ein Mal dort enthalten sein sollten.

		// Ueberpruefen, ob alle berechneten Primzahlen mit der zum jeweiligen Zeitpunkt gewaehlten Anzahl an Threads korrekt sind:
		if (calculations_right_specific_number_of_processes) {
			printf("Alle berechneten Primzahlen mit %ld Prozess(en) sind korrekt.\n",  number_of_processes);
		} else {
			printf("Die berechneten Primzahlen mit %ld Prozess(en) weisen Fehler auf.\n", number_of_processes);
			all_calculations_right = false;
		}

		// Anzahl von Prozessen erhoehen:
		if (number_of_processes < 10) {
			number_of_processes = number_of_processes + 1;
		} else if (number_of_processes < 50) {
			number_of_processes = number_of_processes + 2;
		} else if (number_of_processes < 1000) {
			number_of_processes = number_of_processes + 50;
		} else {
			number_of_processes = number_of_processes + 1000;
		}
	}

	// Ueberpruefen, ob alle berechneten Primzahlen korrekt sind.
	if (all_calculations_right) {
		printf("Alle berechneten Primzahlen sind korrekt.\n");
	} else {
		printf("Die berechneten Primzahlen weisen Fehler auf.\n");
	}

	sem_destroy(&(data->semaphore));	// Die Semaphore kann geloescht und damit zerstoert werden, da sie nicht mehr benoetigt wird.

	shmctl(shmid, IPC_RMID, NULL);		// Das Shared-Memory-Segment wird geloescht, da es ebenfalls nicht mehr benoetigt wird. Der erste Parameter ist die ID des zu loeschenden Shared-Memory-Segments. Der zweite Parameter erlaubt die Ausfuehrung eines Kommandos auf dem Segment, in diesem Fall IPC_RMID zum Loeschen eines Shared-Memory-Segments, aber auch koennen zum Beispiel mit IPC_INFO (nur unter Linux) Informationen eines Shared-Memory-Segments erfragt werden. Das dritte Argument kann in der Regel mit NULL belegt werden, da es einen Pointer auf eine shmid_ds-Struktur darstellt, welche nur in seltenen Faellen benoetigt wird.

	fclose(file_ptr);

	return EXIT_SUCCESS;
}

void initialize_prime_array(bool prime_array[]) {
	int i;

	for (i = 0; i < ARRAY_SIZE; i++) {
		prime_array[i] = true;
	}
}

void find_prime_numbers_parallel_processes(int number_of_processes, struct shared_memory_data * data) {
	// Diese Funktion teilt die Arbeit auf die verschiedenen Prozesse auf. Fuer die Parallelisierung werden UNIX-Prozesse verwendet.

	pid_t pid[number_of_processes];
	int diffenence = ARRAY_SIZE / number_of_processes;	// Anzahl von Indizes, die jeder Prozess abarbeiten muss. Der letzte Prozess muss den Rest abarbeiten, der uebrig bleibt, da die Rechnung nicht immer restlos ist.
	long i = 0;

	data->current_start_index = 0;
	data->current_end_index = data->current_start_index + diffenence - 1;

	for (i = 0; i < number_of_processes; i++) {
		pid[i] = fork();

		// Der Elternprozess (mit pid[i] > 0) fuehrt keine Aufgabe aus, sondern erstellt nur die Kindprozesse, die die Arbeit leisten, und wartet dann auf deren Terminierung.

		if (pid[i] == 0) {	// Kindprozess
			sem_wait(&(data->semaphore));	// Eintritt in den kritischen Bereich. Von nun an kann auf das Shared-Memory-Segment problemlos zugegriffen werden.

			data->number_of_calulating_processes++;
			int start_index = data->current_start_index;
			int end_index = data->current_end_index;

			data->current_start_index = data->current_end_index + 1;
		
			if (data->number_of_calulating_processes == (number_of_processes - 1)) {	// Die Variable i ist in diesem Fall nicht immer gleich number_of_calulating_processes, weil i eine lokale Variable ist, deren Wert beim Fork kopiert wird und sich bis zum Erreichen dieser Code-Zeile im Vaterprozess schon wieder geaendert haben kann, ohne, dass die Kindprozesse davon erfahren haben, da diese Variable nur lokal fuer jeden Prozess existiert. Die Variable number_of_calulating_processes hingegen befindet sich im Shared-Memory-Segment und enthaelt deshalb auch immer die aktuelle Anazhl an rechnenden Prozessen.
				data->current_end_index = ARRAY_SIZE - 1;		// Letzter Prozess arbeitet den Rest ab.
			} else {
				data->current_end_index = data->current_start_index + diffenence - 1;
			}

			sem_post(&(data->semaphore));	// Verlassen des kritischen Bereiches.

			process_function(data->prime_array, start_index, end_index);		// Dieser Aufruf ist nicht Teil des kritischen Bereichs, da ansonsten keine Parallelitaet moeglich waere und die Prozesse alle auf verschiedenen Teilen des Arrays arbeiten, sodass es hier zu keinen Problemen kommt.

			exit(EXIT_SUCCESS);		// Kindprozess beenden.
		} else if (pid < 0) {	// Ist pid negativ, so ist ein Fehler aufgetreten. Somit ist die maximale Anzahl an erzeugbaren Prozessen erreicht.
			number_of_processes = i;	// Durch Gleichsetzen dieser beiden Variablen wird die Schleifen-Fortsetzungsbedingung falsch. In diese if-Abzweigung kommt nur der Vaterprozess hinein, da wegen des Fehlers kein Kindprozess erstellt werden konnte.
		}
	}

	for (i = 0; i < number_of_processes; i++) {
		wait(NULL);	// Der Vaterprozess wartet nun auf die Terminierung seiner Kinprozesse.
	}
}

void process_function(bool prime_array[], int start_index, int end_index) {
	// Diese Funktion berechnet die Primzahlen.

	int i;
	int j;

	for (i = start_index; i <= end_index; i++) {
		if (i == 0) {
			prime_array[0] = false;		// 0 ist per Definition keine Primzahl.
		} else if (i == 1) {
			prime_array[1] = false;		// 1 ist per Definition keine Primzahl.
		} else {
			for (j = 2; j < i; j++) {
				if (i % j == 0) {
					prime_array[i] = false;
				}
			}
		}
	}
}

bool check_if_calculation_is_right(bool prime_array[]) {
	// Diese Funktion vergleicht die berechneten Primzahlen mit einer Datei, welche alle Primzahlen zwischen 1 und 100.000 enthaelt. Somit wird die Korrektheit der Berechnungen ueberprueft.

	FILE * file_ptr;
	int string_size = 7;	// Die groesste Zahl in der Datei ist 99991, enthaelt also 5 Zeichen. Mitsamt des Newline-Characters '\n', welcher von fgets() standardmaessig mit eingelesen wird, wird also die String-Groesse von 7 Zeichen nie ueberschritten.
	char read_prime_number[string_size];		// Aus der Datei ausgelesene Primzahl.
	bool calculation_is_right = true;
	long i;

	file_ptr = fopen("./prime_numbers_1_to_100.000.txt", "r");

	for (i = 2; i < ARRAY_SIZE; i++) {
		if (prime_array[i] == true) {
			fgets(read_prime_number, string_size, file_ptr);		// If this function encounters a newline character '\n' or the end of the file EOF before they have read the maximum number of characters, then it returns only the characters read up to that point including the new line character.

			// Nun muss der Newline-Characters '\n', welcher von fgets() standardmaessig mit eingelesen wird, entfernt werden.

			int j;
			for (j = 0; j < string_size; j++) {
				if (read_prime_number[j] == '\n') {
					read_prime_number[j] = '\0';
				}
			}

			// Nun muss der Index i, welcher die aktuelle Primzahl darstellt, in einen String umgewandelt werden.
			
			char i_as_string[string_size];
			sprintf(i_as_string, "%ld", i);

			if (strcmp(i_as_string, read_prime_number) != 0) {		// Falls die Funktion strcmp() nicht 0 zurueckgibt, so sind die beiden Strings nicht gleich.
				calculation_is_right = false;
			}
		}
	}

	fclose(file_ptr);

	return calculation_is_right;
}

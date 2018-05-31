#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

// Kompilierung: gcc prime_number_calculation_parallel_threads.c -o prime_number_calculation_parallel_threads -pthread
// Ausfuehrung: ./prime_number_calculation_parallel_threads

// Konstanten:
#define ARRAY_SIZE 100000
#define NUMBER_OF_REPEATS 50	// In dieser Variable wird gespeichert, wie oft das Programm hintereinander ausgefuehrt wird, um eine anstaendige Zeitmessung zu gewaehrleisten und Ausreiser auszugleichen.
#define MAXIMUM_NUMBER_OF_THREADS 32000	// In dieser Variable wird die maximale Anzahl an Threads gespeichert, die im Rahmen dieses Programmes erstellt wird. Auf meinem Rechner lassen sich nicht mehr als 32.000 Threads erzeugen. Dies habe ich durch Ausloten der Grenzen festgestellt.

// Funktions-Deklarationen:
void initialize_prime_array();
void find_prime_numbers_parallel_threads(long number_of_threads);
void * thread_function(void * arg);
bool check_if_calculation_is_right();

// Struktur:
struct data {
	long start_index;	// Start-Index fuer die Berechnung
	long end_index;	// End-Index fuer die Berechnung
};

// Globale Variablen:
bool prime_array[ARRAY_SIZE];		// Dieses Array besteht aus Elementen mit den Indizes von 0 bis 99.999. Ein Element des Array wird auf true gesetzt, wenn es sich beim dazugehoerigen Index um eine Primzahl handelt, ansonsten wird das Element auf false gesetzt.

// Funktionen:
int main(int argc, char *argv[]) {
	double average_measured_time = 0;	// Durchschnittlich gemessene Zeit.
	bool all_calculations_right = true;	// Diese Variable speichert, ob alle Berechnungen tatsaechlich die Primzahlen berechnet haben. Die Ueberpruefung geschieht mittels einer Datei, in welcher die Primzahlen von 1 bis 100.000 enthalten sind.
	long number_of_threads = 1;	// Anzahl von eingesetzten Threads im jeweiligen Durchlauf.
	long seconds, useconds;		// Benoetigt fuer die Zeitmessung.
	struct timeval begin, end;	// Benoetigt fuer die Zeitmessung.
	FILE * file_ptr;	// Wird fuer das Auslesen der tatsaechlichen Primzahlen aus der Datei benoetigt.
	int i;

	printf("Parallele Berechnung der Primzahlen von 1 bis 100.000 mit Hilfe von Threads und %d Wiederholung(en):\n", NUMBER_OF_REPEATS);

	file_ptr = fopen("./prime_number_calculation_parallel_threads_csv.csv", "w");		// In diese Datei werden die Ergebnisse der Zeitmessung geschrieben.

	// Tabellen-Kopf der CSV-Datei erstellen:
	fprintf(file_ptr, "Anzahl von Threads;");

	for (i = 1; i <= NUMBER_OF_REPEATS; i++) {
		fprintf(file_ptr, "%d. Berechnung [s];", i);
	}

	fprintf(file_ptr, "Durchschnittswert [s]\n");

	while (number_of_threads <= MAXIMUM_NUMBER_OF_THREADS) {
		average_measured_time = 0;
		bool calculations_right_specific_number_of_threads = true;

		printf("\tBerechnung mittels %ld Thread(s):\n", number_of_threads);
		fprintf(file_ptr, "%ld;", number_of_threads);

		for (i = 1; i <= NUMBER_OF_REPEATS; i++) {
			initialize_prime_array();

			if (gettimeofday(&begin, (struct timezone *) 0)) {	// Zeitmessung starten.
				fprintf(stderr, "Fehler beim Zeitmessen.\n");
				return EXIT_FAILURE;
			}

			find_prime_numbers_parallel_threads(number_of_threads);
		
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

			// Ueberpruefen, ob die berechneten Primzahlen korrekt sind:
			if (!check_if_calculation_is_right()) {
				calculations_right_specific_number_of_threads = false;
				printf("Die %d. Berechnung der Primzahlen mit %ld Thread(s) weist Fehler auf.\n", i, number_of_threads);
			} else {
				printf("Die %d. Berechnung der Primzahlen mit %ld Thread(s) ist korrekt.\n", i, number_of_threads);
			}
		}
		
		average_measured_time = average_measured_time / NUMBER_OF_REPEATS;

		printf("\t\tDurchschnittliche Ausfuehrungszeit des Programmes bei %d Durchlaeufen mit %ld Thread(s): %f s. ", NUMBER_OF_REPEATS, number_of_threads, average_measured_time);

		fprintf(file_ptr, "%f\n", average_measured_time);

		// Ueberpruefen, ob alle berechneten Primzahlen mit der zum jeweiligen Zeitpunkt gewaehlten Anzahl an Threads korrekt sind:
		if (calculations_right_specific_number_of_threads) {
			printf("Alle berechneten Primzahlen mit %ld Thread(s) sind korrekt.\n",  number_of_threads);
		} else {
			printf("Die berechneten Primzahlen mit %ld Thread(s) weisen Fehler auf.\n", number_of_threads);
			all_calculations_right = false;
		}

		// Anzahl von Threads erhoehen:
		if (number_of_threads < 10) {
			number_of_threads = number_of_threads + 1;
		} else if (number_of_threads < 50) {
			number_of_threads = number_of_threads + 2;
		} else if (number_of_threads < 1000) {
			number_of_threads = number_of_threads + 50;
		} else {
			number_of_threads = number_of_threads + 1000;
		}
	}

	// Ueberpruefen, ob alle berechneten Primzahlen korrekt sind.
	if (all_calculations_right) {
		printf("Alle berechneten Primzahlen sind korrekt.\n");
	} else {
		printf("Die berechneten Primzahlen weisen Fehler auf.\n");
	}

	fclose(file_ptr);

	return EXIT_SUCCESS;
}

void initialize_prime_array() {
	int i;

	for (i = 0; i < ARRAY_SIZE; i++) {
		prime_array[i] = true;
	}
}

void find_prime_numbers_parallel_threads(long number_of_threads) {
	// Diese Funktion teilt die Arbeit auf die verschiedenen Threads auf. Fuer die Parallelisierung werden POSIX-Threads verwendet.

	pthread_t threads[number_of_threads];
	struct data * thread_data[number_of_threads];
	long diffenence = ARRAY_SIZE / number_of_threads;	// Anzahl von Indizes, die jeder Thread abarbeiten muss. Der letzte Thread muss den Rest abarbeiten, der uebrig bleibt, da die Rechnung nicht immer restlos ist.
	long current_start_index;
	long current_end_index;
	long i = 0;

	current_start_index = 0;

	for (i = 0; i < number_of_threads; i++) {
		if (i == (number_of_threads - 1)) {
			current_end_index = ARRAY_SIZE - 1;		// Letzter Thread arbeitet den Rest ab.
		} else {
			current_end_index = current_start_index + diffenence - 1;
		}

		thread_data[i] = (struct data *) malloc(sizeof(struct data));
		thread_data[i]->start_index = current_start_index;
		thread_data[i]->end_index = current_end_index;

		if(pthread_create(&threads[i], NULL, &thread_function, thread_data[i]) != 0) {
			// Maximale Anzahl an erzeugbaren Threads ist erreicht.
			number_of_threads = i;	// Durch Gleichsetzen dieser beiden Variablen wird die Schleifen-Fortsetzungsbedingung falsch.
		}

		current_start_index = current_end_index + 1;
	}

	for (i = 0; i < number_of_threads; i++) {
		pthread_join(threads[i], NULL);
		free(thread_data[i]);
	}
}

void * thread_function(void * arg) {
	// Diese Funktion berechnet die Primzahlen.

	struct data * thread_data = (struct data *) arg;
	int i;
	int j;

	for (i = thread_data->start_index; i <= thread_data->end_index; i++) {
		if (i == 0) {
			prime_array[i] = false;		// 0 ist per Definition keine Primzahl.
		} else if (i == 1) {
			prime_array[i] = false;		// 1 ist per Definition keine Primzahl.
		} else {
			for (j = 2; j < i; j++) {
				if (i % j == 0) {
					prime_array[i] = false;
				}
			}
		}
	}

	pthread_exit(NULL);
}

bool check_if_calculation_is_right() {
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

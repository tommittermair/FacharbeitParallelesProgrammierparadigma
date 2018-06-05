#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <string.h>
#include <omp.h>

// Kompilierung: gcc prime_number_calculation_parallel_openmp.c -o prime_number_calculation_parallel_openmp -fopenmp
// Ausfuehrung: ./prime_number_calculation_parallel_openmp

// Konstanten:
#define ARRAY_SIZE 100000
#define NUMBER_OF_REPEATS 50	// In dieser Variable wird gespeichert, wie oft das Programm hintereinander ausgefuehrt wird, um eine anstaendige Zeitmessung zu gewaehrleisten und Ausreiser auszugleichen.
#define MAXIMUM_NUMBER_OF_THREADS 11000	// In dieser Variable wird die maximale Anzahl an OpenMP-Threads gespeichert, die im Rahmen dieses Programmes erstellt wird. Auf meinem Rechner lassen sich nicht mehr als 11.000 Threads mit OpenMP erzeugen. Dies habe ich durch Ausloten der Grenzen festgestellt.

// Funktions-Deklarationen:
void initialize_prime_array(bool prime_array[]);
void find_prime_numbers_parallel_threads(bool prime_array[], long number_of_threads);
bool check_if_calculation_is_right(bool prime_array[]);

// Funktionen:
int main(int argc, char *argv[]) {
	bool prime_array[ARRAY_SIZE];		// Dieses Array besteht aus Elementen mit den Indizes von 0 bis 99.999. Ein Element des Array wird auf true gesetzt, wenn es sich beim dazugehoerigen Index um eine Primzahl handelt, ansonsten wird das Element auf false gesetzt.
	double average_measured_time = 0;	// Durchschnittlich gemessene Zeit.
	bool all_calculations_right = true;	// Diese Variable speichert, ob alle Berechnungen tatsaechlich die Primzahlen berechnet haben. Die Ueberpruefung geschieht mittels einer Datei, in welcher die Primzahlen von 1 bis 100.000 enthalten sind.
	long number_of_threads = 1;	// Anzahl von eingesetzten Threads im jeweiligen Durchlauf.
	long seconds, useconds;		// Benoetigt fuer die Zeitmessung.
	struct timeval begin, end;	// Benoetigt fuer die Zeitmessung.
	FILE * file_ptr;	// Wird fuer das Auslesen der tatsaechlichen Primzahlen aus der Datei benoetigt.
	int i;

	printf("Parallele Berechnung der Primzahlen von 1 bis 100.000 mit Hilfe von OpenMP und %d Wiederholung(en):\n", NUMBER_OF_REPEATS);

	file_ptr = fopen("./prime_number_calculation_parallel_openmp_csv.csv", "w");		// In diese Datei werden die Ergebnisse der Zeitmessung geschrieben.

	// Tabellen-Kopf der CSV-Datei erstellen:
	fprintf(file_ptr, "Anzahl von OpenMP-Threads;");

	for (i = 1; i <= NUMBER_OF_REPEATS; i++) {
		fprintf(file_ptr, "%d. Berechnung [s];", i);
	}

	fprintf(file_ptr, "Durchschnittswert [s]\n");

	while (number_of_threads <= MAXIMUM_NUMBER_OF_THREADS) {
		average_measured_time = 0;
		bool calculations_right_specific_number_of_threads = true;

		printf("\tBerechnung mittels %ld OpenMP-Thread(s):\n", number_of_threads);
		fprintf(file_ptr, "%ld;", number_of_threads);

		for (i = 1; i <= NUMBER_OF_REPEATS; i++) {
			initialize_prime_array(prime_array);

			if (gettimeofday(&begin, (struct timezone *) 0)) {	// Zeitmessung starten.
				fprintf(stderr, "Fehler beim Zeitmessen.\n");
				return EXIT_FAILURE;
			}

			find_prime_numbers_parallel_threads(prime_array, number_of_threads);
		
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
			if (!check_if_calculation_is_right(prime_array)) {
				calculations_right_specific_number_of_threads = false;
				printf("Die %d. Berechnung der Primzahlen mit %ld OpenMP-Thread(s) weist Fehler auf.\n", i, number_of_threads);
			} else {
				printf("Die %d. Berechnung der Primzahlen mit %ld OpenMP-Thread(s) ist korrekt.\n", i, number_of_threads);
			}
		}
		
		average_measured_time = average_measured_time / NUMBER_OF_REPEATS;

		printf("\t\tDurchschnittliche Ausfuehrungszeit des Programmes bei %d Durchlaeufen mit %ld OpenMP-Thread(s): %f s. ", NUMBER_OF_REPEATS, number_of_threads, average_measured_time);

		fprintf(file_ptr, "%f\n", average_measured_time);

		// Ueberpruefen, ob alle berechneten Primzahlen mit der zum jeweiligen Zeitpunkt gewaehlten Anzahl an Threads korrekt sind:
		if (calculations_right_specific_number_of_threads) {
			printf("Alle berechneten Primzahlen mit %ld OpenMP-Thread(s) sind korrekt.\n",  number_of_threads);
		} else {
			printf("Die berechneten Primzahlen mit %ld OpenMP-Thread(s) weisen Fehler auf.\n", number_of_threads);
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

void initialize_prime_array(bool prime_array[]) {
	int i;

	for (i = 0; i < ARRAY_SIZE; i++) {
		prime_array[i] = true;
	}
}

void find_prime_numbers_parallel_threads(bool prime_array[], long number_of_threads) {
	// Diese Funktion teilt die Arbeit auf die verschiedenen OpenMP-Threads auf und berechnet die Primzahlen.

	int i;
	int j;

	// 0 und 1 sind per Definition keine Primzahlen.
	prime_array[0] = false;
	prime_array[1] = false;

	/*
		OpenMP uebernimmt die Aufteilung der Iterationen von Schleifen automatisch, der Programmierer muss nur die Compiler-Direktiven setzen.

		Erklaerung der angefuehrten Klauseln der Compiler-Direktiven:
		 - shared(): Die Daten werden unter allen parallelen Regionen geshared, d.h. sie sind in allen Threads les- und schreibbar. Wenn ein Thread eine Variable ändert, ist sie auch in allen anderen Threads modifiziert.
		 - private(): Jeder Thread bekommt eine private Kopie der Variablen, diese wird nicht initialisiert.
		 - num_threads(): Legt die Anzahl von Threads fest, die verwendet erzeugt werden.
	*/

	#pragma omp parallel num_threads(number_of_threads) private(i, j) shared(prime_array)
	{
		#pragma omp for
		for (i = 2; i < ARRAY_SIZE; i++) {
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

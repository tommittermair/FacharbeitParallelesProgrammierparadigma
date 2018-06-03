#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <mpi.h>

// Fuer die Kompilierung und Ausfuehrung muss eine MPI-Implementierung, zum Beispiel OpenMPI, auf dem Rechner installiert werden. Anleitung zur Installation von OpenMPI: https://ireneli.eu/2016/02/15/installation/

// Kompilierung: mpicc -o prime_number_calculation_parallel_mpi prime_number_calculation_parallel_mpi.c
// Ausfuehrung: mpirun -np <Anzahl von zu erstellenden Prozessen> prime_number_calculation_parallel_mpi

// Konstanten:
#define ARRAY_SIZE 100000
#define MASTER_RANK 0
#define START_INDEX 0
#define END_INDEX 1
#define TRUE 1
#define FALSE 0

void initialize_prime_array(int prime_array[], int length);
void find_prime_numbers_parallel_mpi(int prime_array[], long number_of_mpi_slave_processes);
bool check_if_calculation_is_right(int prime_array[]);

void main(int argc, char *argv[]) {
	int size;	// In dieser Variable wird die Anzahl der erzeugten Prozesse gespeichert.
	int my_rank;	// In dieser Variable wird eine (fortlaufende) Zahl gespeichert, mit welcher die erzeugten Prozesse identifiziert werden koennen. Der Rank eines Prozesses bewegt sich immer im Intervall von 0 bis (size - 1).
	bool calculations_right = true;

	MPI_Init(&argc, &argv);		// MPI wird initialisiert. Von diesem Punkt an koennen Funktionen der MPI-Bibliothek verwendet werden.
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

	printf("I am process %d out of %d\n", my_rank, size);


	if (my_rank == MASTER_RANK) {	// Der 0. Prozess wird zum Master-Prozess auserwaehlt, welcher die Arbeit auf die anderen Prozesse und die Teilloesungen wieder zu einer gesamten Loesung des Problems kombiniert.
	
		int prime_array[ARRAY_SIZE];		// Dieses Array besteht aus Elementen mit den Indizes von 0 bis 99.999. Ein Element des Array wird auf true gesetzt, wenn es sich beim dazugehoerigen Index um eine Primzahl handelt, ansonsten wird das Element auf false gesetzt.
		int current_prime_array_index = 0;
		long number_of_mpi_slave_processes = size - 1;


		double t1; 
		t1 = MPI_Wtime();	// Returns the time in seconds since an arbitrary time in the past. 

		

		find_prime_numbers_parallel_mpi(prime_array, number_of_mpi_slave_processes);		// Nur (size - 1) MPI-Porzesse koennen fuer die Berechnung genutzt werden, da der Master-Prozess selbst keine Berechnungen durchfuehrt.
	
		





		long diffenence = ARRAY_SIZE / number_of_mpi_slave_processes;	// Anzahl von Indizes, die jeder Thread abarbeiten muss. Der letzte Thread muss den Rest abarbeiten, der uebrig bleibt, da die Rechnung nicht immer restlos ist.
		long current_start_index;
		long current_end_index;
		long i = 0;

		current_start_index = 0;

		for (i = 1; i <= number_of_mpi_slave_processes; i++) {
			if (i == number_of_mpi_slave_processes) {
				current_end_index = ARRAY_SIZE - 1;		// Letzter Thread arbeitet den Rest ab.
			} else {
				current_end_index = current_start_index + diffenence - 1;
			}
		
			


			long part_array_size = (current_end_index - current_start_index) + 1;		// Der Puffer, der gesendet wird, soll zwei long-Variablen enthalten.
			int * part_prime_array = (int *) malloc(part_array_size * sizeof(int));		// Diese Nachricht wird an die jeweiligen Prozesse uebermittelt. Sie besteht aus 2 Longs, wobei der erste Long der Start-Index fuer die Berechnung und der zweite Long der End-Index fuer die Berechnung ist.


			MPI_Status status;

			MPI_Recv(part_prime_array, part_array_size, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

			printf("Master: Nachricht von %ld erhalten.\n", i);

			/*
				int MPI Recv(void *rmessage,
				int count,
				MPI Datatype datatype,
				int source,
				int tag,
				MPI Comm comm,
				MPI Status *status)

				This operation is blocking. The parameters have the following meaning:
				 - rmessage specifies the receive buffer in which the message should be stored;
				 - count is the maximum number of elements that should be received;
				 - datatype is the data type of the elements to be received;
				 - source specifies the rank of the sending process which sends the message;
				 - tag is the message tag that the message to be received must have;
				 - comm is the communicator used for the communication;
				 - status specifies a data structure which contains information about a message after the completion of the receive operation.
			*/

			int j;

			for (j = 0; j < part_array_size; j++) {
				prime_array[current_prime_array_index] = part_prime_array[j];
				current_prime_array_index++;
			}


			current_start_index = current_end_index + 1;
		}

		// Laufzeit-Berechnung:v
		double measured_time = MPI_Wtime() - t1;



		//average_measured_time = average_measured_time + measured_time;

		printf("Berechnung: %f s. ", measured_time);
		//fprintf(file_ptr, "%f;", measured_time);

		// Ueberpruefen, ob die berechneten Primzahlen korrekt sind:
		if (!check_if_calculation_is_right(prime_array)) {
			calculations_right = false;
			printf("Die Berechnung der Primzahlen weist Fehler auf.\n");
		} else {
			printf("Die Berechnung der Primzahlen ist korrekt.\n");
		}

	} else {	// Alle anderen Prozesse sind Slave-Prozesse, welche die eigentliche Arbeit verrichten.

		int receive_message_length = 2;	// Der Puffer, der gesendet wird, soll zwei long-Variablen enthalten.
		long * receive_message = (long *) malloc(receive_message_length * sizeof(long));		// Diese Nachricht wird an die jeweiligen Prozesse uebermittelt. Sie besteht aus 2 Longs, wobei der erste Long der Start-Index fuer die Berechnung und der zweite Long der End-Index fuer die Berechnung ist.
		MPI_Status status;


		MPI_Recv(receive_message, receive_message_length, MPI_LONG, MASTER_RANK, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

		long start_index = receive_message[START_INDEX];
		long end_index = receive_message[END_INDEX];

		printf("Ich bin Prozess %d und arbeite Index %ld bis %ld ab.\n", my_rank, start_index, end_index);


		/*
			int MPI Recv(void *rmessage,
			int count,
			MPI Datatype datatype,
			int source,
			int tag,
			MPI Comm comm,
			MPI Status *status)

			This operation is blocking. The parameters have the following meaning:
			 - rmessage specifies the receive buffer in which the message should be stored;
			 - count is the maximum number of elements that should be received;
			 - datatype is the data type of the elements to be received;
			 - source specifies the rank of the sending process which sends the message;
			 - tag is the message tag that the message to be received must have;
			 - comm is the communicator used for the communication;
			 - status specifies a data structure which contains information about a message after the completion of the receive operation.
		*/


		// Jeder Prozess erstellt nun ein Teil-Array, auf welchem er seine Berechnungen durchfuehrt.
		long part_array_size = (end_index - start_index) + 1;

		int * part_prime_array = (int *) malloc(part_array_size * sizeof(int));		// Diese Nachricht wird an die jeweiligen Prozesse uebermittelt. Sie besteht aus 2 Longs, wobei der erste Long der Start-Index fuer die Berechnung und der zweite Long der End-Index fuer die Berechnung ist.

		initialize_prime_array(part_prime_array, part_array_size);

		// Diese Funktion berechnet die Primzahlen.

		int i;
		int j;

		// Obwohl die ganzen Teil-Array alle Indizes von 0 bis (end_index - start_index) besitzen, wird trotzdem so getan, als waeren diese Teil-Arrays Teil des ganzen Arrays, um weiterhin mit den Indizes als potentielle Primzahlen-Kandidaten arbeiten zu koennen.

		printf("Ich bin Prozess %d und beginne jetzzt mit den Berechugnen\n", my_rank);
		fflush(stdout);
		for (i = 0; i <= (end_index - start_index); i++) {
			if ((i + start_index) == 0) {
				part_prime_array[i] = FALSE;		// 0 ist per Definition keine Primzahl.
			} else if ((i + start_index) == 1) {
				part_prime_array[i] = FALSE;		// 1 ist per Definition keine Primzahl.
			} else {
				for (j = 2; j < (i + start_index); j++) {
					if ((i + start_index) % j == 0) {


						
						//printf("Ich bin Prozess %d!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", my_rank);
						

						part_prime_array[i] = FALSE;
					}
				}
			}
		}

		printf("Ich bin Prozess %d und habe  Index %ld bis %ld abgearbeitet.\n", my_rank, start_index, end_index);

		MPI_Send(part_prime_array, part_array_size, MPI_INT, MASTER_RANK, 1, MPI_COMM_WORLD);

		/*
			int MPI Send(void *smessage,
			int count,
			MPI Datatype datatype,
			int dest,
			int tag,
			MPI Comm comm)

			The parameters have the following meaning:
			 - smessage specifies a send buffer which contains the data elements to be sent in successive order;
			 - count is the number of elements to be sent from the send buffer;
			 - datatype is the data type of each entry of the send buffer; all entries have the same data type;
			 - dest specifies the rank of the target process which should receive the data; each process of a communicator has a unique rank; the ranks are numbered from 0 to the number of processes minus one;
			 - tag is a message tag which can be used by the receiver to distinguish different messages from the same sender;
			 - comm specifies the communicator used for the communication. MPI default communicator MPI_COMM_WORLD is used for the communication. This communicator captures all processes executing a parallel program.
		*/


	}

	MPI_Finalize();		// MPI wird beendet. Von diesem Punkt an koennen KEINE Funktionen der MPI-Bibliothek mehr verwendet werden.
}

void initialize_prime_array(int prime_array[], int length) {
	int i;

	for (i = 0; i < length; i++) {
		prime_array[i] = TRUE;
	}
}

void find_prime_numbers_parallel_mpi(int prime_array[], long number_of_mpi_slave_processes) {
	// Diese Funktion teilt die Arbeit auf die verschiedenen MPI-Slave-Prozesse auf.

	long diffenence = ARRAY_SIZE / number_of_mpi_slave_processes;	// Anzahl von Indizes, die jeder Thread abarbeiten muss. Der letzte Thread muss den Rest abarbeiten, der uebrig bleibt, da die Rechnung nicht immer restlos ist.
	long current_start_index;
	long current_end_index;
	long i = 0;

	current_start_index = 0;

	for (i = 1; i <= number_of_mpi_slave_processes; i++) {
		if (i == number_of_mpi_slave_processes) {
			current_end_index = ARRAY_SIZE - 1;		// Letzter Thread arbeitet den Rest ab.
		} else {
			current_end_index = current_start_index + diffenence - 1;
		}
		
		int send_message_length = 2;	// Der Puffer, der gesendet wird, soll zwei long-Variablen enthalten.
		long * send_message = (long *) malloc(send_message_length * sizeof(long));		// Diese Nachricht wird an die jeweiligen Prozesse uebermittelt. Sie besteht aus 2 Longs, wobei der erste Long der Start-Index fuer die Berechnung und der zweite Long der End-Index fuer die Berechnung ist.

		send_message[START_INDEX] = current_start_index;
		send_message[END_INDEX] = current_end_index;
		
		

		printf("Master: Der Prozess mit der ID %ld bekommt die Arbeit %ld bis %ld\n", i, send_message[START_INDEX],send_message[END_INDEX]);
		MPI_Send(send_message, send_message_length, MPI_LONG, i, 0, MPI_COMM_WORLD);

		

		/*
			int MPI Send(void *smessage,
			int count,
			MPI Datatype datatype,
			int dest,
			int tag,
			MPI Comm comm)

			The parameters have the following meaning:
			 - smessage specifies a send buffer which contains the data elements to be sent in successive order;
			 - count is the number of elements to be sent from the send buffer;
			 - datatype is the data type of each entry of the send buffer; all entries have the same data type;
			 - dest specifies the rank of the target process which should receive the data; each process of a communicator has a unique rank; the ranks are numbered from 0 to the number of processes minus one;
			 - tag is a message tag which can be used by the receiver to distinguish different messages from the same sender;
			 - comm specifies the communicator used for the communication. MPI default communicator MPI_COMM_WORLD is used for the communication. This communicator captures all processes executing a parallel program.
		*/

		current_start_index = current_end_index + 1;
	}





	for (i = 1; i <= number_of_mpi_slave_processes; i++) {
		//pthread_join(threads[i], NULL);
		//free(thread_data[i]);
	}
}

bool check_if_calculation_is_right(int prime_array[]) {
	// Diese Funktion vergleicht die berechneten Primzahlen mit einer Datei, welche alle Primzahlen zwischen 1 und 100.000 enthaelt. Somit wird die Korrektheit der Berechnungen ueberprueft.

	FILE * file_ptr;
	int string_size = 7;	// Die groesste Zahl in der Datei ist 99991, enthaelt also 5 Zeichen. Mitsamt des Newline-Characters '\n', welcher von fgets() standardmaessig mit eingelesen wird, wird also die String-Groesse von 7 Zeichen nie ueberschritten.
	char read_prime_number[string_size];		// Aus der Datei ausgelesene Primzahl.
	bool calculation_is_right = true;
	long i;

	file_ptr = fopen("./prime_numbers_1_to_100.000.txt", "r");

	for (i = 2; i < ARRAY_SIZE; i++) {
		if (prime_array[i] == TRUE) {
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

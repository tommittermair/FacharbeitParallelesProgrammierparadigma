#!/bin/sh

number_of_repeats=50;	# Anzahl von Wiederholungen pro Anzahl von MPI-Prozessen.
current_number_of_mpi_processes=2;	# Da MPI mit Master- und Slave-Prozessen arbeitet, muessen mindestens 2 Prozesse erzeugt werden.
maximum_number_of_mpi_processes=253;	# In dieser Variable wird die maximale Anzahl an MPI-Prozessen gespeichert, die im Rahmen dieses Programmes erstellt werden. Auf meinem Rechner lassen sich nicht mehr als 253 MPI-Prozesse erzeugen. Dies habe ich durch Ausloten der Grenzen festgestellt.

printf "Parallele Berechnung der Primzahlen von 1 bis 100.000 mit Hilfe von MPI-Prozessen und $number_of_repeats Wiederholung(en):\n";


# Tabellen-Kopf der CSV-Datei erstellen:

printf "Anzahl von MPI-Prozessen;" > prime_number_calculation_parallel_mpi_csv.csv;

i=1
while [ $i -le $number_of_repeats ]; do
	printf "$i. Berechnung [s];" >> prime_number_calculation_parallel_mpi_csv.csv;
	i=$(( i + 1 ))
done

printf "\n" >> prime_number_calculation_parallel_mpi_csv.csv;


# Programm ausfuehren:

while [ $current_number_of_mpi_processes -le $maximum_number_of_mpi_processes ]; do

	printf "\tBerechnung mittels $current_number_of_mpi_processes MPI-Prozess(en):\n";
	printf "$current_number_of_mpi_processes;" >> prime_number_calculation_parallel_mpi_csv.csv;

	j=1
	while [ $j -le $number_of_repeats ]; do
		printf "\t\t$j. ";
		mpirun -np $current_number_of_mpi_processes ./prime_number_calculation_parallel_mpi;	# Das hier aufgerufene Programm gibt auch selbst Informationen auf der Konsole aus.
		j=$(( j + 1 ))
	done

	printf "\n" >> prime_number_calculation_parallel_mpi_csv.csv;

	current_number_of_mpi_processes=$(( current_number_of_mpi_processes + 1 ))
done

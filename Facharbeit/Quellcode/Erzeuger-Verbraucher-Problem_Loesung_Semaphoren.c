#define N 100 // Puffergroesse
semaphore mutex = 1; // Semaphor, der den Zugriff auf die kritische Bereiche regelt und somit Mutual Exclusion realisiert
semaphore empty = N; // Semaphor, der die leeren Pufferplaetze zaehlt
semaphore full = 0; // Semaphor, der die vollen Pufferplaetze zaehlt

void producer(void){ // Erzeuger
	int item; // Element
  
	while (1){ // Endlosschleife
		item = produce_item(); // Element wird erzeugt
		
		down(&empty); // Zaehler fuer leere Plaetze wird erniedrigt, somit wird die aktuelle Anzahl der leeren Pufferplaetze im Puffer aktualisiert
		down(&mutex); // Eintritt in den kritischen Bereich
		
		insert_item(item); // Erzeugtes Element wird in den Puffer gelegt
		
		up(&mutex); // Verlassen des kritischen Bereiches
		up(&full); // Zaehler fuer volle Plaetze wird erhoeht, somit wird die aktuelle Anzahl der vollen Pufferplaetze im Puffer aktualisiert
	}
}

void consumer(void) { // Verbraucher
	int item; // Element
	
	while (1){ // Endlosschleife
		down(&full); // Zaehler fuer vollen Plaetze wird erniedrigt, somit wird die aktuelle Anzahl der vollen Pufferplaetze im Puffer aktualisiert
		down(&mutex); // Eintritt in den kritischen Bereich
		
		item = remove_item(); // Element wird aus dem Puffer entnommen
		
		up(&mutex); // Verlassen des kritischen Bereiches
		up(&empty); // Zaehler fuer leeren Plaetze wird erhoeht, somit wird die aktuelle Anzahl der leeren Pufferplaetze im Puffer aktualisiert
		
		consume_item(item); // Entnommenes Element wird verbraucht
	}
}
#define N 100 // Puffergroesse
int count = 0; // Aktuelle Anzahl der Elemente im Puffer

void producer(void){ // Erzeuger
	int item; // Element
  
	while (1){ // Endlosschleife
		item = produce_item(); // Element wird erzeugt
		
		if (count == N) { // Falls der Puffer voll ist, muss der Erzeuger schlafen gehen
			SLEEP();
		}
		
		insert_item(item); // Erzeugtes Element wird in den Puffer gelegt
		count++; // Aktuelle Anzahl der Elemente im Puffer wird aktualisiert

		if (count == 1) { // Falls der Puffer vor dem Einfuegen des Elementes leer war, wird der Verbraucher geweckt
			WAKEUP(consumer);
		}
	}
}

void consumer(void) { // Verbraucher
	int item; // Element
	
	while (1){ // Endlosschleife
		if (count == 0) { // Falls der Puffer leer ist, muss der Verbraucher schlafen gehen
			SLEEP();
		}
		
		item = remove_item(); // Element wird aus dem Puffer entnommen
		count--; // Aktuelle Anzahl der Elemente im Puffer wird aktualisiert

		if (count == (N - 1)) { // Falls der Puffer vor dem Entnehmen des Elementes voll war, wird der Erzeuger geweckt
			WAKEUP(producer);
		}
		
		consume_item(item); // Entnommenes Element wird verbraucht
	}
}
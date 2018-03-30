enterRegion:						# Sprungmarke für das Unterprogramm, das das Eintreten in die kritische Region gewährt.
	TSL REGISTER, LOCK				# Die TSL-Anweisung wird aufgerufen, wodurch der aktuelle Wert des Speicherwortes LOCK im Register REGISTER gespeichert wird und in LOCK ein Wert ungleich 0 hineingespeichert wird.
	BNE REGISTER, 0, enterRegion	# Falls der Wert im Register REGISTER und damit der vorherige Wert von LOCK ungleich 0 ist, so springe zurück zur Sprungmarke enterRegion (Schleife). Ansonsten wird der folgende Code aufgeführt.
	RET								# Kehre zur Aufrufstelle zurück. Nach der Aufrufstelle erfolgt er Code des kritischen Bereiches.

leaveRegion:						# Sprungmarke für das Unterprogramm, das das Austreten aus der kritischen Region gewährt.
	MOVE LOCK, 0					# Kopiere den Wert 0 in die Sperr-Variable LOCK und gib damit die gemeinsame Ressource frei.
	RET								# Kehre zur Aufrufstelle zurück.
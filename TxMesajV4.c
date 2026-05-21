#include <c8051F040.h>	// declaratii SFR
#include <uart1.h>
#include <Protocol.h>
#include <UserIO.h>
#include <uart0.h>
extern unsigned char TIP_NOD;			// tip nod initial: Nu Master, Nu Jeton

extern nod retea[];

extern unsigned char timeout;		// variabila globala care indica expirare timp de asteptare eveniment
extern unsigned char AFISARE; // flag permitere afisare
//***********************************************************************************************************
void TxMesaj(unsigned char i);	// transmisie mesaj destinat nodului i

//***********************************************************************************************************
void TxMesaj(unsigned char i){					// transmite mesajul din buffer-ul i
	unsigned char sc, j;
	
	if(retea[i].bufbin.tipmes == POLL_MES)	// daca este un mesaj de interogare POLL_MES (sau JET_MES - au aceeasi valoare)
	{
		sc= retea[i].bufbin.adresa_hw_dest; // calculeaza direct sc
		sc^= retea[i].bufbin.adresa_hw_src; 
		retea[i].bufbin.sc= sc;
	}										
	// altfel...
	else
	{
		sc= retea[i].bufbin.adresa_hw_dest;	// initializeaza SC	cu adresa HW a nodului destinatie
		sc^= retea[i].bufbin.adresa_hw_src;	// ia in calcul adresa_hw_src
		sc^= retea[i].bufbin.tipmes;    	// ia in calcul tipul mesajului
		sc^= retea[i].bufbin.src;			// ia in calcul adresa nodului sursa al mesajului
		sc^= retea[i].bufbin.dest;			// ia in calcul adresa nodului destinatie al mesajului
		sc^= retea[i].bufbin.lng;			// ia in calcul lungimea datelor
																						
		for(j=0;j< retea[i].bufbin.lng;j++)
		{
			sc^= retea[i].bufbin.date[j];   // ia in calcul datele
		}
		
		retea[i].bufbin.sc= sc;              // stocheaza suma de control
	}																	
	
	UART1_MultiprocMode(MULTIPROC_ADRESA);	// urmeaza transmisia octetului de adresa (mod MULTIPROC_ADRESA)
	UART1_TxRxEN(1,1);						// validare Tx si Rx UART1
	UART1_RS485_XCVR(1,1);					// validare Tx si Rx RS485

	UART1_Putch(retea[i].bufbin.adresa_hw_dest);	// transmite adresa HW a nodului dest
																				
	// asteapta sa receptioneze caracterul transmis
	if(UART1_Getch_TMO(2) != retea[i].bufbin.adresa_hw_dest)	// daca caracterul primit e diferit de cel transmis sau a aparut timeout ...
	{	                          
		
		UART1_TxRxEN(0,0);					// dezactivare Tx UART1
		UART1_RS485_XCVR(0,0);				// dezactivare Tx RS485
		Error("\n\rDetectie coliziune!");	// afiseaza Eroare coliziune
		Delay(WAIT);						// asteapta WAIT msec
		return;								// termina transmisia (revine)
	}																
	
	UART1_MultiprocMode(MULTIPROC_DATA);	// urmeaza transmisia octetilor de date (mod MULTIPROC_DATA)
	UART1_TxRxEN(1,0);						// dezactivare Rx UART1
    
	UART1_Putch(retea[i].bufbin.adresa_hw_src);		// transmite adresa HW a nodului sursa
	UART1_Putch(retea[i].bufbin.tipmes);			// transmite tipul mesajului
	
	
	if(retea[i].bufbin.tipmes == USER_MES)			// Daca mesajul este de date ...
	{
		UART1_Putch(retea[i].bufbin.src);			// transmite adresa nodului sursa mesaj
		UART1_Putch(retea[i].bufbin.dest);			// transmite adresa nodului dest mesaj
		UART1_Putch(retea[i].bufbin.lng);			// transmite lungimea mesajului
    																				
		for(j=0;j<retea[i].bufbin.lng;j++)		// transmite octetii de date
		{
			UART1_Putch(retea[i].bufbin.date[j]);
		}
	}		
	
	UART1_Putch(retea[i].bufbin.sc);		// transmite suma de control
	
	UART1_TxRxEN(1,1);						// activare Rx UART1
																				
	if(TIP_NOD != MASTER)					// masterul nu goleste bufferul
	{
		retea[i].full=0;
	}	
	
	UART1_Getch(0);							// asteapta transmisia/receptia ultimului caracter

	UART1_TxRxEN(0,0);						// dezactivare Tx si RX UART1
	UART1_RS485_XCVR(0,0);					// dezactivare Tx si Rx RS485
}

//***********************************************************************************************************

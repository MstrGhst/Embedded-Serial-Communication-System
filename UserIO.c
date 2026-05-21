#include <c8051F040.h>						// declaratii SFR
#include <osc.h>
#include <Protocol.h>
#include <uart0.h>
#include <lcd.h>
#include <keyb.h>
#include <UserIO.h>

void Afisare_meniu(void);					// afisare meniu initial
void Afisare_mesaj(void);					// afisare mesaj receptionat
void Error(char *ptr);						// afisare mesaj de eroare

unsigned char TERM_Input(void);
unsigned char AFISARE = 1;

extern unsigned char LCD_line,LCD_col;

//***********************************************************************************************************
extern unsigned char ADR_MASTER;
extern unsigned char TIP_NOD;
extern unsigned char STARE_IO;
extern nod retea[];

//***********************************************************************************************************
void UserIO(void){					// interfata cu utilizatorul
	static unsigned char tasta, dest, lng;	// variabile locale statice
	
	if(0 == (tasta = TERM_Input())){
		tasta = KEYB_Input();
		if(tasta) LCD_Putch(tasta);
	}

	if(tasta){										// periodic verifica daca s-a apasat o tasta
		
		switch(STARE_IO){
			
			case 0:	// Starea S0 - Asteptare comanda (IDLE)
                switch(tasta){									
                    case '1': 											// s-a dat comanda de transmisie mesaj								
                        UART0_Putstr("\n\rTx Msg:> Nod = ");            // afiseaza Tx Msg:> Nod = 
                        LCD_DelLine(1);
                        LCD_PutStr(1,0, "Tx Msg:> Nod = ");
                        
                        AFISARE = 0;                                    // blocheaza afisarea mesajelor din task-ul de comunicatie
                        STARE_IO = 1;                                   // trece in starea 1 (conform schemei)
                        break;
                    
                    case '2': 											// s-a dat comanda de afisare Stare Nod:
                        UART0_Putstr("\n\rStare Nod:> ");
                        LCD_DelLine(1);
                        LCD_PutStr(1,0, "Stare Nod:> ");
                        
                        AFISARE = 0;                                    // blocheaza afisarea mesajelor
                        STARE_IO = 1;                                   // trece in starea 1 (conform schemei)
                        break;

                    default: 
                        break;
                }
                break;
									
			case 1:	// Starea S1 - Selectie adresa (Solicit adresa nod)
                // daca adresa este intre '0' - '2', mai putin adresa proprie
                if(tasta >= '0' && tasta <= '2' && tasta != (ADR_NOD + '0')){
                    dest = tasta - '0';                                 // extrage dest din tasta

                    // Daca este deja un mesaj in buffer ...
                    if(retea[dest].full){
                        Error("\n\rBuffer plin");                        // afiseaza Buffer plin
                        STARE_IO = 0;                                   // trece in starea 0 (corespunde cmd='2' din schema)
                        Afisare_meniu();                                // afisare meniu
                    }
                    else {
                        // Altfel, pregatire buffer (cmd='1' in schema)
                        if(TIP_NOD == MASTER){
                            retea[dest].bufbin.adresa_hw_dest = dest;           // Master: adresa hw dest egala cu dest
                        } else {
                            retea[dest].bufbin.adresa_hw_dest = ADR_MASTER;     // Slave: adresa hw dest egala cu ADR_MASTER
                        }

                        retea[dest].bufbin.adresa_hw_src = ADR_NOD;             // pune in bufferul dest adresa hw sursa egala cu ADR_NOD
                        retea[dest].bufbin.src = ADR_NOD;               // pune in bufferul dest adresa nodului sursa ADR_NOD
                        retea[dest].bufbin.dest = dest;                  // pune in bufferul dest adresa nodului destinatie (dest)
                        
                        UART0_Putstr("\n\rMesaj: ");                    // cere introducerea mesajului
                        LCD_DelLine(1);
                        LCD_PutStr(1,0, "Mesaj: ");
                        
                        lng = 0;                                        // initializeaza lng = 0 
                        STARE_IO = 2;                                   // trece in starea 2 (S2 din schema)
                    }
                }
                else {
                    // Daca se apasa o tasta invalida sau cmd='2', revenim in S0
                    STARE_IO = 0;
                    Afisare_meniu();
                }
                break;

			case 2:	// Starea S2 - Asteapta character mesaj
                // daca tasta e diferita de CR, de NL si de '*' si nu s-a ajuns la limita (MAX_DATA)
                if(tasta != '\r' && tasta != '\n' && tasta != '*' && lng < 16){ 
                    retea[dest].bufbin.date[lng] = tasta;               // stocheaza codul tastei
                    lng++;                                              // incrementeaza lng
                    // Bucla !tasta din schema te tine aici
                }
                else {
                    // Altfel (s-a apasat Enter, '*' sau buffer plin - ieșire S2 din schemă)
                    retea[dest].bufbin.lng = lng;                       // stocheaza lng
                    retea[dest].bufbin.tipmes = USER_MES;                  // pune in bufbin tipul mesajului
                    retea[dest].full = 1;                               // marcheaza buffer plin
                    
                    STARE_IO = 0;                                       // trece in starea 0
                    Afisare_meniu();                                    // afisare meniu
                }
                break;	
		}
	}
}

//***********************************************************************************************************
void Afisare_meniu(void){				  			// afisare meniu initial
	AFISARE = 1;
	UART0_Putstr("\n\rTema ");
	LCD_PutStr(0,0,"T");
	UART0_Putch(TEMA + '0');
	LCD_Putch(TEMA + '0');
	
#if(PROTOCOL == MS)
	if(TIP_NOD == MASTER){
		UART0_Putstr(" Master ");	// daca programul se executa pe nodul master
		LCD_PutStr(LCD_line, LCD_col, " Master:");
	}
	else{ 
		UART0_Putstr(" Slave ");						// daca programul se executa pe un nod slave
		LCD_PutStr(LCD_line, LCD_col, " Slave:");
	}
#elif(PROTOCOL == JT)
	if(TIP_NOD == JETON){
		UART0_Putstr(" Jeton ");
		LCD_PutStr(LCD_line, LCD_col, " Jeton:");
	}
	else{
		UART0_Putstr(" NoJet ");
		LCD_PutStr(LCD_line, LCD_col, "NoJet:");
	}
#endif
	
	UART0_Putch(ADR_NOD + '0');						// afiseaza adresa nodului
	LCD_Putch(ADR_NOD + '0');
#if(TEMA == 1 || TEMA == 3)
	UART0_Putstr(":ASC" );								// afiseaza parametrii specifici temei
	LCD_PutStr(LCD_line, LCD_col, " ASC");
#elif(TEMA == 2 || TEMA == 4)
	UART0_Putstr(":BIN" );
	LCD_PutStr(LCD_line, LCD_col, " BIN");
#endif
	UART0_Putstr("\n\r> 1-TxM 2-Stare :>");	// meniul de comenzi
	LCD_PutStr(1,0, "1-TxM 2-Stare :>");
}


//***********************************************************************************************************
void Afisare_mesaj(void){          		// afisare mesaj din bufferul de receptie i
	unsigned char j, lng, *ptr;
	if(retea[ADR_NOD].full){						// exista mesaj in bufferul de receptie?
		lng = retea[ADR_NOD].bufbin.lng;
		UART0_Putstr("\n\r>Rx: De la nodul ");
		LCD_DelLine(1);
		LCD_PutStr(1,0, "Rx: ");		
		UART0_Putch(retea[ADR_NOD].bufbin.src + '0');			// afiseaza adresa nodului sursa al mesajului
		LCD_Putch(retea[ADR_NOD].bufbin.src + '0');
		
		UART0_Putstr(": ");
		LCD_PutStr(LCD_line, LCD_col, ">: ");	
		
		for(j = 0, ptr = retea[ADR_NOD].bufbin.date; j < lng; j++) UART0_Putch(*ptr++);	// afiseaza mesajul, caracter cu caracter
		for(j = 0, ptr = retea[ADR_NOD].bufbin.date; j < lng; j++) LCD_Putch(*ptr++);		// afiseaza mesajul, caracter cu caracter

		retea[ADR_NOD].full = 0;					// mesajul a fost afisat, marcheaza buffer gol
	}
}

//***********************************************************************************************************
void Error(char *ptr){
	if(AFISARE){
		UART0_Putstr(ptr);
		LCD_DelLine(1);
		LCD_PutStr(1,0, ptr+2);
	}
}

unsigned char TERM_Input(void){

	unsigned char ch, SFRPAGE_save = SFRPAGE;
	
	SFRPAGE = LEGACY_PAGE;
	
	ch = 0;
	if(RI0) ch = UART0_Getch(1);
	
	SFRPAGE = SFRPAGE_save;
	
	return ch;
}
#include <c8051F040.h>      // declaratii SFR
#include <wdt.h>
#include <osc.h>
#include <port.h>
#include <uart0.h>
#include <uart1.h>
#include <lcd.h>
#include <keyb.h>
#include <Protocol.h>
#include <UserIO.h>

nod retea[NR_NODURI]; // retea cu 3 noduri

unsigned char STARE_COM = 0; // starea initiala a FSA COM
unsigned char STARE_IO = 0;  // stare initiala FSA interfata IO
unsigned char TIP_NOD = 0;   // tip nod initial: Slave sau No JET
unsigned char ADR_MASTER;    // adresa nod master

extern unsigned char AFISARE; // flag permitere afisare

// Prototipuri functii
void TxMesaj(unsigned char i); 
unsigned char RxMesaj(unsigned char i); 

void main(void) {
  unsigned char i, found;


  WDT_Disable();
  SYSCLK_Init();
  UART1_Init(NINE_BIT, BAUDRATE_COM); // port comunicatie RS-485
  UART1_TxRxEN(1, 1);

  PORT_Init(); 
  LCD_Init();
  KEYB_Init();
  UART0_Init(EIGHT_BIT, BAUDRATE_IO); // port USB-UART pentru IO
  Timer0_Init();

  EA = 1; // validare globala intreruperi
  SFRPAGE = LEGACY_PAGE;

  // Golire buffere la initiere
  for (i = 0; i < NR_NODURI; i++) {
    retea[i].full = 0;
    retea[i].bufasc[0] = ':';
  }

  Afisare_meniu();

  while (1) {
    switch (STARE_COM) {
    case 0: // --- STARE RECEPTIE (SLAVE) ---
      #if(PROTOCOL == MS)
      // Nodul asteapta un mesaj. Daca nu primeste nimic, devine Master
      switch (RxMesaj(ADR_NOD)) {
      case TMO:
        // Eroare de timeout: magistrala e libera, preluam controlul
        TIP_NOD = MASTER; 
        ADR_MASTER = ADR_NOD;
        if (AFISARE) {
          Afisare_meniu();
        }
        
        /* MODIFICARE: Initializam i = 0. 
           In Case 2, prima instructiune este i = (i + 1) % NR_NODURI.
           Astfel, prima interogare va fi trimisa catre Nodul 1.
        */
        i = 0; 
        STARE_COM = 2; // Trecem direct in starea de Master (interogare)
        break;

      case ROK:
        Afisare_mesaj(); // Am primit date utile
//        break; 
        
      case POK:
        STARE_COM = 1; // Am fost interogati, trecem sa raspundem
        break; 

      default:
        break;
      }
      #endif
      break;

    case 1: // --- STARE RASPUNS (SLAVE) ---
      #if(PROTOCOL == MS)
      found = 0;
      // Cautam daca avem date pregatite de utilizator pentru Master
      for (i = 0; i < NR_NODURI; i++) {
        if (retea[i].full) {
          found = 1;
          retea[i].bufbin.adresa_hw_dest = ADR_MASTER;
          TxMesaj(i);
          retea[i].full = 0; 
          break;
        }
      }

      // Daca nu avem date, trimitem un pachet de tip POLL (gol)
      if (!found) {
        retea[ADR_MASTER].bufbin.adresa_hw_dest = ADR_MASTER;
        retea[ADR_MASTER].bufbin.adresa_hw_src = ADR_NOD;
        retea[ADR_MASTER].bufbin.tipmes = POLL_MES;
        TxMesaj(ADR_MASTER);
      }

      STARE_COM = 0; // Revenim in receptie
      #endif
      break;

    case 2: // --- STARE INTEROGARE (MASTER) ---
      #if(PROTOCOL == MS)
      // Selectam urmatorul nod din lista
      i = (i + 1) % NR_NODURI;
      
      // Daca am ajuns la propria adresa, sarim peste ea
      if (i == ADR_NOD) {
        i = (i + 1) % NR_NODURI;
      }

      retea[i].bufbin.adresa_hw_dest = i;

      if (retea[i].full) {
        // Trimitem datele pregatite pentru Slave-ul i
        TxMesaj(i);
        // Masterul nu sterge bufferul aici daca e protocol de retransmisie, 
        // dar in TxMesaj se ocupa de flag
      } else {
        // Nu avem date, facem doar prezenta (Poll)
        retea[i].bufbin.adresa_hw_src = ADR_NOD;
        retea[i].bufbin.tipmes = POLL_MES;
        TxMesaj(i);
      }

      STARE_COM = 3; // Asteptam raspunsul de la Slave
      #endif
      break;

    case 3: // --- STARE ASTEPTARE RASPUNS (MASTER) ---
      #if(PROTOCOL == MS)
      // Masterul asteapta raspunsul de la nodul interogat
      switch (RxMesaj(i)) {
      case TMO:
				Error("\n\rTimeout Nod: ");	// afiseaza Eroare coliziune
			//UART1_Putstr(retea[i].bufbin.adresa_hw_dest +'0'); //afisare la care e problema	
		if(AFISARE){
			UART0_Putch( i+'0');
			LCD_Putch(i+'0');
		}// Slave-ul i nu a raspuns
        break; 
      case ROK:
        Afisare_mesaj(); // Am primit date de la Slave
        break; 
      case POK:
        // Slave-ul a confirmat ca e activ dar nu are date
        break; 
      default:
        break;
      }

      STARE_COM = 2; // Trecem la urmatorul Slave din lista
      #endif
      break;
    }

    UserIO(); // Task-ul de interfata utilizator
  }
}
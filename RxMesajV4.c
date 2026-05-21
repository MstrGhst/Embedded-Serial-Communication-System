#include <c8051F040.h> // declaratii SFR

#include <uart1.h>

#include <Protocol.h>

#include <UserIO.h>

extern nod retea[]; // reteaua cu 3 noduri

extern unsigned char TIP_NOD; // tip nod
extern unsigned char ADR_MASTER; // adresa nodului master

extern unsigned char timeout; // variabila globala care indica expirare timp de asteptare eveniment

//***********************************************************************************************************
unsigned char RxMesaj(unsigned char i); // primire mesaj de la nodul i

//***********************************************************************************************************
unsigned char RxMesaj(unsigned char i) {
  unsigned char j, ch, sc, adresa_hw_src, screc, src, dest, lng, tipmes;

  UART1_TxRxEN(0, 1); // dezactivare Tx, validare RX UART1
  UART1_RS485_XCVR(0, 1); // dezactivare Tx, validare RX RS485
  UART1_MultiprocMode(MULTIPROC_ADRESA); // receptie doar octeti de adresa (mod MULTIPROC_ADRESA)

  if (TIP_NOD == MASTER) { // Daca nodul este master sau detine jetonul ...
    ch = UART1_Getch_TMO(WAIT); // M: asteapta cu timeout raspunsul de la slave
    if (timeout == 1) return TMO; // M: timeout, terminare receptie, devine master sau regenerare jeton

    if(retea[i].full == 1)
      retea[i].full = 0; // M: raspunsul de la slave a venit, considera ca mesajul anterior a fost transmis cu succes
    if (ch != ADR_NOD) return ERA; // M: adresa HW gresita, terminare receptie
  } else {
	  do{
	  
      ch = UART1_Getch_TMO(2 * WAIT + ADR_NOD * WAIT); // S: asteapta inceput mesaj nou

      if (timeout == 1) return TMO; // S: nu exista activitate in retea
	  }
      while(ch != ADR_NOD); 
    
  }

  UART1_MultiprocMode(MULTIPROC_DATA); // receptie octeti de date (mod MULTIPROC_DATA)

  screc = ADR_NOD; // M+S: initializeaza screc cu adresa HW dest (ADR_NOD)

  adresa_hw_src = UART1_Getch_TMO(5); // M+S: Asteapta cu timeout receptia adresei HW sursa
  if (timeout == 1) return CAN; // M+S: mesaj incomplet

  screc ^= adresa_hw_src; // M+S: ia in calcul in screc adresa hw src

  if (TIP_NOD == SLAVE) ADR_MASTER = adresa_hw_src; // S: actualizeaza adresa master

  tipmes = UART1_Getch_TMO(5); // M+S: Asteapta cu timeout receptie tip mesaj
  if (timeout == 1) return CAN; // M+S: mesaj incomplet

  if (tipmes > 1) return TIP; // M+S: tip mesaj eronat, terminare receptie

  screc ^= tipmes; // M+S: ia in calcul in screc tipul mesajului

  if (tipmes == USER_MES) { // M+S: Daca mesajul este unul de date (USER_MES)
    src = UART1_Getch_TMO(5); // M+S: asteapta cu timeout adresa nodului sursa
    if (timeout == 1) return CAN;
    screc ^= src; // M+S: ia in calcul in screc adresa src

    dest = UART1_Getch_TMO(5); // M+S: asteapta cu timeout adresa nodului destinatie
    if (timeout == 1) return CAN;
    screc ^= dest; // M+S: ia in calcul in screc adresa dest

    if (TIP_NOD == MASTER) { // Daca nodul este master...
      if (retea[dest].full == 1) return OVR; // M: bufferul destinatie este deja plin, terminare receptie
    }

    lng = UART1_Getch_TMO(5); // M+S: asteapta cu timeout receptia lng
    if (timeout == 1) return CAN;
    screc ^= lng; // M+S: ia in calcul in screc lungimea datelor

    if (TIP_NOD == MASTER) { // Daca nodul este master...
      retea[dest].bufbin.adresa_hw_src = ADR_NOD; // M: stocheaza adresa HW sursa = ADR_NOD
      retea[dest].bufbin.tipmes = tipmes; // M: stocheaza tipul mesajului
      retea[dest].bufbin.src = src; // M: stocheaza la src codul nodului sursa al mesajului
      retea[dest].bufbin.dest = dest; // M: stocheaza la dest codul nodului destinatie a mesajului
      retea[dest].bufbin.lng = lng; // M: stocheaza lng

      for (j = 0; j < lng; j++) { // M: asteapta cu timeout un octet de date
        retea[dest].bufbin.date[j] = UART1_Getch_TMO(5);
        if (timeout == 1) return CAN;
        screc ^= retea[dest].bufbin.date[j]; // M: ia in calcul in screc datele
      }

      sc = UART1_Getch_TMO(5); // M: Asteapta cu timeout receptia sumei de control
      if (timeout == 1) return CAN;

      if (sc == screc) { // Daca sc e corect...
        retea[dest].full = 1; // M: mesaj corect, marcare buffer plin
        return ROK;
      } else { // Altfel...
        return ESC; // M: eroare SC, terminare receptie
      }
    } else { // Altfel (nodul este slave)
      retea[ADR_NOD].bufbin.src = src; // S: stocheaza la src codul nodului sursa al mesajului
      retea[ADR_NOD].bufbin.lng = lng; // S: stocheaza lng

      for (j = 0; j < lng; j++) { // S: asteapta cu timeout un octet de date
        retea[ADR_NOD].bufbin.date[j] = UART1_Getch_TMO(5);
        if (timeout == 1) return CAN;
        screc ^= retea[ADR_NOD].bufbin.date[j]; // S: ia in calcul in screc octetul de date
      }

      sc = UART1_Getch_TMO(5); // S: Asteapta cu timeout receptia sumei de control
      if (timeout == 1) return CAN;

      if (sc == screc) { // Daca sc e corect...
        retea[ADR_NOD].full = 1; // S: mesaj corect, marcare buffer plin
        return ROK;
      } else { // Altfel...
        return ESC; // S: eroare SC, terminare receptie
      }
    }
  } else { // Altfel (este un mesaj POLL_MES sau JET_MES)
    retea[ADR_NOD].bufbin.adresa_hw_src = adresa_hw_src; // JT: memoreaza de la cine a primit jetonul

    sc = UART1_Getch_TMO(5); // M+S: Asteapta cu timeout receptia sumei de control
    if (timeout == 1) return CAN;

    if (sc == screc) { // M+S:Daca sc e corect, returneaza POK
      return POK;
    } else {
      return ESC; // M+S: Altfel, eroare SC, terminare receptie
    }
  }
}
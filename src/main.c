#define BAUD 115200
#define MYUBRRF F_CPU/8/BAUD -1 //full duplex

void uart1_Init(unsigned int UBRR) { // Baud Rate Register
  UCSR0A = (1 << U2X0);  // double speed
  UCSR0B |= (1 << RXEN0) | (1 << TXEN0); // enable receive and trasmit
  UCSR0C |= (1 << UVSZ00) | (1 << UCSZ01); // 8 bit symboler kommunikeres
  UBRR0H = (unsigned char)(UBRR >> 8);
  UBRR0L = (unsigned char) UBRR; // vi bruger 2 bytes til at gemme hele UART pakken
}

char getchUSART0(void)  {
  while(!(UCSR0A & (1 << RXC0))); // wait until a character is received
  return UDR0;
}

char putchUSART0 (char tx) {
  while(!(UCSR0A & (1 << UDRE0)));  // wait for wait for empty transmit buffer
}

int main()  {
  uart1_Init(MYUBRRF);
//tissemand
}
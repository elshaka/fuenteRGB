#define STX 0x02
#define ETX 0x03

// LEDs y valvula
sbit led_rojo at PORTC.B0;
sbit led_verde at PORTC.B1;
sbit led_azul at PORTC.B2;
sbit valvula at PORTC.B3;

// PWM
unsigned short int periodo;
unsigned short int ciclo_rojo, ciclo_verde, ciclo_azul;
unsigned short int color_rojo, color_verde, color_azul;

// Serial
unsigned short int id;
unsigned short int network_broadcast_id;
unsigned short int indice;
unsigned short int uart_rx;
unsigned short int uart_rojo, uart_verde, uart_azul;
unsigned short int transicion;
bit uart_valvula;

// Configuracion inicial
void init() {
  // Puerto A
  TRISA = 0;
  PORTA = 0;
  // Puerto B
  TRISB = 1;
  INTCON.RBIE = 0;
  // Puerto C
  TRISC = 0b10000000;
  PORTC = 0;

  ADCON1 = 6;
  T1CON = 0;

  // PWM
  color_rojo = 0x00;
  color_verde = 0xff;
  color_azul = 0x00;
  transicion = 0;

  // Valvula
  valvula = 1;

  // UART
  indice = 0;
  UART1_Init(9600);
  Delay_ms(100);
  INTCON.PEIE = 1;
  PIE1.RCIE = 1;

  // TIMER0
  INTCON.TMR0IE = 0;
  INTCON.TMR0IF = 0;
  OPTION_REG = 0b100;

  // Habilitar interrupciones
  INTCON.GIE = 1;
}

void main() {
  id = PORTB;
  network_broadcast_id = id & 0b11100000 | 0b00011111;
  init();
  // PWM
  periodo = 1;
  while(1) {
    periodo--;
    if(STATUS.Z) {
      ciclo_rojo = color_rojo;
      ciclo_verde = color_verde;
      ciclo_azul = color_azul;
      led_rojo = !color_rojo;
      led_verde = !color_verde;
      led_azul = !color_azul;
    } else {
      if(!ciclo_rojo)
        led_rojo = 1;
      else
        ciclo_rojo--;
      if(!ciclo_verde)
        led_verde = 1;
      else
        ciclo_verde--;
      if(!ciclo_azul)
        led_azul = 1;
      else
        ciclo_azul--;
    }
  }
}

// Interrupciones
void interrupt() {
  // UART
  if(PIR1.RCIF) {
    uart_rx = UART1_Read();
    if(uart_rx == STX)
      indice = 1;
    else switch(indice) {
      case 0:
        break;
      case 1:
        if(uart_rx == id || uart_rx == network_broadcast_id)
          indice = 2;
        else
          indice = 0;
        break;
      case 2:
        uart_valvula = uart_rx.B0;
        indice = 3;
        break;
      case 3:
        uart_rojo = uart_rx;
        indice = 4;
        break;
      case 4:
        uart_verde = uart_rx;
        indice = 5;
        break;
      case 5:
        uart_azul = uart_rx;
        indice = 6;
        break;
      default:
        indice = 0;
        if(uart_rx == ETX) {
          valvula = uart_valvula;
          transicion = 0b111;
          INTCON.TMR0IE = 1;
        }
    }
  }

  // TIMER0
  if(INTCON.TMR0IF) {
    INTCON.TMR0IF = 0;
    if(transicion) {
      if(color_rojo == uart_rojo)
        transicion.B0 = 0;
      else if(color_rojo < uart_rojo)
        color_rojo++;
      else
        color_rojo--;
      if(color_verde == uart_verde)
        transicion.B1 = 0;
      else if(color_verde < uart_verde)
        color_verde++;
      else
        color_verde--;
      if(color_azul == uart_azul)
        transicion.B2 = 0;
      else if(color_azul < uart_azul)
        color_azul++;
      else
        color_azul--;
    } else INTCON.TMR0IE = 0;
  }
}

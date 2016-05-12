#ifndef APP_H
#define APP_H

// ###########################################################################################################################################
// DESENVOLVIMENTO
#define LOG_MODBUS		pdON

// ###########################################################################################################################################
// MODEL WORKTEMP

#define MODEL			"ms10"	// somente 4 chars, por causa so modbus
#define FIRMWARE_VERSION "1.0"	// usar x.y só com uma casa decimal, pois estamos usando a impressão de temperatura
								// usar 3 chars por causa do modbus
#define PROGAM_NAME "MSIP" MODEL " firmware v" FIRMWARE_VERSION " [compiled in " __DATE__ " " __TIME__ "]" CMD_TERMINATOR


#define nTHERMOMETER  		 9    	// Quantidade de termômetro que o RH deve gerenciar. O ID modbus do termômetro deve ser sequencial de 1 a quantidade de termômetro
#define EXP_nSLOTS_SAMPES 	 600  	// 600 = 10 minutos. Quantos slots guardaremos para as amostras das temperaturas
#define nBUFFER_SAMPLES_READ 50  	// Quantidade de registradores dos resultado vamos ler por vez. Máximo 120



// ###########################################################################################################################################
// INCLUDES


// ###########################################################################################################################################
// MODBUS
#define COM_PORT "/dev/ttyAMA0" 	// Consultar /dev/ttySxxx do linux
									// Opensuse com placa pci para serial = "/dev/ttyS4"
									// Raspberry "/dev/ttyAMA0"
									// Para descobrir qual o nó da porta UART faça o seguinte:
									// 		Conecte o RASP com um PC via cabo serial.
									//		No PC com um terminal de comnicação serial abra uma conexão com RASP a 115200bps 8N1
									//		No RAPS liste as portas tty: ls tty*
									// 		No RAPS na linha de comando envie uma mensagem para cada porta serial: echo ola /dev/ttyXXXX
									//			Faça para para todas as portas listadas até que receba a mensagem no terminal do PC

#define MODBUS_BAUDRATE		B57600 // Não usar acima de 57600, pois há erro de recepção do raspberry.
									// Deve ser algum bug de hardware do rasp porque o baudrate do rasp não fica indentico do ARM
									// pois a comunicação com PC a 115200 funciona bem.
									// Ou a tolerancia de erro no rasp não é tão grande como no PC onde o ARM tem um erro considerável
									//	TODO Quando usar o oscilador interno do ARM refazer os testes a sabe com usando oscilador interno do ARM isso se resolve

// ###########################################################################################################################################
// CONTROLE DO SISTEMA

typedef enum {
	cmdNONE = 0,
	cmdGET_INFO,        // RH: Comando para ler os registradores: modelo do RH e versão do firmware
						// NetWork: Pegar informações de controle do experimento e valores dos termômetros
    cmdGET_ACTION,      // RH: Comando para ler que ação o recurso de hardware está fazendo agora
                                // 0: O recurso de hardware não está fazendo nada;
                                // 1: O recurso de hardware está fazendo um experimento ou uma configuração nos termômetros.
    cmdGET_STATUS,      // RH: Comando para o status do experimento
                                // 0: Sinaliza que o experimento ou processo de configuração dos termômetros foi feito com sucesso;
                                // 1: Não foi possível se comunicar pelo menos com um termômetro;
                                // 2: Pelo menos um dos termômetros deu erro de leitura de temperatura, ou a mesma ficou fora dos limites de trabalho
    cmdGET_SAMPLES,     // RH: Comando para pegar quantas amostras foram tiradas durante experimento. Máixma 600 amostra = 10 minutos de experimento
    cmdGET_RESULTS,     // RH/Network: Comando para ler as amostras tiradas durante experimento de todos os termômetros. Máximo 600 amostra = 10 minutos de experimento
    cmdEXP_START,       // RH/Network: Comando para dar início ao experimento
    cmdEXP_STOP,        // RH/Network: Comando para parar o experimento
    cmdGET_TEMP,        // RH/Network: Comando para ler as temperaturas
    cmdRST_STAT_TEMP,   // RH/Network: Comando para iniciar a estatisticas de temperatura
    cmdSET_TEMP_TYPE    // RH/Network: Comando para ajustar a grandeza da temperatura
                                // 0 - Ajuste via jumper
                                // 1 - Celsius
                                // 2 - Fahrenheit
} tCommand;

typedef struct {
	int stsCom;				// Status de comunicação:
							//		0: Sem comunicação com o dispositivo. O mesmo não está conectado, ou está desligado, ou não há dispositivo neste endereço.
							//		1: O multímetro recebeu uma função que não foi implementada;
							//		2: Foi acessado a um endereço de registrador inexistente;
							//		3: Foi tentado gravar um valor inválido no registrador do multímetro;
							//		4: Um irrecuperável erro ocorreu enquanto o multímetro estava tentando executar a ação solicitada;
							//		5: Comunicação estabelecida com sucesso
	int sts;				// Status do sensor de temperatura
							//      0: Sinaliza que o WorkTemp está lendo pela primeira vez a temperatura.
							//    		Isto somente acontece no momento que o WorkTemp é ligado.
							//      1: O WorkTemp já contém valor da temperatura;
							//      2: Sinaliza um erro, indica que a temperatura está abaixo da escala
							//          permitida pelo WorkTemp. Ou seja, a conexão com o sensor
							//          está em curto, seja o sensor em si ou o fio que se liga a ele;
							//      3: Sinaliza um erro, indica que a temperatura está acima da escala
							//          permitida pelo WorkTemp. Ou seja, a conexão com o sensor está aberta,
							//          ou o sensor está danificado, ou o fio do sensor está partido
							//          ou não está conectado ao WorkTemp.

	int temperatureType;	//  Sinaliza se a grandeza da temperatura:
							//      0: Graus Celsius;
                    		//      1: Graus Fahrenheit.
	int value; 				// valor da temperatura * 10
	int valueMin; 			// valor da temperatura * 10
	int valueMax; 			// valor da temperatura * 10
} tThermometer;


typedef struct {
	// flash de controle de ação para acesso ao RH. Esses flags devem ser atualizados via rede
	unsigned exit:1;		// 1 Sinaliza para cair fora do programa
	unsigned getInfo:1;		// Sinaliza para capturar as informações do recurso de hardware
	unsigned getTemp:1;
	unsigned setTempType:1;
	unsigned rstTempStatistics:1;
	unsigned getAction:1;
	unsigned getStatus:1;
	unsigned getSamples:1;
	unsigned getResult:1;
	unsigned expStart:1;
	unsigned expStop:1;
	unsigned expGetResult:1;	// Sinaliza para capturar as informações das saídas digitais do recurso de hardware

	int stsCom;			// Status de comunicação com RH via modbus
							//		0: Sem comunicação com o RH. O mesmo não está conectado, ou está desligado, ou não há dispositivo neste endereço.
							//		1: O RH recebeu uma função que não foi implementada;
							//		2: Foi acessado a um endereço de registrador inexistente;
							//		3: Foi tentado gravar um valor inválido no registrador do RH;
							//		4: Um irrecuperável erro ocorreu enquanto o RH estava tentando executar a ação solicitada;
							//		5: Comunicação estabelecida com sucesso
	uint rhID;			// Qual o ID do RH no barramento modbus
	string rhModel;		// Modelo do RH
	string rhFirmware;	// Firmware versão
	tThermometer thermometer[nTHERMOMETER];
	uint tempType;
	int thermAccessAddr;  							// Endereço do termometro a ser lido
    int nThermAccessSamples; 						// Quantidade de valores devemos ler no acesso. Valor máximo de 120
    int thermBufferSamples[nBUFFER_SAMPLES_READ];   // Buffer temporário da leitura dos registradores das asmostras de temperatura lida do RH
    int action;			// Sinaliza que ação o recurso de hardware está fazendo agora
                    		// 0: O recurso de hardware não está fazendo nada;
                    		// 1: O recurso de hardware está fazendo um experimento ou uma configuração nos termômetros.
	int status;     	// Guarda o status do experimento
                    		// 0: Sinaliza que o experimento ou processo de configuração dos termômetros foi feito com sucesso;
                    		// 1: Não foi possível se comunicar pelo menos com um termômetro;
                   	 		// 2: Pelo menos um dos termômetros deu erro de leitura de temperatura, ou a mesma ficou fora dos limites de trabalho
    int nSamples;       // Quantidades de amostras de todos os termômetros tiradas durante o experimento
    int expResult[nTHERMOMETER][EXP_nSLOTS_SAMPES]; // resultado dos experimentos
} tControl, *pControl;

typedef enum {readREGS, writeREG, writeREGS} tcmd;


// ###############################################################################
// PROTOTIPOS
int modbus_Init(void);
void modbus_SendCommand( tCommand c) ;
void init_control_tad();
void * modbus_Process(void * params);

#endif

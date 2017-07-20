#include "timer/timer.h"
#include "_config_cpu_.h"
#include "uart/uart.h"
#include "modbus/modbus_master.h"
#include <nan.h>
#include "app.h"
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <v8.h>
#include <string.h>

#define TIMEOUT 50

using namespace v8;
pthread_t id_thread;
static int waitResponse = pdFALSE;
static tCommand cmd;
static u16 regs[120]; // registrador de trabalho para troca de dados com os multimetros
tControl control;
int kelvin_enable = 0;

int modbus_Init(void) {
    //fprintf(flog, "Abrindo UART %s"CMD_TERMINATOR, COM_PORT);
#if (LOG_MODBUS == pdON)
    printf("Abrindo UART %s"CMD_TERMINATOR, COM_PORT);
#endif
    if (uart_Init(COM_PORT, MODBUS_BAUDRATE) == pdFAIL) {
        //fprintf(flog, "Erro ao abrir a porta UART"CMD_TERMINATOR);
        //fprintf(flog, "    Verifique se essa porta n�o esteja sendo usada por outro programa,"CMD_TERMINATOR);
        //fprintf(flog, "    ou se o usu�rio tem permiss�o para usar essa porta: chmod a+rw %s"CMD_TERMINATOR, COM_PORT);
#if (LOG_MODBUS == pdON)
        printf("Erro ao abrir a porta UART"CMD_TERMINATOR);
        printf("    Verifique se essa porta n�o esteja sendo usada por outro programa,"CMD_TERMINATOR);
        printf("    ou se o usu�rio tem permiss�o para usar essa porta: chmod a+rw %s"CMD_TERMINATOR, COM_PORT);
#endif
        return pdFAIL;
    }

    //fprintf(flog, "Port UART %s aberto com sucesso"CMD_TERMINATOR, COM_PORT);
#if (LOG_MODBUS == pdON)
    printf("Port UART %s aberto com sucesso a %d bps"CMD_TERMINATOR, COM_PORT, MODBUS_BAUDRATE);
#endif
    modbus_MasterInit(uart_SendBuffer, uart_GetChar, uart_ClearBufferRx);
    modbus_MasterAppendTime(now, 3000);

    return pdPASS;
}



// para grava��o os valores dos registradores devem estar preenchidos
// ap�s a leitura os registradores estar�o preenchidos

// Envia um comando para o escravo, nesta mesma fun��o � feito o procedimento de erro de envio
// Uma vez enviado o comando com sucesso caprturar resposta no modbus_Process
//	BUSY: Ficar na espera da resposta
//  ERROR: Notificar  o erro e tomar procedimento cab�veis
//  OK para escrita: Nada, pois os valores dos registradores foram salvos no escravo com sucesso
//	OK para Leitura: Capturar os valores dos registradores lidos do escravo
// Parametros
// c: Tipo de comando para ser enviado ao escravo
// funcResponse: Ponteiro da fun��o para processar a resposta da comunica��o

void modbus_SendCommand(tCommand c) {
    if (waitResponse) return; // pdFAIL;

    cmd = c;
    // APONTA QUAIS REGISTRADORES A ACESSAR NO DISPOSITIVO
    // -----------------------------------------------------------------------------------------------------------------

    // Comando para ler os registradores: modelo, vers�o firmware e modo de opera��es dos reles
    tcmd typeCMD = readREGS;
    uint addrInit = 0;
    uint nRegs = 3;
    u16 value = 0;
    int x;
    // Comando para ler que a��o o recurso de hardware est� fazendo agora
    if (cmd == cmdGET_ACTION) {
        addrInit = 0x11;
        nRegs = 1;

        // Comando para o status do experimento
    } else if (cmd == cmdGET_STATUS) {
        addrInit = 0x12;
        nRegs = 1;

        // Comando para pegar quantas amostras foram tiradas durante experimento. M�ixma 600 amostra = 10 minutos de experimento
    } else if (cmd == cmdGET_SAMPLES) {
        addrInit = 0x13;
        nRegs = 1;

        // Comando para ajustar a grandeza da temperatura
    } else if (cmd == cmdSET_TEMP_TYPE) {
        typeCMD = writeREG;
        addrInit = 0x14;
        nRegs = 1;
        value = control.tempType;

        // Comando para dar in�cio ao experimento
    } else if (cmd == cmdEXP_START) {
        typeCMD = writeREG;
        addrInit = 0x15;
        nRegs = 1;

        // Comando para parar o experimento
    } else if (cmd == cmdEXP_STOP) {
        typeCMD = writeREG;
        addrInit = 0x16;
        nRegs = 1;

        // Comando para iniciar a estatisticas de temperatura
    } else if (cmd == cmdRST_STAT_TEMP) {
        typeCMD = writeREG;
        addrInit = 0xA001;
        nRegs = 1;

        // Comando para ler as temperaturas
    } else if (cmd == cmdGET_TEMP) {
        addrInit = 0x400;
        nRegs = nTHERMOMETER * 5; // incluindo regs de temps min e max

        // Comando para ler as amostras tiradas durante experimento de todos os term�metros.
    } else if (cmd == cmdGET_RESULTS) {
        addrInit = control.thermAccessAddr;
        if (control.nThermAccessSamples >= nBUFFER_SAMPLES_READ) control.nThermAccessSamples = nBUFFER_SAMPLES_READ;
        nRegs = control.nThermAccessSamples;
        for (x = 0; x < nBUFFER_SAMPLES_READ; x++) control.thermBufferSamples[x] = 0;
    }


    // ENVIA O COMANDO AO DISPOSITIVO ESCRAVO
    // -----------------------------------------------------------------------------------------------------------------
    int ret;
    if (typeCMD == writeREG) {
#if (LOG_MODBUS == pdON)
        printf("modbus WriteReg [cmd %d] [slave %d] [reg 0x%x] [value 0x%x]"CMD_TERMINATOR, cmd, control.rhID, addrInit, value);
#endif
        ret = modbus_MasterWriteRegister(control.rhID, addrInit, value);
    } else if (typeCMD == writeREGS) {
#if (LOG_MODBUS == pdON)
        printf("modbus WriteRegs [cmd %d] [slave %d] [reg 0x%x] [len %d]"CMD_TERMINATOR, cmd, control.rhID, addrInit, nRegs);
#endif
        ret = modbus_MasterWriteRegisters(control.rhID, addrInit, nRegs, regs);
    } else {
#if (LOG_MODBUS == pdON)
        printf("modbus ReadRegs [cmd %d] [slave %d] [reg 0x%x] [len %d]"CMD_TERMINATOR, cmd, control.rhID, addrInit, nRegs);
#endif
        ret = modbus_MasterReadRegisters(control.rhID, addrInit, nRegs, regs);
    }


    // se foi enviado com sucesso ficaremos na espera da resposta do recurso de hardware
    if (ret == pdPASS) {
        waitResponse = pdTRUE;
#if (LOG_MODBUS == pdON)
        //else fprintf(flog, "modbus err[%d] send querie"CMD_TERMINATOR, modbus_MasterReadStatus());
    } else {
        waitResponse = pdTRUE;
        printf("modbus err[%d] SEND querie"CMD_TERMINATOR, modbus_MasterReadStatus());
    }
#endif

    return;
}

void init_control_tad() {
    int x;
    control.rhID = 1; // Sinaliza que o endere�o do RH no modbus � 1
    control.getInfo = 0; // sinaliza para pegar as informa��es do RH
    control.getTemp = 0;
    control.setTempType = 0;
    control.rstTempStatistics = 0;
    control.expStart = 0;
    control.expStop = 0;
    control.getAction = 0;
    control.getStatus = 0;
    control.getSamples = 0;
    control.getResult = 0;
    control.expGetResult = 0;
    control.exit = 0;
    control.stsCom = 0;
    control.thermAccessAddr = 0;
    control.nThermAccessSamples = 0;
    control.action = 0;
    control.status = 0;
    control.nSamples = 0;

    for (x = 0; x < nBUFFER_SAMPLES_READ; x++)
        control.thermBufferSamples[x] = 0;
    memset(control.rhModel, '\0', __STRINGSIZE__);
    memset(control.rhFirmware, '\0', __STRINGSIZE__);
    for (x = 0; x < nTHERMOMETER; x++) {
        control.thermometer[x].stsCom = 0;
        control.thermometer[x].sts = 0;
    }
    return;

}

// processo do modbus.
//	Neste processo gerencia os envios de comandos para o recurso de hardware e fica no aguardo de sua resposta
//	Atualiza as variaveis do sistema de acordo com a resposta do recurso de hardware.

void * modbus_Process(void * params) {

    int first = 0;
    while (control.exit == 0) {
        if (first) {
            sleep(0.2);
        }
        first = 1;

        modbus_MasterProcess();

        // Gerenciador de envio de comandos
        // se n�o estamos esperando a resposta do SendCommand vamos analisar o pr�ximo comando a ser enviado
        if (!waitResponse) {
            // checa se � para pegar as informa��es do RH
            if (control.getInfo) {
                modbus_SendCommand(cmdGET_INFO);

            } else if (control.expStart) {
                modbus_SendCommand(cmdEXP_START);
                control.expStart = 0;

            } else if (control.expStop) {
                modbus_SendCommand(cmdEXP_STOP);
                control.expStop = 0;

            } else if (control.getResult) {
                modbus_SendCommand(cmdGET_RESULTS);

            } else if (control.getTemp) {
                modbus_SendCommand(cmdGET_TEMP);

            } else if (control.setTempType) {
                modbus_SendCommand(cmdSET_TEMP_TYPE);
                
            } else if (control.rstTempStatistics) {
                modbus_SendCommand(cmdRST_STAT_TEMP);

            } else if (control.getAction) {
                modbus_SendCommand(cmdGET_ACTION);

            } else if (control.getStatus) {
                modbus_SendCommand(cmdGET_STATUS);

            } else if (control.getSamples) {
                modbus_SendCommand(cmdGET_SAMPLES);

            }

            continue;
        }


        int ret = modbus_MasterReadStatus();
        //	BUSY: Ficar na espera da resposta
        //  ERROR: Notificar  o erro e tomar procedimento cab�veis
        //  OK para escrita: Nada, pois os valores dos registradores foram salvos no escravo com sucesso
        //	OK para Leitura: Capturar os valores dos registradores lidos do escravo

        // se ainda est� ocupado n�o faz nada

        if (ret == errMODBUS_BUSY) continue;
        waitResponse = pdFALSE;
        // se aconteceu algum erro
        if (ret < 0) {
#if (LOG_MODBUS == pdON)
            //fprintf(flog, "modbus err[%d] wait response "CMD_TERMINATOR, ret);
            printf("modbus err[%d] WAIT response "CMD_TERMINATOR, ret);
#endif
            control.stsCom = modbus_MasterReadException();
            // modbusILLEGAL_FUNCTION: O multimetro recebeu uma fun��o que n�o foi implementada ou n�o foi habilitada.
            // modbusILLEGAL_DATA_ADDRESS: O multimetro precisou acessar um endere�o inexistente.
            // modbusILLEGAL_DATA_VALUE: O valor contido no campo de dado n�o � permitido pelo multimetro. Isto indica uma falta de informa��es na estrutura do campo de dados.
            // modbusSLAVE_DEVICE_FAILURE: Um irrecuper�vel erro ocorreu enquanto o multimetro estava tentando executar a a��o solicitada.
            continue;
        }

        control.stsCom = 5; // sinaliza que a conex�o foi feita com sucesso

        // ATUALIZA VARS QUANDO A COMUNICA��O FOI FEITA COM SUCESSO
        // -----------------------------------------------------------------------------------------------------------------
        int x;
        // Comando para ler os registradores: modelo e vers�o firmware do RH
        if (cmd == cmdGET_INFO) {
#if (LOG_MODBUS == pdON)
            printf("model %c%c%c%c"CMD_TERMINATOR, (regs[0] & 0xff), (regs[0] >> 8), (regs[1] & 0xff), (regs[1] >> 8));
            printf("firware %c.%c"CMD_TERMINATOR, (regs[2] & 0xff), (regs[2] >> 8));
#endif
            control.rhModel[0] = (regs[0] & 0xff);
            control.rhModel[1] = (regs[0] >> 8);
            control.rhModel[2] = (regs[1] & 0xff);
            control.rhModel[3] = (regs[1] >> 8);
            control.rhModel[4] = 0;
            control.rhFirmware[0] = (regs[2] & 0xff);
            control.rhFirmware[1] = (regs[2] >> 8);
            control.rhFirmware[2] = 0;

            control.getInfo = 0; // sinalizo para n�o pegar mais informa��es

            // comando para ajuste dos reles, vamos sinalizar para n�o enviar mais comandos
        } else if (cmd == cmdGET_ACTION) {
            control.action = regs[0];
            control.getAction = 0; // sinalizo para n�o pegar mais a��o

            // Comando para o status do experimento
        } else if (cmd == cmdGET_STATUS) {
            control.status = regs[0];
            control.getStatus = 0; // sinalizo para n�o pegar mais status
            // Comando para pegar quantas amostras foram tiradas durante experimento
            // Comando para o status do experimento
        } else if (cmd == cmdGET_SAMPLES) {
            control.nSamples = regs[0];
            control.getSamples = 0; // sinalizo para n�o pegar mais status

            // Comando para ler os term�metros
        } else if (cmd == cmdGET_TEMP) {

            for (x = 0; x < nTHERMOMETER; x++) {
                control.thermometer[x].stsCom = regs[5 * x] & 0xff;
                control.thermometer[x].temperatureType = (regs[(5 * x) + 1] & 0x10) >> 4;
                control.thermometer[x].sts = regs[(5 * x) + 1] & 0xf;
                control.thermometer[x].value = regs[(5 * x) + 2];
                control.thermometer[x].valueMin = regs[(5 * x) + 3];
                control.thermometer[x].valueMax = regs[(5 * x) + 4];

#if (LOG_MODBUS == pdON)
                printf("THERMOMETER[%d] stsCom 0x%x TempType %d sts 0x%x value %d"CMD_TERMINATOR,
                        x + 1,
                        control.thermometer[x].stsCom,
                        control.thermometer[x].temperatureType,
                        control.thermometer[x].sts,
                        control.thermometer[x].value
                        );
#endif
            }

            control.getTemp = 0; // sinalizo para n�o pegar mais a temperatura

            // Comando para ler as amostras tiradas durante experimento de todos os term�metros.
        } else if (cmd == cmdGET_RESULTS) {
            for (x = 0; x < control.nThermAccessSamples; x++)
                control.thermBufferSamples[x] = regs[x];
            control.getResult = 0; // sinalizo para n�o pegar mais os resultados do experimento
        }
    }

    return NULL;
}

NAN_METHOD(Setup) {
    NanScope();
    FILE *flog = fopen("msip.log", "w");
    int counter=0;

    while (control.exit && counter < TIMEOUT) {
        usleep(100000);
        counter++;
    }
    
    if (counter >= TIMEOUT) {
        NanReturnValue(NanNew("0"));
    }

    if (modbus_Init() == pdFAIL) {
        fprintf(flog, "Erro ao abrir a porta UART"CMD_TERMINATOR);
        fprintf(flog, "Verifique se essa porta n�o esteja sendo usada por outro programa,"CMD_TERMINATOR);
        fprintf(flog, "ou se o usu�rio tem permiss�o para usar essa porta: chmod a+rw %s"CMD_TERMINATOR, COM_PORT);
        NanReturnValue(NanNew("0"));

    }

    fclose(flog);

    NanReturnValue(NanNew("1"));
}

NAN_METHOD(Digital) {
    NanScope();

    if (args.Length() < 1) {
        NanThrowTypeError("Wrong number of arguments");
        NanReturnUndefined();
    }

    if (!args[0]->IsNumber()) {
        NanThrowTypeError("Wrong arguments");
        NanReturnUndefined();
    }
    int sel = args[0]->NumberValue();

    if (sel) {
        control.expStart = 1;
        printf("Ligando aquecimento\n");

    } else {
        control.expStop = 1;
        printf("Desligando aquecimento\n");
    }

    while (!control.expStop && !control.expStart) {
        sleep(0.2);
    }

    NanReturnValue(NanNew("1"));
}

NAN_METHOD(Run) {
    NanScope();
    char argv[15];
    std::string str_argv = *v8::String::Utf8Value(args[0]->ToString());
    strcpy(argv, str_argv.c_str());
    init_control_tad();
    
    int rthr = pthread_create(&id_thread, NULL, modbus_Process, (void *) 0); // fica funcionando at� que receba um comando via WEB para sair		
    if (rthr) {
        printf("Unable to create thread void * modbus_Process");

    }

    kelvin_enable = 0;
    /*
if(!strcmp(argv,"fahrenheit")){
            control.tempType = 1;
    }else if (!strcmp(argv,"kelvin")){
            kelvin_enable = 2731;
    }else{
            control.tempType = 0;
    }	
	
    control.setTempType = 1; */

    NanReturnValue(NanNew("1"));

}

NAN_METHOD(Exit) {
    NanScope();
    init_control_tad();
    control.getInfo = 0;
    control.exit = 1;
    uart_Close();
    printf("Fechando programa"CMD_TERMINATOR);
    NanReturnValue(NanNew("1"));
}

NAN_METHOD(GetValues) {
    NanScope();
    int i;
    char value[10];
    std::string buffer_temp = std::string("\"thermometers\":[");
    control.getTemp = 1;
    while (control.getTemp) {
        sleep(0.2);
    }

    for (i = 0; i < nTHERMOMETER; i++) {
        if (control.thermometer[i].sts == 1)
            sprintf(value, "%.1f", ((double) (control.thermometer[i].value + kelvin_enable) / 10));
        else
            sprintf(value, "%.1f", (double) 0);

        if (i == nTHERMOMETER - 1)
            buffer_temp = buffer_temp + std::string(value);
        else
            buffer_temp = buffer_temp + std::string(value) + std::string(",");

    }
    buffer_temp = buffer_temp + std::string("]");

    NanReturnValue(NanNew("{" + buffer_temp + "}"));

}

NAN_METHOD(GetAllValues) {
    NanScope();
    int i;
    char value[10];
    std::string buffer_temp = std::string("\"thermometers\":["); // Montar arquivo JSON
    control.thermAccessAddr = 0x1000; // Log do primeiro term�metro 
    control.getResult = 1;
    while (control.getResult) {
        sleep(0.2);
    }
    printf("Pegando todos valores\n");

    for (i = 0; i < control.nThermAccessSamples; i++) {

        sprintf(value, "%.1f", (double) (control.thermBufferSamples[i] / 10));

        if (i == control.nThermAccessSamples - 1)
            buffer_temp = buffer_temp + std::string(value);
        else
            buffer_temp = buffer_temp + std::string(value) + std::string(",");

    }

    buffer_temp = buffer_temp + std::string("]");

    NanReturnValue(NanNew("{" + buffer_temp + "}"));
}

void Init(Handle<Object> exports) { // cria fun��es para serem invocadas na aplica��o em nodejs
    exports->Set(NanNew("run"), NanNew<FunctionTemplate>(Run)->GetFunction());
    exports->Set(NanNew("setup"), NanNew<FunctionTemplate>(Setup)->GetFunction());
    exports->Set(NanNew("exit"), NanNew<FunctionTemplate>(Exit)->GetFunction());
    exports->Set(NanNew("getvalues"), NanNew<FunctionTemplate>(GetValues)->GetFunction());
    exports->Set(NanNew("getallvalues"), NanNew<FunctionTemplate>(GetAllValues)->GetFunction());
    exports->Set(NanNew("digital"), NanNew<FunctionTemplate>(Digital)->GetFunction());
}


NODE_MODULE(panel, Init)

